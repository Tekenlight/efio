#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>
#include <thread_pool.h>
#include <sys/stat.h>

#include <ev_globals.h>
#include <ef_io.h>
#include <ef_internals.h>
#include <ev_spin_lock.h>
#include <ev_queue.h>

typedef enum {
	fw_invalid_evt=0,
	fw_write_req_evt,
	fw_write_cmpl_evt,
	fw_shutdown_evt
}file_writer_cmd_type;

struct file_writer_task_s {
	file_writer_cmd_type	_cmd;
	EF_FILE *				_file_ptr;
};

typedef enum {
	fs_invalid_evt=0,
	fs_shutdown_evt
} slow_sync_cmd_type;

struct slow_sync_task_s {
	slow_sync_cmd_type	_cmd;
};

static int ef_one_single_write(EF_FILE *file_ptr, struct write_task_s * w_t);

struct ef_file_write_s {
	EF_FILE *					_file_ptr;
};

/* TBD: API of close has to be cleaned up. */
struct ef_file_read_s {
	EF_FILE *	_file_ptr;
	long		_curr_page_no;
	int			_which;
};

struct close_task_s {
	EF_FILE *f_ptr;
	int		f_index;
	struct eio_file_cmd *_action;
};

static atomic_int sg_handle_to_file_action[EF_MAX_FILES] = { } ;
static EF_FILE * sg_open_files[EF_MAX_FILES] = {};
static ev_queue_type sg_fd_queue = NULL;;
static thread_pool_type sg_disk_io_thr_pool = NULL;;
static thread_pool_type sg_file_writer_pool = NULL;;
static thread_pool_type sg_slow_sync_pool = NULL;;
static ev_queue_type sg_file_writer_queue = NULL;;
static pthread_mutex_t sg_file_writer_mutex;
static pthread_cond_t sg_file_writer_cond;
static ev_queue_type sg_slow_sync_queue = NULL;;
static size_t sg_page_size = 0;
static pthread_t sg_file_writer_tid;
static ef_notification_f_type sg_cb_func = NULL;
static void * sg_cb_data = NULL;
static int sg_ef_init_done = 0;
static int sg_thr_pool_init_done = 0;
static void file_writer(void * data);
static void slow_sync(void * data);
static void alloc_file_action(EF_FILE * file_ptr, file_oper_type oper);
static void free_file_action(EF_FILE * file_ptr);
static void low_ef_sync(EF_FILE * file_ptr);
static void sync_buf_to_file(EF_FILE *file_ptr);
static void sync_file_writes(int fd);

struct _fd_s {
	int fd;
};

static void st_enqueue_sg_fd_queue(int fd)
{
	//EV_DBGP("Here\n");
	struct _fd_s* ptr = malloc(sizeof(struct _fd_s));
	ptr->fd = fd;
	enqueue(sg_fd_queue,ptr);

	return;
}

static int st_dequeue_sg_fd_queue()
{
	void * p = dequeue(sg_fd_queue);
	if (p == NULL) return -1;
	int fd = ((struct _fd_s*)p)->fd;
	free(p);
	return fd;
}

static void setup_fd_slots()
{
	//EV_DBGP("Here\n");
	for (int i = 0 ; i < EF_MAX_FILES ; i++) {
		st_enqueue_sg_fd_queue(i);
	}
	/*
	for (int i = 0 ; i < EF_MAX_FILES ; i++) {
		enqueue(sg_fd_queue,(void*)(long)i);
	}
	for (int i = 1 ; i < (EF_MAX_FILES+1) ; i++) {
		enqueue(sg_fd_queue,(void*)(long)i);
	}
	*/
	return ;
}

static void ef_file_open(void * data)
{
	EF_FILE		*file_ptr = data;
	// If action is null, it is not known what to do.
	errno = 0;
	if (!file_ptr->_action)
	{
		return;
	}
	errno = 0;

	file_ptr->_action->_action_status = ACTION_IN_PROGRESS;

	if (O_CREAT&(file_ptr->_action->_inp._open_inp._oflag)) {
#ifdef __linux__
		if (!((O_APPEND|O_WRONLY|O_RDWR)&(file_ptr->_action->_inp._open_inp._oflag))) {
			file_ptr->_action->_inp._open_inp._oflag |= O_DIRECT;
		}
#endif
		file_ptr->_fd = open(file_ptr->_action->_inp._open_inp._path,
							file_ptr->_action->_inp._open_inp._oflag,
							file_ptr->_action->_inp._open_inp._mode);
	}
	else {
#ifdef __linux__
		if (!((O_APPEND|O_WRONLY|O_RDWR)&file_ptr->_action->_inp._open_inp._oflag)) {
			file_ptr->_action->_inp._open_inp._oflag |= O_DIRECT;
		}
#endif
		file_ptr->_fd = open(file_ptr->_action->_inp._open_inp._path,
							file_ptr->_action->_inp._open_inp._oflag);
	}

	if (file_ptr->_fd >= 0) {
		struct stat buf;
		//fcntl(file_ptr->_fd,F_GLOBAL_NOCACHE,1);
#ifdef __APPLE__
		fcntl(file_ptr->_fd,F_NOCACHE,1);
#endif
		fstat(file_ptr->_fd,&buf);
		file_ptr->_orig_file_size_on_disk = file_ptr->_curr_file_size_on_disk = buf.st_size;
		file_ptr->_running_file_size = file_ptr->_curr_file_size_on_disk;
		file_ptr->_file_offset = 0;
		file_ptr->_block_write_unaligned = 0;
		file_ptr->_oflag = file_ptr->_action->_inp._open_inp._oflag;
		if (O_APPEND&file_ptr->_oflag) {
			if (O_WRONLY&file_ptr->_oflag) {
				file_ptr->_file_offset = lseek(file_ptr->_fd,0,SEEK_END);
				if (file_ptr->_file_offset % sg_page_size) {
					file_ptr->_block_write_unaligned = 1;
				}
#ifdef __linux__
				else {
					int flg = fcntl(file_ptr->_fd,F_GETFL);
					flg |= O_DIRECT;
					fcntl(file_ptr->_fd,F_SETFL,flg);
				}
#endif
			}
#ifdef __linux__
			else if (O_RDWR&file_ptr->_oflag) {
				if (file_ptr->_running_file_size % sg_page_size) {
					file_ptr->_block_write_unaligned = 1;
				}
				else {
					int flg = fcntl(file_ptr->_fd,F_GETFL);
					flg |= O_DIRECT;
					fcntl(file_ptr->_fd,F_SETFL,flg);
				}
			}
#endif
		}
		file_ptr->_buf._buffer_index = (file_ptr->_file_offset / sg_page_size);
	}

	file_ptr->_action->_return_value = file_ptr->_fd;
	file_ptr->_action->_err = errno;
	file_ptr->_action->_action_status = ACTION_COMPLETE;
	file_ptr->_status = (file_ptr->_fd == -1)?FILE_NOT_OPEN:FILE_OPEN;
	atomic_thread_fence(memory_order_acq_rel);
	void* cb_data = sg_cb_data;
	atomic_thread_fence(memory_order_acq_rel);
	ef_notification_f_type cb_func = sg_cb_func;
	if (cb_func) cb_func(file_ptr->_ef_fd, oper_open, cb_data);
	return ;
}

/*
 * Much more work needs to be done at this point.
 * It is required that this procedure be completed
 * post the development of read and write procedures.
 * All data that is writen and not yet synced to the disk
 * should be synced and then the file should be closed.
 * Right now, this is only a cover function.
 * */
static void ef_file_close(void * data)
{
	EF_FILE		*file_ptr = data;
	int			ret = 0;
	int			file_index;
	struct close_task_s *ptr = NULL;

	ptr = (struct close_task_s *)data;
	file_ptr = ptr->f_ptr;
	file_index = ptr->f_index;
	free(ptr);
	ptr = NULL;
	// If action is null, it is not known what to do.
	errno = 0;
	if (!file_ptr->_action) return;

	file_ptr->_action->_action_status = ACTION_IN_PROGRESS;

	free(file_ptr->_buf._buffer);
	file_ptr->_buf._buffer = NULL;

	ret = close(file_ptr->_fd);
	file_ptr->_action->_return_value = ret;
	file_ptr->_action->_err = errno;
	file_ptr->_status = FILE_NOT_OPEN;
	file_ptr->_action->_action_status = ACTION_COMPLETE;
	atomic_thread_fence(memory_order_acq_rel);
	void* cb_data = sg_cb_data;
	atomic_thread_fence(memory_order_acq_rel);
	ef_notification_f_type cb_func = sg_cb_func;
	if (cb_func) cb_func(file_ptr->_ef_fd, oper_close, cb_data);
	/* Clean up the action, file data etc. as soon as the file is closed.
	 * Return data handling not required. */
	ef_close_status(file_ptr->_ef_fd);

	return ;
}

