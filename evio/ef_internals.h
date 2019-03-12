#ifndef EF_INTERNALS_H_INCLUDED
#define EF_INTERNALS_H_INCLUDED

#include <ef_io.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <ev_spin_lock.h>
#include <ev_queue.h>

typedef enum {
	oper_open=0,
	oper_read,
	oper_close,
	oper_readahead,
	oper_write
} file_oper_type;

#define FILE_NOT_OPEN 0
#define FILE_OPEN_ATTEMPTED 1
#define FILE_OPEN 2

#define ACTION_INITIATED 0
#define ACTION_IN_PROGRESS 2
#define ACTION_READAHEAD_COMPLETE 3
#define ACTION_COMPLETE 4

struct eio_open_oper_s {
	const char		*_path;
	int				_oflag;
	mode_t			_mode;
};
#define eio_open_oper_init(inp) { \
	(inp)._path = NULL; \
	(inp)._oflag = 0; \
	(inp)._mode = 0; \
}

struct eio_read_oper_s {
	off_t			_offset;
	size_t			_size;
};

#define eio_read_oper_init(inp) { \
	(inp)._offset = 0; \
	(inp)._size = 0; \
}

typedef enum {
	file_write_evt = 0,
	file_fsync_evt
} write_task_evt_type;

struct write_task_s {
	write_task_evt_type		_cmd;
	void					*_buf;
	size_t					_nbyte;
	struct write_task_s 	*_next;
};

struct data_buf_s {
	void *				_buf;
	ssize_t				_nbyte;
	int					_errno;
	struct data_buf_s *	_next;
	long				_page_no;
};

struct buf_list_s {
	ev_queue_type		_queue;
};

struct accum_writes_s {
	void *						_data;
	size_t						_bytes;
	int							_first_write;
};

struct eio_write_oper_s {
	ev_queue_type				_w_queue; // Queue containing write requests.
	struct write_task_s *		_b_w_list; // Batched w_t list
	struct accum_writes_s 		_w_accum;
	long						_write_totreq_count;
	long						_write_req_count;
	long						_write_cmpl_count;
};

#define eio_write_oper_init(inp) { \
	(inp)._w_queue = create_ev_queue(); \
	(inp)._b_w_list = NULL; \
	(inp)._w_accum._data = NULL; \
	(inp)._w_accum._bytes = 0; \
	(inp)._w_accum._first_write = 1; \
	(inp)._write_totreq_count = 0; \
	(inp)._write_req_count = 0; \
	(inp)._write_cmpl_count = 0; \
}

union eio_inp {
	struct eio_open_oper_s	_open_inp;
	struct eio_read_oper_s	_read_inp;
	struct eio_write_oper_s	_write_inp;
};

struct eio_file_cmd {
	file_oper_type	_cmd;
	int				_action_status;
	int				_return_value;
	int				_err;
	union eio_inp	_inp;
};

#define SET_WRITE_ACTION_STATUS(a) { \
	if ((a) && ((a)->_cmd == oper_write)) {\
		if ((a)->_inp._write_inp._write_totreq_count == (a)->_inp._write_inp._write_cmpl_count) { \
			(a)->_action_status = ACTION_COMPLETE; \
		} \
	} \
}

#define low_eio_cmd_init(a,oper) { \
	a->_action_status = ACTION_INITIATED; \
	a->_return_value = -100; \
	a->_err = 0; \
	switch ((oper)) { \
		case oper_open: \
			a->_cmd = oper_open; \
			eio_open_oper_init(a->_inp._open_inp); \
			break; \
		case oper_read: \
			a->_cmd = oper_read; \
			eio_read_oper_init(a->_inp._read_inp); \
			break; \
		case oper_readahead: \
			a->_cmd = oper_readahead; \
			eio_read_oper_init(a->_inp._read_inp); \
			break; \
		case oper_write: \
			a->_cmd = oper_write; \
			break; \
		case oper_close: \
			a->_cmd = oper_close; \
			break; \
		default: \
			free(a); \
			a = NULL; \
			break; \
	} \
}

#define eio_cmd_init(p,oper) { \
	struct eio_file_cmd * a = NULL;\
	a = malloc(sizeof(struct eio_file_cmd)); \
	low_eio_cmd_init(a,oper); \
	if (oper ==  oper_write) { \
		a->_cmd = oper_write; \
		eio_write_oper_init(a->_inp._write_inp); \
	} \
	(p)->_action = a; \
}

