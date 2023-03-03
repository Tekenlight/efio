#include <stdio.h>
#include <stdlib.h>

#include <ev_include.h>
#include <unistd.h>
#include <thread_pool.h>
#include <errno.h>
#include <stdatomic.h>
#include <string.h>
#include <stdbool.h>
#include <ev_queue.h>
#include <assert.h>

#if defined SEMAPHORE_BASED
#include <semaphore.h>
#endif

struct task_s {
	task_func_type		_task_function;
	task_argument_type	_arg;
};

struct thr_s {
	pthread_t	_t;
	atomic_int 	_state;
	atomic_int	_task_count;
	bool	_subscribed;
};
#if defined THREAD_FREE
#undef THREAD_FREE
#endif
#if defined THREAD_BUSY
#undef THREAD_BUSY
#endif
#define THREAD_FREE 0
#define THREAD_BUSY 1

struct thread_pool_s {
	atomic_uint			_cond_count;
	struct thr_s		*_threads;
	int					_num_threads;
	ev_queue_type		_task_queue;
	atomic_int			_shutdown;
	atomic_int			_enq_index;
	ev_queue_type		_free_thr_queue;
	int					_min_sleep_usec;
	int					_max_sleep_usec;
	int					_alwd_busy_waits;
#if defined MUTEX_BASED
	pthread_cond_t		_cond;
	pthread_mutex_t		_mutex;
#elif defined SEMAPHORE_BASED
	sem_t*				_semaphore;
#else
#error
#endif
};

struct thr_free_s {
	pthread_t	_t;
	int			_thr_index;
};

struct thr_inp_data_s {
	struct thread_pool_s	*_pool;
	int						_thr_index;
};

static void thr_wakeup_sig_handler(int s)
{
	return;
}

// The main thread looping to take and execute a task.
//
static void * thread_loop(void *data)
{
	struct thread_pool_s	*pool = NULL;
	struct task_s			*qe = NULL;
	int						s = 0;
	unsigned int			o_c = 0;
	unsigned int			c = 0;
	int						thr_index = 0;
	int						slept_count = 0;
	int						sleeping_time = 0;
	int						slept_time = 0;
	int						ret = 0;

	pool = ((struct thr_inp_data_s *)data)->_pool;
	thr_index = ((struct thr_inp_data_s *)data)->_thr_index;
	free(data);
	data = NULL; //Seems to be giving problem in Ubuntu

	//EV_DBGP("POOL = [%p]\n", pool);
	pool->_threads[thr_index]._state = THREAD_FREE;
	for (;;) {
		qe = NULL;
		s = atomic_load_explicit(&(pool->_shutdown),memory_order_acquire);
		if (!s) {
			int i = 0;
			while (s == 0) {
				qe = dequeue(pool->_task_queue);
				if (qe) {
					break;
				}

				ret = 0;
#if defined MUTEX_BASED
				ret = pthread_mutex_lock(&(pool->_mutex));
				if (ret) {
					EV_DBGP("POOL COND = [%p]\n", &(pool->_cond));
					EV_ABORT("%s\n", strerror(ret));
				}

				s = atomic_load_explicit(&(pool->_shutdown),memory_order_acquire);
				if (s) {
					pthread_mutex_unlock(&(pool->_mutex));
					break;
				}
				qe = dequeue(pool->_task_queue);
				if (qe) {
					pthread_mutex_unlock(&(pool->_mutex));
					break;
				}
#endif

				ret = 0;
#if defined MUTEX_BASED
				ret = pthread_cond_wait(&(pool->_cond), &(pool->_mutex));
				if (ret) {
					EV_DBGP("POOL COND = [%p] \n", &(pool->_cond));
					EV_DBGP("%s\n", strerror(ret));
					EV_ABORT("%s\n", strerror(ret));
				}
				pthread_mutex_unlock(&(pool->_mutex));
#elif defined SEMAPHORE_BASED
				ret = sem_wait(pool->_semaphore);
				if (ret) {
					EV_DBGP("SEMAPHORE = [%p] \n", &(pool->_semaphore));
					EV_DBGP("%s\n", strerror(errno));
					EV_ABORT("%s\n", strerror(errno));
				}
#else
#error
#endif
				qe = dequeue(pool->_task_queue);
				if (qe) {
					break;
				}

				//EV_DBGP("WAITING COMPLETE FOR COND on [%p]\n", pool);fflush(stdout);
				s = atomic_load_explicit(&(pool->_shutdown),memory_order_acquire);
				//if ((sleeping_time + pool->_min_sleep_usec)<pool->_max_sleep_usec) i++;
			}
		}
		if (s) {
			// If stop signal is coming out of _shutdown set to 1
			//EV_DBGP("Here\n");
			if (qe) {
				//EV_DBGP("[%p]\n", qe);
				if ((qe->_task_function)) {
					//EV_DBGP("Here\n");
				}
				else {
					//EV_DBGP("[%p]\n", qe);
					free(qe);
					qe = NULL;
					break;
				}
			}
			else {
				qe = dequeue(pool->_task_queue);
				if (qe) {
					//EV_DBGP("[%p]\n", qe);
					if ((qe->_task_function)) {
						//EV_DBGP("Here\n");
					}
					else {
						//EV_DBGP("[%p]\n", qe);
						free(qe);
						qe = NULL;
						break;
					}
				}
				else {
					//EV_DBGP("Here\n");
					break;
				}
			}
		}
		else {
			if (qe != NULL) {
				if (!(qe->_task_function)) {
					// If stop signal is coming out of a NULL task
					//printf("%s:%d [%lx]\n", __FILE__, __LINE__, pthread_self());
					free(qe);
					qe = NULL;
					break;
				}
			}
			else {
				EV_ABORT("Dequeued element cannot be NULL. ");
			}
		}
		atomic_thread_fence(memory_order_acq_rel);
		pool->_threads[thr_index]._state = THREAD_BUSY;
		if (!(qe->_task_function)) EV_ABORT("Task function cannot be null. ");
		//EV_DBGP("Here qe=%p\n", qe);
		//EV_DBGP("Here tf=%p\n", qe->_task_function);
		//EV_DBGP("Here arg=%p\n", qe->_arg);
		(*(qe->_task_function))(qe->_arg);
		//EV_DBGP("Here\n");
		free(qe);
		//EV_DBGP("Here\n");
		atomic_fetch_add(&(pool->_threads[thr_index]._task_count),1);
		pool->_threads[thr_index]._state = THREAD_FREE;
	}

	//EV_DBGP("Slept for %d times and for %d us\n",slept_count,(slept_time));

	return NULL;
}