/* This needs to be cleaned up
 * This should handle outstanding data to be written. */
static int ef_close_api(int fd , bool immediate)
{
	int ret = 0;
	errno = 0;
	if (!sg_ef_init_done) {
		errno = EBADF;
		EV_ABORT("INIT NOT DONE");
	}
	if (!sg_open_files[fd]) { errno = EINVAL; return -1; }

	if (sg_open_files[fd]->_status != FILE_OPEN) {
		errno = EBADF;
		return -1;
	}
	EF_FILE * file_ptr = sg_open_files[fd];

	atomic_thread_fence(memory_order_acquire);
	if (sg_open_files[fd]->_action && sg_open_files[fd]->_action->_cmd == oper_close) {
		errno = EBUSY;
		return -1;
	}
	else if (sg_open_files[fd]->_action && sg_open_files[fd]->_action->_cmd != oper_write &&
								sg_open_files[fd]->_action->_action_status != ACTION_COMPLETE) {
		errno = EAGAIN;
		return -1;
	}
	else if (sg_open_files[fd]->_action && sg_open_files[fd]->_action->_action_status == ACTION_COMPLETE) {
		/* Close has been called before checking for the return value of
		 * the previous action. */
		free_file_action(sg_open_files[fd]);
	}

	sync_file_writes(fd);
	atomic_thread_fence(memory_order_acquire);
	SET_WRITE_ACTION_STATUS(sg_open_files[fd]->_action);
	while (sg_open_files[fd]->_action && sg_open_files[fd]->_action->_action_status != ACTION_COMPLETE) {

		EV_YIELD();
		atomic_thread_fence(memory_order_acquire);
		SET_WRITE_ACTION_STATUS(sg_open_files[fd]->_action);
	}


	if (sg_open_files[fd]->_action) {
		free_file_action(sg_open_files[fd]);
	}

	alloc_file_action(sg_open_files[fd],oper_close);

	if (immediate) {
		/* This memory is freed in ef_file_close. */
		struct close_task_s *ptr = malloc(sizeof(struct close_task_s));
		ptr->f_ptr = sg_open_files[fd];
		ptr->f_index = fd;
		ef_file_close(ptr);
	}
	else {
		/* This memory is freed in ef_file_close. */
		struct close_task_s *ptr = malloc(sizeof(struct close_task_s));
		ptr->f_ptr = sg_open_files[fd];
		ptr->f_index = fd;
		enqueue_task(sg_disk_io_thr_pool,ef_file_close,ptr);
		ret = 0;
	}
	return ret;
}

static void ef_file_read_ahead(void *data)
{
	struct ef_file_read_s * file_read_ptr = (struct ef_file_read_s *)data;
	EF_FILE *file_ptr = file_read_ptr->_file_ptr;
	off_t offset = file_read_ptr->_curr_page_no * sg_page_size;
	int	which = file_read_ptr->_which;
	long page_no = file_read_ptr->_curr_page_no + which;

	int ret = 0;
	struct data_buf_s * buf_list_ptr = NULL;

	free(file_read_ptr);
	file_read_ptr = NULL;

	{

		errno = 0;
		/* This memory is freed in the function ef_read and ef_file_close. */
		buf_list_ptr = malloc(sizeof(struct data_buf_s));
		buf_list_ptr->_next = NULL;
		/* This memory is freed in the function ef_read and ef_file_close.
		 * Along with freeing of _buffer. */
		{
			if (posix_memalign(&(buf_list_ptr->_buf),sg_page_size,sg_page_size))
				EV_ABORT("Cannot allocate memory");
		}
		//memset(buf_list_ptr->_buf,0,sg_page_size);

		buf_list_ptr->_nbyte = pread(file_ptr->_fd,buf_list_ptr->_buf, sg_page_size, page_no*sg_page_size);
		buf_list_ptr->_page_no = page_no;

		buf_list_ptr->_errno = errno;

		enqueue(file_ptr->_buf._r_a_list._queue,buf_list_ptr);

		buf_list_ptr = NULL;

	}

	file_ptr->_action->_action_status = ACTION_COMPLETE;
	atomic_thread_fence(memory_order_acq_rel);
	void* cb_data = sg_cb_data;
	atomic_thread_fence(memory_order_acq_rel);
	ef_notification_f_type cb_func = sg_cb_func;
	if (cb_func && (file_ptr->_buf._buffer == NULL)) cb_func(file_ptr->_ef_fd, oper_read, cb_data);

	return;
}

static void ef_file_read(void *data)
{
	//ma:
	int buf_was_empty = 0;
	struct ef_file_read_s * file_read_ptr = (struct ef_file_read_s *)data;
	EF_FILE *file_ptr = file_read_ptr->_file_ptr;
	off_t offset = file_read_ptr->_curr_page_no * sg_page_size;

	/* This is free of malloc done in ef_fire_file_read. */
	free(data);
	data = NULL;
 
	int ret = 0;
	file_ptr->_action->_action_status = ACTION_IN_PROGRESS;


	/* This buffer is freed in ef_read. */
	if (!file_ptr->_buf._buffer) {
		buf_was_empty = 1;
		{
			if (posix_memalign(&(file_ptr->_buf._buffer),sg_page_size,sg_page_size))
				EV_ABORT("Cannot allocate memory [%d] [%s]", errno, strerror(errno));
		}
	}
	//memset(file_ptr->_buf._buffer,0,sg_page_size);

	ret = pread(file_ptr->_fd,file_ptr->_buf._buffer,sg_page_size,offset);

	file_ptr->_action->_return_value = ret;
	file_ptr->_action->_err = errno;
	file_ptr->_action->_action_status = ACTION_COMPLETE;
	atomic_thread_fence(memory_order_acq_rel);
	void* cb_data = sg_cb_data;
	atomic_thread_fence(memory_order_acq_rel);
	ef_notification_f_type cb_func = sg_cb_func;
	if (cb_func && buf_was_empty) cb_func(file_ptr->_ef_fd, oper_read, cb_data);

	return;
}

static void ef_fire_file_read_ahead(EF_FILE * file_ptr)
{
	if ((file_ptr->_buf._buffer_index * sg_page_size) != file_ptr->_file_offset) {
		errno = EBADF;
		EV_ABORT("");
	}

	/* This module is designed for only one thread to operate
	 * on the file while in foreground, i.e. any one thread will have
	 * the custody of the file at a time, it will do operations
	 * and that is all. */
	atomic_thread_fence(memory_order_acquire);
	if (file_ptr->_action) {
		EV_ABORT("THIS CAN NEVER HAPPEN \n");
	}
	alloc_file_action(file_ptr,oper_readahead);

	if (file_ptr->_buf._r_a_started > file_ptr->_buf._r_a_completed) {
		EV_ABORT("fd = [%d] _r_a_started = [%d] Misfired\n",
				file_ptr->_fd,file_ptr->_buf._r_a_started);
	}
	file_ptr->_buf._r_a_started++;

	/* Freed in ef_file_read_ahead. */
	struct ef_file_read_s * file_read_ptr = malloc(sizeof(struct ef_file_read_s));
	file_read_ptr->_file_ptr = file_ptr;
	file_read_ptr->_curr_page_no = file_ptr->_buf._buffer_index;
	file_read_ptr->_which = 1;

	enqueue_task(sg_disk_io_thr_pool,ef_file_read_ahead,file_read_ptr);
	return;
}

static struct data_buf_s * get_read_ahead_buf(EF_FILE * file_ptr,long page_no)
{
	struct data_buf_s * buf_list_ptr = NULL;

	while (1) {
		buf_list_ptr = dequeue(file_ptr->_buf._r_a_list._queue);
		if (!buf_list_ptr) break;
		if (buf_list_ptr->_page_no == page_no) break;
		free(buf_list_ptr->_buf);
		free(buf_list_ptr);
		buf_list_ptr = NULL;
	}

	return buf_list_ptr;
}