struct buf_s {
	void *				_buffer;
	int					_buffer_index;
	int					_is_dirty;
	struct buf_list_s	_r_a_list;
	int					_r_a_started;
	int					_r_a_completed;
};

/* File entry
 * Contains details needed for operating a file
 */
typedef struct _s {
	int						_ef_fd;
	int						_fd;
	int						_oflag;
	atomic_int				_status;
	atomic_int				_outstanding_writes;
	atomic_int				_write_suspended;
	off_t					_orig_file_size_on_disk;
	off_t					_curr_file_size_on_disk;
	off_t					_running_file_size;
	int						_block_write_unaligned;
	off_t					_file_offset;
	struct buf_s			_buf;
	struct eio_file_cmd		*_action;
	atomic_int				_prevent_action_mutation;
	/*
	int						in_sync;
	int						in_write;
	*/
} EF_FILE;

/* printf("[%s:%d] Reached [%d] \n",s,l,(a)->_cmd);fflush(stdout); \*/

/*
#define EF_FILE_ACTION_CLEANUP(s,l,a) { \
	*/
#define EF_FILE_ACTION_CLEANUP(p) { \
	if ((p)->_action->_cmd == oper_write) { \
		destroy_ev_queue((p)->_action->_inp._write_inp._w_queue); \
		(p)->_action->_inp._write_inp._w_queue = NULL; \
		if ((p)->_action->_inp._write_inp._b_w_list) { \
			struct write_task_s * w_t = NULL, *ptr = NULL; \
			w_t = (p)->_action->_inp._write_inp._b_w_list; \
			while (w_t) { \
				ptr = w_t; \
				w_t = w_t->_next; \
				free(ptr->_buf); \
				free(ptr); \
				ptr = NULL; \
			} \
			(p)->_action->_inp._write_inp._b_w_list = NULL; \
		} \
		if ((p)->_action->_inp._write_inp._w_accum._data) { \
			free((p)->_action->_inp._write_inp._w_accum._data); \
			(p)->_action->_inp._write_inp._w_accum._data = NULL; \
		} \
	} \
	else if ((p)->_action->_cmd == oper_open) { \
		if ((p)->_action->_inp._open_inp._path) free((void*)((p)->_action->_inp._open_inp._path)); \
		(p)->_action->_inp._open_inp._path = NULL; \
	} \
	free((p)->_action); \
	(p)->_action = NULL; \
}

/* printf("[%s:%d] Reached\n",s,l);fflush(stdout); \ */

/*
#define EF_FILE_CLEANUP(s,l,f) { \
*/
#define EF_FILE_CLEANUP(f) { \
	if ((f)->_action) { \
		free_file_action((f)); \
	} \
	if ((f)->_buf._buffer) \
		free((f)->_buf._buffer); \
	(f)->_buf._buffer = NULL; \
	destroy_ev_queue((f)->_buf._r_a_list._queue); \
	(f)->_buf._r_a_list._queue = NULL; \
	free((f)); \
	(f) = NULL; \
}

#define EF_FILE_INIT(f) { \
	f->_fd = -1; \
	f->_oflag = 0; \
	f->_status = FILE_NOT_OPEN; \
	f->_outstanding_writes = 0; \
	f->_write_suspended = 0; \
	f->_orig_file_size_on_disk = 0; \
	f->_curr_file_size_on_disk = 0; \
	f->_running_file_size = 0; \
	f->_block_write_unaligned = 0; \
	f->_buf._buffer_index = 0; \
	f->_buf._buffer = NULL; \
	f->_buf._is_dirty = 0; \
	f->_buf._r_a_started = 0; \
	f->_buf._r_a_completed = 0; \
	f->_buf._r_a_list._queue = create_ev_queue(); \
	f->_action = NULL; \
	f->_prevent_action_mutation = 0; \
}
/*
f->in_sync = 0; \
f->in_write = 0; \
*/

#define EF_FILE_SIZE sizeof(EF_FILE)

/* We support maximum of 128 open files
 * Can be overridden later with parameterization */
#define EF_MAX_FILES 128

#ifdef __APPLE__
#include "TargetConditionals.h"
#if TARGET_OS_IPHONE && TARGET_IPHONE_SIMULATOR
#define TARGET_OS_ONLY_IPHONE_SIMULATOR 1
#elif TARGET_OS_IPHONE
#define TARGET_OS_UNION_IPHONE 1
#else
#define TARGET_OS_OSX 1
#endif
#endif

#endif