static void wake_all_threads(struct thread_pool_s *pool, int immediate)
{
	//EV_DBGP("WAKING ALL THREADS [%d] on [%p]\n", pool->_num_threads, pool);fflush(stdout);
	struct task_s * qe = NULL;
	for (int i = 0; i < pool->_num_threads; i++) {
		qe = malloc(sizeof(struct task_s));
		qe->_task_function = NULL;
		qe->_arg = NULL;
		enqueue(pool->_task_queue,qe);
#if defined SEMAPHORE_BASED
		sem_post(pool->_semaphore);
#endif
	}
#if defined MUTEX_BASED
	pthread_mutex_lock(&(pool->_mutex));
	pthread_cond_broadcast(&(pool->_cond));
	pthread_mutex_unlock(&(pool->_mutex));
#endif
	return;
}

static void wake_any_one_thread(struct thread_pool_s *pool)
{
	//EV_DBGP("SIGNALLING CONDITION on [%p]\n", pool);fflush(stdout);
#if defined MUTEX_BASED
	pthread_mutex_lock(&(pool->_mutex));
	pthread_cond_signal(&(pool->_cond));
	pthread_mutex_unlock(&(pool->_mutex));
#elif defined SEMAPHORE_BASED
	sem_post(pool->_semaphore);
#else
#error
#endif
	return;
}

struct thread_pool_s * create_thread_pool(int num_threads)
{
	//EV_DBGP("CREATING A THREAD POOL OF [%d] THREADS\n", num_threads);fflush(stdout);
	struct thread_pool_s * pool =NULL;
	pthread_attr_t attr;

	pthread_attr_init(&attr);
	//pthread_attr_setschedpolicy(&attr,SCHED_RR);
	//pthread_attr_setinheritsched(&attr,PTHREAD_EXPLICIT_SCHED);

	if (0>=num_threads) {
		errno = EINVAL;
		return NULL;
	}

	pool = malloc(sizeof(struct thread_pool_s));
	//EV_DBGP("POOL = [%p]\n", pool);fflush(stdout);
	memset(pool, 0, sizeof(struct thread_pool_s));

	pool->_threads = malloc(num_threads*sizeof(struct thr_s));
	memset(pool->_threads, 0, num_threads*sizeof(struct thr_s));
	pool->_num_threads = 0;
	pool->_cond_count = 0;
	pool->_task_queue = create_ev_queue();
	pool->_shutdown = 0;
	pool->_enq_index = 0;
	pool->_free_thr_queue = create_ev_queue();
	pool->_min_sleep_usec = MIN_SLEEP_USEC;;
    pool->_max_sleep_usec = MAX_SLEEP_USEC;
    pool->_alwd_busy_waits = ALWD_BUSY_WAITS;
	//EV_DBGP("POOL = [%p]\n", pool);
	int ret = 0;
#if defined MUTEX_BASED
	ret = pthread_cond_init(&(pool->_cond), NULL);
	pthread_mutex_init(&(pool->_mutex), NULL);
	if (ret) {
		EV_ABORT("%s\n", strerror(ret));
	}
#elif defined SEMAPHORE_BASED
	char buf[L_tmpnam + 1];
	memset(buf, 0, (L_tmpnam+1));
	strcpy(buf, "/tmp/tmp.XXXXXX");
	char * tmp_name = tmpnam(buf);
	pool->_semaphore = sem_open(tmp_name, O_CREAT, S_IRUSR|S_IWUSR, 0);
	if (pool->_semaphore == SEM_FAILED) {
		EV_ABORT("%s\n", strerror(errno));
	}
#else
#error
#endif
	//EV_DBGP("POOL COND = [%p] ret=[%d]\n", &(pool->_cond), ret); fflush(stdout);
	{
		for (int i=0; i < num_threads;i++) {
			struct thr_inp_data_s * data = malloc(sizeof(struct thr_inp_data_s));
			data->_pool = pool;
			data->_thr_index = i;
			if (pthread_create(&pool->_threads[i]._t,&attr,thread_loop,data)) {
				destroy_thread_pool(pool);
				EV_ABORT("Could not create thread \n");
				return NULL;
			}
			//printf("%s:%d Created thread [%p]\n", __FILE__, __LINE__, pool->_threads[i]._t);
			pool->_threads[i]._state = THREAD_FREE;
			pool->_threads[i]._task_count = 0;
			pool->_num_threads++;
		}
	}

#if defined MUTEX_BASED
	pthread_attr_destroy(&attr);
#endif