static int ef_process_readahead_action_status(EF_FILE *file_ptr)
{
	if (file_ptr->_action && file_ptr->_action->_cmd == oper_readahead) {
		if (file_ptr->_action->_action_status == ACTION_COMPLETE) {
			free_file_action(file_ptr);
			file_ptr->_buf._r_a_completed++;
			return 1;
		}
		else {
			errno = EAGAIN;
			return -1;
		}
	}

	/* Cannot call this function either when there is no initiated action
	 * or if the initiated action is not oper_readahead.
	 * */
	errno = EINVAL;
	return -1;
}

static void ef_fire_file_read(EF_FILE * file_ptr)
{
	if ((file_ptr->_buf._buffer_index * sg_page_size) != file_ptr->_file_offset) {
		errno = EBADF;
		EV_ABORT("");
	}

	/* This module is designed for only one thread to operate
	 * on the file while in foreground, i.e. any one thread will have
	 * the custody of the file at a time, it will do operations
	 * and that is all. */
	atomic_thread_fence(memory_order_acquire);
	if (file_ptr->_action) {
		EV_ABORT("THIS CAN NEVER HAPPEN \n");
	}

	alloc_file_action(file_ptr,oper_read);

	struct ef_file_read_s * file_read_ptr = malloc(sizeof(struct ef_file_read_s));
	file_read_ptr->_file_ptr = file_ptr;
	file_read_ptr->_curr_page_no = file_ptr->_buf._buffer_index;

	enqueue_task(sg_disk_io_thr_pool,ef_file_read,file_read_ptr);
	return;
}

static int ef_process_read_action_status(EF_FILE *file_ptr)
{
	int ret = 0;
	/* Handle only error conditions here. */
	/* The dereference of a shared pointer is safe here, since
	 * this is the only thread, which can free the pointer.
	 * All others are background threads which only update the action. */
	atomic_thread_fence(memory_order_acquire);

	if (file_ptr->_action && file_ptr->_action->_cmd == oper_read) {
		if (file_ptr->_action->_action_status == ACTION_COMPLETE) {
			if (file_ptr->_action->_return_value == -1) {
				errno = file_ptr->_action->_err;
				free_file_action(file_ptr);
				atomic_thread_fence(memory_order_release);
				return -1;
			}
			else if (file_ptr->_action->_return_value == -100) {
				errno = EAGAIN;
				return -1;
			}
			else {
				if (file_ptr->_action->_return_value == 0) {
					// EOF has been reached.
					free_file_action(file_ptr);
					atomic_thread_fence(memory_order_release);
					return 0;
				}
				else {
					// Read has happened successfully
					ret = file_ptr->_action->_return_value;
					free_file_action(file_ptr);
					atomic_thread_fence(memory_order_release);
					/*
					 * Commented out read ahead feature.
					 * Since it was not working correctly under test conditions
					 * on 31-Jan-2020.
					 *
					 * This feature when enabled, is interefering with results of prior read.
					 * It requires to be redesigned and fixed.
					 * */
					 // ef_fire_file_read_ahead(file_ptr);
					return ret;
				}
			}
		}
		else {
			/* We dont know the status yet. */
			errno = EAGAIN;
			return -1;
		}
	}

	return 0;
}

static ssize_t chk_fd_read_conditions(int fd)
{
	errno = 0;
	int flg = 0;
	if (!sg_ef_init_done) {
		errno = EBADF;
		EV_ABORT("INIT NOT DONE");
	}
	if (!sg_open_files[fd]) {
		errno = EINVAL;
		return -1;
	}
	if (sg_open_files[fd]->_status != FILE_OPEN) {
		errno = EBADF;
		return -1;
	}
	if (sg_open_files[fd]->_file_offset >= sg_open_files[fd]->_curr_file_size_on_disk) {
		/* End of file has been reached. */
		return 0;
	}
	flg = sg_open_files[fd]->_oflag;
#ifdef __linux__
	if (flg & O_DIRECT) flg ^= O_DIRECT;
#endif
	if (!(flg&(O_RDWR)) && (flg != O_RDONLY)) {
		/* File opened without read access. */
		errno = EBADF;
		return -1;
	}

	return 1;
}

ssize_t chk_read_conditions(int fd, void * buf, size_t nbyte)
{
	errno = 0;
	int flg = 0;
	if (!sg_ef_init_done) {
		errno = EBADF;
		EV_ABORT("INIT NOT DONE");
	}
	if (!sg_open_files[fd]) {
		errno = EINVAL;
		return -1;
	}
	/* Only <= page_size (4k) of read supported. */
	if (NULL == buf || 0 >= nbyte || sg_page_size < nbyte) {
		errno = EINVAL;
		return -1;
	}
	if (sg_open_files[fd]->_status != FILE_OPEN) {
		errno = EBADF;
		return -1;
	}
	if (sg_open_files[fd]->_file_offset >= sg_open_files[fd]->_curr_file_size_on_disk) {
		/* End of file has been reached. */
		return 0;
	}
	flg = sg_open_files[fd]->_oflag;
#ifdef __linux__
	if (flg & O_DIRECT) flg ^= O_DIRECT;
#endif
	if (!(flg&(O_RDWR)) && (flg != O_RDONLY)) {
		/* File opened without read access. */
		errno = EBADF;
		return -1;
	}

	return 1;
}

static int ef_analyse_readahead_action_status(EF_FILE *file_ptr)
{
	if (file_ptr->_action && file_ptr->_action->_cmd == oper_readahead) {
		if (file_ptr->_action->_action_status == ACTION_COMPLETE) {
			//EV_DBGP("Here ret = %d\n", 1);
			return 1;
		}
		else {
			//EV_DBGP("Here ret = %d\n", -1);
			errno = EAGAIN;
			return -1;
		}
	}

	/* Cannot call this function either when there is no initiated action
	 * or if the initiated action is not oper_readahead.
	 * */
	errno = EINVAL;
	//EV_DBGP("Here ret = %d\n", -1);
	return -1;
}

static int ef_analyse_read_action_status(EF_FILE *file_ptr)
{
	int ret = 0;
	atomic_thread_fence(memory_order_acquire);

	if (file_ptr->_action && file_ptr->_action->_cmd == oper_read) {
		errno = 0;
		if (file_ptr->_action->_action_status == ACTION_COMPLETE) {
			if (file_ptr->_action->_return_value == -1) {
				errno = file_ptr->_action->_err;
				//EV_DBGP("Here ret = %d\n", -1);
				return -1;
			}
			else if (file_ptr->_action->_return_value == -100) {
				errno = EAGAIN;
				//EV_DBGP("Here ret = %d\n", -1);
				return -1;
			}
			else {
				if (file_ptr->_action->_return_value == 0) {
					// EOF has been reached.
					//EV_DBGP("Here ret = %d\n", 0);
					return 0;
				}
				else {
					// Read has happened successfully
					ret = file_ptr->_action->_return_value;
					//EV_DBGP("Here ret = %d\n", ret);
					return ret;
				}
			}
		}
		else {
			/* We dont know the status yet. */
			errno = EAGAIN;
			//EV_DBGP("Here ret = %d\n", -1);
			return -1;
		}
	}

	//EV_DBGP("Here ret = %d\n", 0);
	return 0;
}

/*
 * Checks the status of the file for read operation.
 * Following are the return values.
 *
 * 0 : EOF has been reached
 * -1: Status to be determined based on errno
 *  	EAGAIN    : Read action has been triggered and is still in progress
 *  	any other : fd in an erroneous state and read cannot happen.
 * 1 : Data Available for read in the file buffer, and read action will succeed
 */
ssize_t ef_file_ready_for_read(int fd)
{
	ssize_t		ret = -1;
	off_t		page_offset = 0;
	EF_FILE		*file_ptr = NULL;

	/* EOF could have been reached by the read initiated just before this.
	 * Hence cannot return as error condition.
	 * */
	if ((ret = chk_fd_read_conditions(fd)) < 0) {
		return ret;
	}

	ret = -1;
	file_ptr = sg_open_files[fd];
	page_offset = file_ptr->_file_offset - (file_ptr->_buf._buffer_index * sg_page_size);

	if (file_ptr->_action) {
		if (file_ptr->_action->_cmd == oper_readahead) {
			ret = ef_analyse_readahead_action_status(file_ptr);
			//EV_DBGP("Here ret = %zd\n", ret);
			if (ret < 0) return ret;
		}
		else if (file_ptr->_action->_cmd == oper_read) {
			ret = ef_analyse_read_action_status(file_ptr);
			//EV_DBGP("Here ret = %zd\n", ret);
			if (ret <= 0) return ret;
		}
		else {
			errno = EBUSY;
			ret = -1;
			return ret;
		}
	}
	else {
		/*
		 * If no action (therefore no read action) has been triggered
		 * and if the read buffer is null,
		 * file is not read ready.
		 * */
		errno = EINVAL;
		ret = -1;
	}

	return ret;
}

static ssize_t low_ef_read(int fd, void * buf, size_t nbyte)
{
	ssize_t		ret = 0;
	off_t		page_offset = 0;
	size_t		transfer_size = 0;
	EF_FILE		*file_ptr = NULL;

	if ((ret = chk_read_conditions(fd,buf,nbyte)) <= 0) {
		//EV_DBGP("Here fd = [%d]\n",fd);
		return ret;
	}
	ret = 0;

	//EV_DBGP("Here\n");

	file_ptr = sg_open_files[fd];
	page_offset = file_ptr->_file_offset - (file_ptr->_buf._buffer_index * sg_page_size);

	if (file_ptr->_action) {
		if (file_ptr->_action->_cmd == oper_readahead) {
			ret = ef_process_readahead_action_status(file_ptr);
			if (ret < 0) return ret;
		}
		else if (file_ptr->_action->_cmd == oper_read) {
			ret = ef_process_read_action_status(file_ptr);
			if (ret <= 0) return ret;
		}
		else if (file_ptr->_action->_cmd == oper_write) {
			//low_ef_sync(file_ptr);
			sync_file_writes(fd);
			SET_WRITE_ACTION_STATUS(file_ptr->_action);
			if (file_ptr->_action->_action_status != ACTION_COMPLETE) {
				errno = EBUSY;
				ret = -1;
				return ret;
			}
		}
		else {
			if (file_ptr->_action->_action_status != ACTION_COMPLETE) {
				errno = EBUSY;
				ret = -1;
				return ret;
			}
		}
	}

	if (file_ptr->_action && file_ptr->_action->_action_status == ACTION_COMPLETE) {
		/* Unchecked _action status is lying from the previous action. */
		free_file_action(file_ptr);
	}
	ret = 0;

	if (file_ptr->_buf._buffer == NULL) {
		/* Fire a fresh read. */
		atomic_thread_fence(memory_order_acquire);
		if (file_ptr->_action) {
			/* If thre is another pending action
			 * Read has to be fired once the pending
			 * action is over. */
			errno = EAGAIN;
			ret = -1;
		}
		else  {
			if (file_ptr->_buf._r_a_started) {
				if (file_ptr->_buf._r_a_completed == file_ptr->_buf._r_a_started) {
					struct data_buf_s * buf_list_ptr = get_read_ahead_buf(file_ptr,file_ptr->_buf._buffer_index);
					if (buf_list_ptr) {
						if (buf_list_ptr->_nbyte <= 0) {
							ssize_t ret1 = buf_list_ptr->_nbyte;
							errno = buf_list_ptr->_errno;
							free(buf_list_ptr->_buf);
							free(buf_list_ptr);
							buf_list_ptr = NULL;
							ret = ret1;
						}
						else {
							ssize_t ret1 = buf_list_ptr->_nbyte;
							if (file_ptr->_buf._buffer) {
								free(file_ptr->_buf._buffer);
								file_ptr->_buf._buffer = NULL;
							}
							file_ptr->_buf._buffer = buf_list_ptr->_buf;
							free(buf_list_ptr);
							buf_list_ptr = NULL;
							ef_fire_file_read_ahead(file_ptr);
							ret = ret1;
						}
					}
					else {
						ef_fire_file_read(file_ptr);
						errno = EAGAIN;
						ret = -1;
					}
				}
			}
			else {
				ef_fire_file_read(file_ptr);
			}
			errno = EAGAIN;
			ret = -1;
		}
	}
	if (ret <0) return ret;
	ret = 0;

	if (file_ptr->_buf._buffer != NULL) {
		if ((sg_page_size - page_offset) > 0) {
			transfer_size = (nbyte <= (sg_page_size - page_offset))?nbyte:(sg_page_size - page_offset);
			if ((file_ptr->_file_offset+transfer_size)>file_ptr->_curr_file_size_on_disk) {
				transfer_size = transfer_size - ((file_ptr->_file_offset+transfer_size) - file_ptr->_curr_file_size_on_disk);
			}
			memcpy(buf,file_ptr->_buf._buffer+page_offset,transfer_size);
			page_offset += transfer_size;
			file_ptr->_file_offset += transfer_size;
			ret = transfer_size;
			if (page_offset == sg_page_size) {
				/* Enable firing a fresh read. */
				page_offset = 0;
				file_ptr->_buf._buffer_index++;
				free(file_ptr->_buf._buffer);
				file_ptr->_buf._buffer = NULL;
			}
			else if (page_offset > sg_page_size) {
				EV_ABORT("Should have never reached here\n");fflush(stdout);
			}
		}
		else {
			errno = EBADF;
			return -1;
		}
	}

	return ret;
}

static void ef_file_fsync(EF_FILE * file_ptr)
{
	off_t		page_offset = 0;
	//file_ptr->in_sync = 1;
	sync_buf_to_file(file_ptr);
	file_ptr->_curr_file_size_on_disk = file_ptr->_running_file_size;
	page_offset = file_ptr->_file_offset - (file_ptr->_buf._buffer_index * sg_page_size);
	if (page_offset == sg_page_size) {
		if (file_ptr->_buf._buffer) free(file_ptr->_buf._buffer);
		file_ptr->_buf._buffer = NULL;
		file_ptr->_buf._buffer_index++;
	}
#ifdef __linux__
	else if (O_APPEND&(file_ptr->_oflag)) {
		int flg = fcntl(file_ptr->_fd,F_GETFL);
		if (O_DIRECT&flg) flg ^= O_DIRECT;
		fcntl(file_ptr->_fd,F_SETFL,flg);
		file_ptr->_block_write_unaligned = 1;
	}
#endif
	//file_ptr->in_sync = 0;
	return ;
}

static void ef_file_write(void * data)
{
	EF_FILE * file_ptr = (EF_FILE *)data;
	struct write_task_s * w_list = file_ptr->_action->_inp._write_inp._b_w_list;
	struct write_task_s * w_t = NULL;
	int bytes_transfered = 0;

	file_ptr->_action->_action_status = ACTION_IN_PROGRESS;

	w_t = w_list;
	while(w_t) {
		errno = 0;
		if (w_t->_cmd == file_write_evt) bytes_transfered += ef_one_single_write(file_ptr,w_t);
		else if (w_t->_cmd == file_fsync_evt)  ef_file_fsync(file_ptr);
		w_t = w_t->_next;
	}

	file_ptr->_action->_return_value += bytes_transfered;
	file_ptr->_action->_err = errno;

	{
		struct file_writer_task_s * fwt = malloc(sizeof(struct file_writer_task_s));
		fwt->_cmd = fw_write_cmpl_evt;
		fwt->_file_ptr = file_ptr;
		enqueue(sg_file_writer_queue,fwt);
		//pthread_kill(sg_file_writer_tid,SIGINT);
		pthread_mutex_lock(&sg_file_writer_mutex);
		pthread_cond_signal(&sg_file_writer_cond);
		pthread_mutex_unlock(&sg_file_writer_mutex);
	}

	return;
}

static void free_file_action(EF_FILE * file_ptr)
{
	{
		int pam = 0;
		while (!atomic_compare_exchange_strong(&(file_ptr->_prevent_action_mutation),&pam,1));
	}

	EF_FILE_ACTION_CLEANUP(file_ptr);

	{
		int pam = 1;
		atomic_compare_exchange_strong(&(file_ptr->_prevent_action_mutation),&pam,0);
	}
	return;
}

static void alloc_file_action(EF_FILE * file_ptr, file_oper_type oper)
{
	{
		int pam = 0;
		while (!atomic_compare_exchange_strong(&(file_ptr->_prevent_action_mutation),&pam,1));
	}

	eio_cmd_init(file_ptr,oper);

	{
		int pam = 1;
		atomic_compare_exchange_strong(&(file_ptr->_prevent_action_mutation),&pam,0);
	}
	return;
}

/* This function will ensure that only one thread will run this at a time.
 * If the subsequent thread, executes this, it and the work is already done
 * there will be no harm. */