	return pool;
}

static void free_thread_pool(struct thread_pool_s *pool)
{
	struct task_s * qe = NULL;

    free(pool->_threads);
#if defined MUTEX_BASED
	destroy_ev_queue(pool->_task_queue);
	destroy_ev_queue(pool->_free_thr_queue);
	pthread_cond_destroy(&(pool->_cond));
#elif defined SEMAPHORE_BASED
	sem_close(pool->_semaphore);
#else
#error
#endif

	return;
}

typedef struct {
	void*						_task_input;
	void*						_ref_data;
	task_func_with_return_t		_task_func;
	notify_task_completion_t	_on_complete;
} generic_task_data_t, * generic_task_data_ptr_t;

static void envelope_function(void* task_data)
{
	/*
		EV_DBGP("Here tdp = %p\n", task_data);
		EV_DBGP("Here func = %p inp = %p \n", tdp->_task_func, tdp->_task_input );
		EV_DBGP("Here\n");
		EV_DBGP("Here\n");
		EV_DBGP("Here\n");
	*/
	generic_task_data_ptr_t tdp = (generic_task_data_ptr_t)(task_data);
	void * return_data = tdp->_task_func(tdp->_task_input);
	tdp->_on_complete(return_data, tdp->_ref_data);
	free(tdp);
	return;
}

void enqueue_task_function (struct thread_pool_s *pool, task_func_with_return_t func,
			task_argument_type input_data, void * ref_data, notify_task_completion_t notification_func)
{

	/*
	EV_DBGP("Here tdp = %p\n", tdp);
	EV_DBGP("Here func = %p notification_func = %p inp = %p \n", tdp->_task_func, tdp->_on_complete, tdp->_task_input);
	EV_DBGP("Here qe = %p, qe->_arg=tdp = %p\n", qe, qe->_arg);
	EV_DBGP("Here\n");
	*/
	generic_task_data_ptr_t tdp = malloc(sizeof(generic_task_data_t));
	tdp->_task_input = input_data;
	tdp->_ref_data = ref_data;
	tdp->_task_func = func;
	tdp->_on_complete = notification_func;

	enqueue_task(pool, envelope_function, tdp);
}

void enqueue_task(struct thread_pool_s *pool, task_func_type func,
										task_argument_type argument)
{
	struct task_s * qe = NULL;
	qe = malloc(sizeof(struct task_s));
	qe->_task_function = func;
	qe->_arg = argument;
	enqueue(pool->_task_queue,qe);

	atomic_fetch_add(&pool->_cond_count,1);
	wake_any_one_thread(pool);
}

int destroy_thread_pool(struct thread_pool_s *pool)
{
	int s = 0;

	if (!pool) {
		errno = EINVAL;
		return -1;
	}

	s = atomic_load_explicit(&pool->_shutdown,memory_order_acquire);
	if (s > 0) {
		errno = EBUSY;
		return -1;
	}

	atomic_store_explicit(&(pool->_shutdown),1,memory_order_release);
	wake_all_threads(pool,0);

	for (int i=0;i<pool->_num_threads ; i++) {
		if (pthread_join(pool->_threads[i]._t,NULL)) {
			//printf("%s:%d thread=[%d] thread id [%lx]\n", __FILE__, __LINE__, i, pool->_threads[i]._t);
			EV_ABORT("pthread_join failed");
		}
		//printf("%s:%d thread=[%d] thread id [%lx]\n", __FILE__, __LINE__, i, pool->_threads[i]._t);
	}
	//printf("%s:%d\n", __FILE__, __LINE__);

	free_thread_pool(pool);

	return 0;
}

int thrpool_get_task_count(struct thread_pool_s *pool)
{
	int count = 0, i = 0;
	for (i=0;i<pool->_num_threads;i++) {
		count += atomic_load_explicit(&pool->_threads[i]._task_count,memory_order_acquire);
	}
	return count;
}