static void sync_file_writes(int fd)
{
	EF_FILE * file_ptr = NULL;
	{
		int htfa = 0;
		while (!atomic_compare_exchange_strong(&(sg_handle_to_file_action[fd]),&htfa,1)) htfa = 0;
	}
	file_ptr = sg_open_files[fd];
	if (!file_ptr) goto SYNC_FILE_WRITES_FINALLY;
	if (file_ptr->_status != FILE_OPEN) goto SYNC_FILE_WRITES_FINALLY;
	if (!(file_ptr->_oflag&(O_RDWR|O_WRONLY))) goto SYNC_FILE_WRITES_FINALLY;

	{
		int pam = 0;
		while (!atomic_compare_exchange_strong(&(file_ptr->_prevent_action_mutation),&pam,1)) pam = 0;
	}
	{
		int ws = 0;
		while (!atomic_compare_exchange_strong(&(file_ptr->_write_suspended),&ws,1)) ws = 0;
		/* At this point ef_write will block. */
	}

	/* At this point, no other action will get created or existing action will not be freed. */
	/* If it is not oper_write, it is required to wait for that action to complete. */
	if (file_ptr->_action && file_ptr->_action->_cmd != oper_write) {
		atomic_thread_fence(memory_order_acquire);
		while (file_ptr->_action->_action_status != ACTION_COMPLETE) {
			EV_YIELD();
			atomic_thread_fence(memory_order_acquire);
		}
	}

	low_ef_sync(file_ptr);
	if (file_ptr->_action && file_ptr->_action->_cmd == oper_write &&
						file_ptr->_action->_action_status != ACTION_COMPLETE) {
		struct write_task_s * w_t = malloc(sizeof(struct write_task_s));

		w_t->_buf = file_ptr->_action->_inp._write_inp._w_accum._data;;
		w_t->_nbyte = file_ptr->_action->_inp._write_inp._w_accum._bytes;
		w_t->_next = NULL;
		w_t->_cmd = file_fsync_evt;

		file_ptr->_action->_inp._write_inp._write_totreq_count++;
		enqueue(file_ptr->_action->_inp._write_inp._w_queue, w_t);

		low_eio_cmd_init(file_ptr->_action,oper_write);
		/* Free for the malloc below happens in the main event loop of file_writer. */
		struct file_writer_task_s * fwt = malloc(sizeof(struct file_writer_task_s));
		fwt->_cmd = fw_write_req_evt;
		fwt->_file_ptr = file_ptr;
		enqueue(sg_file_writer_queue,fwt);
		//pthread_kill(sg_file_writer_tid,SIGINT);
		pthread_mutex_lock(&sg_file_writer_mutex);
		pthread_cond_signal(&sg_file_writer_cond);
		pthread_mutex_unlock(&sg_file_writer_mutex);
	}

	{
		int ws = 1;
		atomic_compare_exchange_strong(&(file_ptr->_write_suspended),&ws,0);
	}
	{
		int pam = 1;
		atomic_compare_exchange_strong(&(file_ptr->_prevent_action_mutation),&pam,0);
	}

SYNC_FILE_WRITES_FINALLY:
	{
		int htfa = 1;
		atomic_compare_exchange_strong(&(sg_handle_to_file_action[fd]),&htfa,0);
	}
	return;
}

static void slow_sync(void * data)
{
	struct slow_sync_task_s *fst = NULL;
	while (1) {
		usleep(1000000);
		fst = dequeue(sg_slow_sync_queue);
		if (fst && fst->_cmd == fs_shutdown_evt) break;
		for (int i = 0; i < EF_MAX_FILES ; i++) {
			if (sg_open_files[i]) sync_file_writes(i);
		}
	}
	return;
}

static void file_writer(void * data)
{
	file_writer_cmd_type  cmd = fw_invalid_evt;
	EF_FILE * file_ptr = NULL;
	struct write_task_s * w_t = NULL;
	struct write_task_s * ptr = NULL;
	int shutdown = 0;
	sigset_t				set;
	sigset_t				r_set;
	sigset_t				o_set;
	int						sig = 0;
	long compl_count = 0L;

	sg_file_writer_tid = pthread_self();

	struct file_writer_task_s * fwt = NULL;

	sigemptyset(&set);
	sigaddset(&set,SIGINT);
	pthread_sigmask(SIG_BLOCK, &set, &o_set);

	while (1) {
		fwt = dequeue(sg_file_writer_queue);
		while (!fwt) {
			pthread_mutex_lock(&sg_file_writer_mutex);
			fwt = dequeue(sg_file_writer_queue);
			if (fwt) {
				pthread_mutex_unlock(&sg_file_writer_mutex);
				break;
			}
			pthread_cond_wait(&sg_file_writer_cond, &sg_file_writer_mutex);
			pthread_mutex_unlock(&sg_file_writer_mutex);
			fwt = dequeue(sg_file_writer_queue);
		}
		cmd = fwt->_cmd;
		file_ptr = fwt->_file_ptr;;
		free(fwt);
		fwt = NULL;
		switch (cmd) {
			case fw_write_cmpl_evt:
				{
					/* Have to free the list here. */
					compl_count = 0L;
					w_t = file_ptr->_action->_inp._write_inp._b_w_list;
					while (w_t) {
						/* Moved this increment to the end of the case condition.
						 * This is to avoid any race condition that can occur with
						 * another thread waiting for _write_cmpl_count to become
						 * equal to the total req_count and then go and clear 
						 * the _action memory. */

						compl_count++;
						ptr = w_t;
						w_t = w_t->_next;
						free(ptr->_buf);
						free(ptr);
						ptr = NULL;
					}
					file_ptr->_action->_inp._write_inp._b_w_list = NULL;
				}
				{
					w_t = dequeue(file_ptr->_action->_inp._write_inp._w_queue);
					if (w_t) {
						w_t->_next = NULL;
						file_ptr->_action->_inp._write_inp._b_w_list = w_t;
						file_ptr->_action->_inp._write_inp._write_req_count++;
						ptr = dequeue(file_ptr->_action->_inp._write_inp._w_queue);
						while (ptr) {
							file_ptr->_action->_inp._write_inp._write_req_count++;
							ptr->_next = NULL;
							w_t->_next = ptr;
							w_t = ptr;
							ptr = dequeue(file_ptr->_action->_inp._write_inp._w_queue);
						}
						enqueue_task(sg_disk_io_thr_pool,ef_file_write,file_ptr);
					}
					w_t = NULL;
				}
				file_ptr->_action->_inp._write_inp._write_cmpl_count += compl_count;
				atomic_thread_fence(memory_order_acq_rel);
				break;
			case fw_write_req_evt:
				if (file_ptr->_action->_inp._write_inp._write_req_count !=
											file_ptr->_action->_inp._write_inp._write_cmpl_count) {

					break;
				}
				{
					w_t = dequeue(file_ptr->_action->_inp._write_inp._w_queue);
					if (w_t) {
						w_t->_next = NULL;
						file_ptr->_action->_inp._write_inp._b_w_list = w_t;
						file_ptr->_action->_inp._write_inp._write_req_count++;
						ptr = dequeue(file_ptr->_action->_inp._write_inp._w_queue);
						while (ptr) {
							file_ptr->_action->_inp._write_inp._write_req_count++;
							ptr->_next = NULL;
							w_t->_next = ptr;
							w_t = ptr;
							ptr = dequeue(file_ptr->_action->_inp._write_inp._w_queue);
						}
						enqueue_task(sg_disk_io_thr_pool,ef_file_write,file_ptr);
					}
					w_t = NULL;
				}
				break;
			case fw_shutdown_evt:
			default:
				shutdown = 1;
				break;
		}
		if (shutdown) break;
	}

	return;
}

static void sync_buf_from_file(EF_FILE *file_ptr)
{
	struct data_buf_s * buf_list_ptr = NULL;

	if (!file_ptr->_buf._buffer) {
		if (posix_memalign(&(file_ptr->_buf._buffer),sg_page_size,sg_page_size)) EV_ABORT("Cannot allocate memory");
	}
	//memset(file_ptr->_buf._buffer,0,sg_page_size);
	file_ptr->_buf._is_dirty = 0;
	if ((file_ptr->_buf._buffer_index * sg_page_size) < file_ptr->_curr_file_size_on_disk) {
		buf_list_ptr = NULL;
		if (file_ptr->_buf._r_a_started)
			buf_list_ptr = get_read_ahead_buf(file_ptr,(file_ptr->_buf._buffer_index));
		if (buf_list_ptr) {
			free(file_ptr->_buf._buffer);
			file_ptr->_buf._buffer = NULL;
			file_ptr->_buf._buffer = buf_list_ptr->_buf;
			buf_list_ptr->_buf = NULL;
			free(buf_list_ptr);
			buf_list_ptr = NULL;
			//file_ptr->_block_write_unaligned = 0;
		}
		else if  ((O_RDONLY == file_ptr->_oflag) || (O_RDWR&file_ptr->_oflag)) {
			errno = 0;
			//EV_DBGP("fd = [%d]\n",file_ptr->_fd);
			//EV_DBGP("Buf ptr = [%p]\n",file_ptr->_buf._buffer);
			//EV_DBGP("Page size = [%zu]\n",sg_page_size);
			//EV_DBGP("Offset = [%zu]\n",(file_ptr->_buf._buffer_index * sg_page_size));
			if (-1 == pread(file_ptr->_fd,file_ptr->_buf._buffer,sg_page_size,
									(file_ptr->_buf._buffer_index * sg_page_size))) {
				EV_ABORT("Cannot handle this error\n");

			}
			//file_ptr->_block_write_unaligned = 0;
		}
	}
}

static void sync_buf_to_file(EF_FILE *file_ptr)
{
	if (file_ptr->_buf._is_dirty) {
		/*
		EV_DBGP("Writing %llu bytes to disk\n",
				file_ptr->_running_file_size - (file_ptr->_buf._buffer_index * sg_page_size));
		*/
		errno = 0;
		if (-1 == pwrite(file_ptr->_fd,file_ptr->_buf._buffer,
					sg_page_size,(file_ptr->_buf._buffer_index * sg_page_size))) {
			//EV_DBGP("is=%d,iw=%d,fd = %d, index = %d, Buf ptr = [%ld]\n",file_ptr->in_sync,file_ptr->in_write,file_ptr->_fd,file_ptr->_buf._buffer_index,(long)(file_ptr->_buf._buffer));
			//EV_DBGP("fd = [%d]\n",file_ptr->_fd);
			//EV_DBGP("fd = [%d]\n",file_ptr->_fd);
			//EV_DBGP("Buf ptr = [%ld]\n",(long)(file_ptr->_buf._buffer));
			//EV_DBGP("Page size = [%zu]\n",sg_page_size);
			//EV_DBGP("Offset = [%zu]\n",(file_ptr->_buf._buffer_index * sg_page_size));
			//EV_ABORT("Cannot handle this error\n");fflush(stdout);
		}
		//EV_DBGP("is=%d,iw=%d,fd = %d, index = %d, Buf ptr = [%ld]\n",file_ptr->in_sync,file_ptr->in_write,file_ptr->_fd,file_ptr->_buf._buffer_index,(long)(file_ptr->_buf._buffer));
		//fcntl(file_ptr->_fd,F_FULLFSYNC);
		if (ftruncate(file_ptr->_fd,file_ptr->_running_file_size)) EV_ABORT("ftruncate call failed");
		file_ptr->_buf._is_dirty = 0;
	}
	return;
}

static int ef_one_single_write(EF_FILE *file_ptr, struct write_task_s * w_t)
{
	int bytes_transfered = 0;
	int transfer_size = 0;
	int remaining_bytes = 0;
	off_t		page_offset = 0;
	size_t		wb_pos = 0;

	/* If file is opened with O_APPEND, writes have to be done at the end of the file. */
	if (file_ptr->_oflag&O_APPEND) {
		/* If the file position is not at the end. */
		if (!(file_ptr->_buf._is_dirty)) {
			int old_buffer_index = file_ptr->_buf._buffer_index;
			file_ptr->_buf._buffer_index = 0;
			file_ptr->_file_offset = lseek(file_ptr->_fd,0,SEEK_END);
			file_ptr->_buf._buffer_index = (file_ptr->_file_offset / sg_page_size);
			if (old_buffer_index != file_ptr->_buf._buffer_index) {
				free(file_ptr->_buf._buffer);
				file_ptr->_buf._buffer = NULL;
			}
		}
		else {
			if (file_ptr->_buf._buffer_index != (file_ptr->_curr_file_size_on_disk / sg_page_size)) {
				EV_ABORT("This situation must not arise \n");fflush(stdout);
			}
		}
	}

	/* We have to update file_ptr->_curr_file_size_on_disk and file_ptr->_file_offset */
	page_offset = file_ptr->_file_offset - (file_ptr->_buf._buffer_index * sg_page_size);
	remaining_bytes = w_t->_nbyte;
	while (remaining_bytes) {

		if (remaining_bytes<0) {
			EV_ABORT("should have never reached here\n");fflush(stdout);
		}

		if (!(file_ptr->_buf._buffer) && !(file_ptr->_buf._is_dirty)) sync_buf_from_file(file_ptr);

		transfer_size = (remaining_bytes <= (sg_page_size - page_offset))?remaining_bytes:(sg_page_size - page_offset);

		if (file_ptr->_block_write_unaligned) {
			if (!(file_ptr->_oflag&(O_APPEND|O_WRONLY))) {
				EV_ABORT("File not opened in Write only and Append mode, no need for unaligned write\n");
			}

			errno = 0;
			if (-1 == pwrite(file_ptr->_fd, w_t->_buf, transfer_size, file_ptr->_curr_file_size_on_disk)) {
				EV_ABORT("Cannot handle this error\n");fflush(stdout);
			}
			//fcntl(file_ptr->_fd,F_FULLFSYNC);
			page_offset += transfer_size;
			file_ptr->_file_offset += transfer_size;
			bytes_transfered += transfer_size;
			remaining_bytes -= transfer_size;
			wb_pos += transfer_size;
			file_ptr->_curr_file_size_on_disk += transfer_size;
			file_ptr->_running_file_size += transfer_size;
			if (page_offset == sg_page_size) {
#ifdef __linux__
				{
					int flg = fcntl(file_ptr->_fd,F_GETFL);
					flg |= O_DIRECT;
					fcntl(file_ptr->_fd,F_SETFL,flg);
				}
#endif
				file_ptr->_block_write_unaligned = 0;
				page_offset = 0;
				file_ptr->_buf._is_dirty = 0;
				file_ptr->_buf._buffer_index++;
				sync_buf_from_file(file_ptr);
			}
		}
		else {
			file_ptr->_buf._is_dirty = 1;
			memcpy(file_ptr->_buf._buffer+page_offset,(w_t->_buf+wb_pos),transfer_size);
			page_offset += transfer_size;
			bytes_transfered += transfer_size;
			remaining_bytes -= transfer_size;
			wb_pos += transfer_size;
			/* Total size (_running_file_size) increases when there are writes in the last page.
			 * And byeond the current offset. */
			if ((file_ptr->_file_offset/sg_page_size) == (file_ptr->_running_file_size/sg_page_size)) {
				int size_increase = 0;
				if ((file_ptr->_file_offset + transfer_size) > file_ptr->_running_file_size)
					size_increase = file_ptr->_file_offset + transfer_size - file_ptr->_running_file_size;
				file_ptr->_running_file_size += size_increase;
			}
			file_ptr->_file_offset += transfer_size;
			if (page_offset == sg_page_size) {
				/* Transfer content to disk . */
				//file_ptr->in_write = 1;
				sync_buf_to_file(file_ptr);
				//file_ptr->in_write = 0;
				file_ptr->_buf._buffer_index++;
				if ((file_ptr->_buf._buffer_index * sg_page_size) > file_ptr->_curr_file_size_on_disk) {
					file_ptr->_curr_file_size_on_disk = (file_ptr->_buf._buffer_index * sg_page_size);
					//The below line is redundant.
					//file_ptr->_running_file_size = (file_ptr->_buf._buffer_index * sg_page_size);
				}
				//memset(file_ptr->_buf._buffer,0,sg_page_size);
				file_ptr->_buf._is_dirty = 0;
				page_offset = 0;
				sync_buf_from_file(file_ptr);
			}
			else if (page_offset > sg_page_size) {
				EV_ABORT("Should have never reached here\n");fflush(stdout);
			}
		}
	}

	return bytes_transfered;
}

static void print_wt(void * data)
{
	struct write_task_s * w_t = data;
	EV_DBGP ("enqueued w t = [%zu]\n",w_t->_nbyte);fflush(stdout);
	return;
}

static void write_buffer_sync(EF_FILE * file_ptr)
{
	/* Free for the malloc below, happens in the completion event
	 * of the file_writer thread. */
	struct write_task_s * w_t = malloc(sizeof(struct write_task_s));

	w_t->_buf = file_ptr->_action->_inp._write_inp._w_accum._data;;
	w_t->_nbyte = file_ptr->_action->_inp._write_inp._w_accum._bytes;
	w_t->_next = NULL;
	w_t->_cmd = file_write_evt;

	file_ptr->_action->_inp._write_inp._w_accum._data = NULL;
	file_ptr->_action->_inp._write_inp._w_accum._first_write = 0;
	file_ptr->_action->_inp._write_inp._w_accum._bytes = 0;

	file_ptr->_action->_inp._write_inp._write_totreq_count++;
	enqueue(file_ptr->_action->_inp._write_inp._w_queue, w_t);

	low_eio_cmd_init(file_ptr->_action,oper_write);
	/* Free for the malloc below happens in the main event loop of file_writer. */
	struct file_writer_task_s * fwt = malloc(sizeof(struct file_writer_task_s));
	fwt->_cmd = fw_write_req_evt;
	fwt->_file_ptr = file_ptr;
	enqueue(sg_file_writer_queue,fwt);
	pthread_mutex_lock(&sg_file_writer_mutex);
	pthread_cond_signal(&sg_file_writer_cond);
	pthread_mutex_unlock(&sg_file_writer_mutex);
	/* Wake up file writer thread, which is running file_writer. */
	//pthread_kill(sg_file_writer_tid,SIGINT);
	return;
}

static ssize_t low_ef_write(EF_FILE * file_ptr, void * buf, size_t nbyte)
{
	int bytes_transfered = 0;
	errno = 0;

	/* Prevent write from getting suspended. */
	/* From this point below, return should happen only at the end of the function. */
	{
		int ws = 0;
		while (!atomic_compare_exchange_strong(&(file_ptr->_write_suspended),&ws,-1)) ws = 0;
	}

	if (file_ptr->_action && file_ptr->_action->_action_status == ACTION_COMPLETE) {
		/* Write has been called before checking for the return value of
		 * the previous action. */
		free_file_action(file_ptr);
	}
	else if (file_ptr->_action && file_ptr->_action->_cmd != oper_write) {
		/* If any other action is initiated, the caller has to wait till that action completes
		 * before initiating write. However it is not necessary to wait for the previous write
		 * to complete. */
		errno = EBUSY;
		bytes_transfered = -1;
		goto LOW_EF_WRITE_FINALLY;
	}

	if (!(file_ptr->_action)) {
		/* This function will be called by only one thread.
		 * Hence no protection against dereference. */
		alloc_file_action(file_ptr,oper_write);
	}

	/* Accumulate the data to be written. */
	{
		size_t remaining_bytes = 0;
		bytes_transfered = 0;
		remaining_bytes = nbyte;
		while (remaining_bytes > 0) {
			/* Free of malloc below, happens in the completion event of file writer thread. */
			if (!file_ptr->_action->_inp._write_inp._w_accum._data) {
				file_ptr->_action->_inp._write_inp._w_accum._data = malloc(sg_page_size);
				memset(file_ptr->_action->_inp._write_inp._w_accum._data,0,sg_page_size);
			}


			size_t space_in_the_page = sg_page_size - file_ptr->_action->_inp._write_inp._w_accum._bytes;
			size_t to_be_copied = (remaining_bytes > space_in_the_page)?space_in_the_page:remaining_bytes;
			memcpy((file_ptr->_action->_inp._write_inp._w_accum._data+
					file_ptr->_action->_inp._write_inp._w_accum._bytes),
					buf+bytes_transfered, to_be_copied);
			file_ptr->_action->_inp._write_inp._w_accum._bytes += to_be_copied;
			bytes_transfered += to_be_copied;
			remaining_bytes = remaining_bytes - to_be_copied;

			/* NB: Here we can return the status of the previous write.
			 * This can be a future enhancement. */

			if (file_ptr->_action->_inp._write_inp._w_accum._bytes >= sg_page_size) {
				write_buffer_sync(file_ptr);
			}
		}
	}

LOW_EF_WRITE_FINALLY:

	/* Allow write suspension after this. */
	{
		int ws = -1;
		if (!atomic_compare_exchange_strong(&(file_ptr->_write_suspended),&ws,0))
			EV_ABORT("This is an impossible condition");
	}

	return bytes_transfered;

}

static void low_ef_sync(EF_FILE * file_ptr)
{
	if (file_ptr->_action && file_ptr->_action->_cmd == oper_write && file_ptr->_action->_inp._write_inp._w_accum._bytes) {
		write_buffer_sync(file_ptr);
	}
	return;
}

int ef_poll(int fd)
{
	int ret = 0;
	EF_FILE * file_ptr = NULL;
	if (!sg_ef_init_done) {
		errno = EBADF;
		EV_ABORT("INIT NOT DONE");
	}
	if (!sg_open_files[fd]) {
		errno = EBADF;
		return -1;
	}
	if (sg_open_files[fd]->_status != FILE_OPEN) {
		errno = EBADF;
		return -1;
	}

	file_ptr = sg_open_files[fd];
	if ((O_WRONLY|O_RDWR)&file_ptr->_oflag)
		ret = ret|1;

	if (((file_ptr->_oflag == O_RDONLY) || (O_RDWR&file_ptr->_oflag)) && (file_ptr->_buf._buffer))
		ret = ret|2;

	return ret;
}

int ef_close(int fd)
{
	errno = 0;
	return ef_close_api(fd,false);;
}

/* API of close has to be cleaned up. */
int ef_close_immediate(int fd)
{
	errno = 0;
	return ef_close_api(fd,true);
}

/* Request to open the file */
/* There is a global array of file handles and an index which indicates last used index
 * for opening. The logic in this function is to use a slot that is till now unused.
 * Start from the place of global index + 1 and search until an unused slot is found.
 * Search only EF_MAX_FILES number of times, if not successful within that many attemts return
 * with error. */
int ef_open(const char * path, int oflag, ...)
{
	int fd = 0;
	EF_FILE * file_ptr = NULL;
	mode_t	mode =0;

	errno = 0;
	if (!sg_ef_init_done) {
		errno = EBADF;
		EV_ABORT("INIT NOT DONE");
	}
	if (path == NULL || *path == '\0') {
		/* operation not supported, invalid input */
		errno = EINVAL;
		return -1;
	}
	if (O_APPEND == oflag) {
		/* O_APPEND should be accompanied with either, O_RDONLY
		 * OWRONLY or O_RDWR. */
		errno = EINVAL;
		return -1;
	}

	if ((O_RDONLY != oflag) && !((O_RDWR|O_APPEND|O_CREAT)&oflag)) {
		/* operation not supported, invalid input */
		errno = EINVAL;
		return -1;
	}

	fd = st_dequeue_sg_fd_queue();
	//fd = (int)(long)dequeue(sg_fd_queue);
	if (-1 == fd) {
		/* Too many open files */
		errno = EMFILE;
		return -1;
	}

	{
		int htfa = 0;
		while (!atomic_compare_exchange_strong(&(sg_handle_to_file_action[fd]),&htfa,1)) htfa = 0;
	}
	/* Free for this malloc is in ef_close_status.
	 * ef_close_status is called automatically in ef_close_immediate.
	 * If async ef_close is called, the calling is not automatic. */
	sg_open_files[fd] = malloc(sizeof(EF_FILE));
	{
		int htfa = 1;
		atomic_compare_exchange_strong(&(sg_handle_to_file_action[fd]),&htfa,0);
	}
	file_ptr = (EF_FILE *)(sg_open_files[fd]);
	EF_FILE_INIT(file_ptr);

	/* Submit Request to open the file */
	file_ptr->_ef_fd = fd;
	alloc_file_action(file_ptr,oper_open);
	file_ptr->_action->_inp._open_inp._path = strdup(path);
	file_ptr->_action->_inp._open_inp._oflag = oflag;
	file_ptr->_status = FILE_OPEN_ATTEMPTED;
	if (O_CREAT&oflag) {
		va_list ap;
		va_start(ap,oflag);
		mode= (mode_t)va_arg(ap,int);
		file_ptr->_action->_inp._open_inp._mode = mode;
		va_end(ap);
	}

	enqueue_task(sg_disk_io_thr_pool,ef_file_open,file_ptr);

	/* Return the handle to file. */
	return fd;
}

int ef_file_state(int fd)
{
	return ((sg_open_files[fd])?sg_open_files[fd]->_status:FILE_NOT_OPEN);
}

int ef_open_status(int fd)
{
	int status = 0;
	errno = 0;
	if (!sg_open_files[fd]) {
		errno = EINVAL;
		return -1;
	}
	if (sg_open_files[fd]->_status == FILE_OPEN_ATTEMPTED) {
		errno = EAGAIN;
		return -1;
	}
	if (sg_open_files[fd]->_action && sg_open_files[fd]->_action->_cmd == oper_open) {
		if (sg_open_files[fd]->_action->_action_status == ACTION_COMPLETE) {
			if (sg_open_files[fd]->_action->_return_value == -1) {
				errno = sg_open_files[fd]->_action->_err;

				{
					int htfa = 0;
					while (!atomic_compare_exchange_strong(&(sg_handle_to_file_action[fd]),&htfa,1)) htfa = 0;
				}
				EF_FILE_CLEANUP(sg_open_files[fd]);
				{
					int htfa = 1;
					atomic_compare_exchange_strong(&(sg_handle_to_file_action[fd]),&htfa,0);
				}
				st_enqueue_sg_fd_queue(fd);
				//enqueue(sg_fd_queue,(void*)(long)fd);

				//errno = EBADF;
				return -1;
			}
			else if (sg_open_files[fd]->_action->_return_value == -100) {
				errno = EAGAIN;
				return -1;
			}
			else {
				free_file_action(sg_open_files[fd]);
				atomic_thread_fence(memory_order_release);
				//return sg_open_files[fd]->_fd;
				return fd;
			}
		}
		else {
			errno = EAGAIN;
			return -1;
		}
	}
	else {
		if (sg_open_files[fd]->_status == FILE_OPEN) {
			//return sg_open_files[fd]->_fd;
			return fd;
		}
		else {
			errno = EBADF;
			return -1;
		}
	}

	return status;
}

int ef_close_status(int fd)
{
	errno = 0;
	if (!sg_open_files[fd]) {
		return FILE_NOT_OPEN;
	}
	if (sg_open_files[fd]->_action && sg_open_files[fd]->_action->_cmd == oper_close) {
		if (sg_open_files[fd]->_action->_action_status == ACTION_COMPLETE) {
			if (sg_open_files[fd]->_action->_return_value == -1) {
				
				errno = sg_open_files[fd]->_action->_err;
				free_file_action(sg_open_files[fd]);
				atomic_thread_fence(memory_order_release);

				return -1;
			}
			else if (sg_open_files[fd]->_action->_return_value == -100) {
				errno = EAGAIN;
				return -1;
			}
			else {
				{
					int htfa = 0;
					while (!atomic_compare_exchange_strong(&(sg_handle_to_file_action[fd]),&htfa,1)) htfa = 0;
				}
				EF_FILE_CLEANUP(sg_open_files[fd]);
				{
					int htfa = 1;
					atomic_compare_exchange_strong(&(sg_handle_to_file_action[fd]),&htfa,0);
				}
				st_enqueue_sg_fd_queue(fd);
				//enqueue(sg_fd_queue,(void*)(long)fd);
				return 0;
			}
		}
		else {
			errno = EAGAIN;
			return -1;
		}
	}
	else {
		errno = EINVAL;
		return -1;
	}

	return FILE_NOT_OPEN;
}

ssize_t ef_read(int fd, void * buf, size_t nbyte)
{
	ssize_t ret = 0;
	uint64_t	t1 = 0, t2 = 0;
	//t1 = clock_gettime_nsec_np(CLOCK_REALTIME);
	ret = low_ef_read(fd,buf,nbyte);
	//t2 = clock_gettime_nsec_np(CLOCK_REALTIME);
	//T+= (t2-t1);
	return ret;
}

/* To initialize the FILE IO module */
/* It is assumed that this function will operate in a 
 * single threaded manner */
void ef_init()
{
	errno = 0;
	ev_init_globals();

	sg_page_size = (size_t)get_sys_pagesize();

	/*
	 * Page size has to be aligned to powers of 2 and a minimum of the 
	 * block size given in the format of the disk.
	 *
	 * This is necessary because, we are doing direct IO and LINUX
	 * platform needs the alignment  for unbuffered IO to work.
	 * 
	 * The current value of 1 M (asuming a format size of 4K) is fixed
	 * to provide for minimizing of IO operations.
	 *
	 * */
	sg_page_size =  256 * sg_page_size;

	/* Setup the table to hold the state data of files. */
	for (int i = 0; i < EF_MAX_FILES; i++) {
		sg_open_files[i] = NULL;
		sg_handle_to_file_action[i] = 0;
	}
	sg_fd_queue = create_ev_queue();
	sg_file_writer_queue = create_ev_queue();
	sg_slow_sync_queue = create_ev_queue();
	setup_fd_slots();


	pthread_mutex_init(&sg_file_writer_mutex, NULL);
	pthread_cond_init(&sg_file_writer_cond, NULL);

	atomic_thread_fence(memory_order_release);

	/* Start the thread pool needed to carryout the file operations
	 * in an async manner.
	 * One thread reserved for transfering data from memory to disk
	 * 2 threads for carrying out asynchrnous, read, open and close
	 * operations.
	 */
	if (!sg_disk_io_thr_pool) sg_disk_io_thr_pool = create_thread_pool(2);
	if (!sg_file_writer_pool) sg_file_writer_pool = create_thread_pool(1);
	if (!sg_slow_sync_pool) sg_slow_sync_pool = create_thread_pool(1);

	enqueue_task(sg_file_writer_pool,file_writer,NULL);
	enqueue_task(sg_slow_sync_pool,slow_sync,NULL);

	sg_ef_init_done = 1;

	return;
}

void ef_set_thrpool(thread_pool_type thr_pool)
{
	if (sg_thr_pool_init_done) return;
	sg_thr_pool_init_done = 1;
	if (sg_disk_io_thr_pool) destroy_thread_pool(sg_disk_io_thr_pool);
	sg_disk_io_thr_pool = thr_pool;

	return;
}

void ef_set_cb_func(ef_notification_f_type cb_func, void * cb_data)
{
	sg_cb_func = cb_func;
	sg_cb_data = cb_data;
}

void ef_unset_cb_func()
{
	sg_cb_func = NULL;
	atomic_thread_fence(memory_order_acq_rel);
	sg_cb_data = NULL;
	atomic_thread_fence(memory_order_acq_rel);
}

ssize_t ef_write(int fd, void * buf, size_t nbyte)
{
	errno = 0;

	if (!sg_open_files[fd]) {
		errno = EINVAL;
		return -1;
	}
	if (NULL == buf || 0 >= nbyte) {
		errno = EINVAL;
		return -1;
	}
	if (sg_open_files[fd]->_status != FILE_OPEN) {
		errno = EBADF;
		return -1;
	}
	if (!(sg_open_files[fd]->_oflag&(O_RDWR|O_WRONLY))) {
		/* File opened without write access. */
		errno = EBADF;
		return -1;
	}
	
	return low_ef_write(sg_open_files[fd],buf, nbyte);
}

/* ef_sync is an extension to ef_write.
 * It ensures that the data in the buffers are pushed
 * to the disk. Entire work is carried out synchronously.
 * This function is to be used only in necessary places,
 * otherwise the advantage of asynchronous writes will be lost. */
int ef_sync(int fd)
{
	int ret = 0;
	off_t		page_offset = 0;

	errno = 0;
	if (!sg_open_files[fd]) { errno = EINVAL; return -1; }

	if (!sg_ef_init_done) {
		errno = EBADF;
		EV_ABORT("INIT NOT DONE");
	}
	if (sg_open_files[fd]->_status != FILE_OPEN) {
		errno = EBADF;
		return -1;
	}
	EF_FILE * file_ptr = sg_open_files[fd];

	atomic_thread_fence(memory_order_acquire);
	if (!sg_open_files[fd]->_action) {
		/* There is nothing outstanding to sync. */
		errno = EINVAL;
		return -1;
	}
	else if (sg_open_files[fd]->_action && sg_open_files[fd]->_action->_action_status == ACTION_COMPLETE) {
		/* There is nothing outstanding to sync. */
		free_file_action(sg_open_files[fd]);
		errno = EINVAL;
		return -1;
	}
	else if (sg_open_files[fd]->_action && sg_open_files[fd]->_action->_cmd != oper_write) {
		errno = EBUSY;
		return -1;
	}
	/* Prevent write from getting suspended. */
	/* From this point below, return should happen only at the end of the function. */
	{
		int ws = 0;
		while (!atomic_compare_exchange_strong(&(file_ptr->_write_suspended),&ws,-2)) ws = 0;
	}

	low_ef_sync(file_ptr);

EF_SYNC_FINALLY:
	/* Allow write suspension after this. */
	{
		int ws = -2;
		if (!atomic_compare_exchange_strong(&(file_ptr->_write_suspended),&ws,0))
			EV_ABORT("This is an impossible condition");
	}
	return 0;

}

