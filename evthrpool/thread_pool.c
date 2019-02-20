#include <stdio.h>

#include <ev_include.h>
#include <unistd.h>
#include <thread_pool.h>
#include <errno.h>
#include <stdatomic.h>
#include <string.h>
#include <stdbool.h>
#include <ev_queue.h>

struct task_s {
	task_func_type task_function;
	task_argument_type * arg;
	int					_shutdown;
};

struct thr_s {
	pthread_t	t;
	atomic_int 	state;
	atomic_int	task_count;
	bool	subscribed;
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
};

struct thr_free_s {
	pthread_t	t;
	int			thr_index;
};

struct thr_inp_data_s {
	struct thread_pool_s	*pool;
	int						thr_index;
};

void thr_wakeup_sig_handler(int s)
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

	pool = ((struct thr_inp_data_s *)data)->pool;
	thr_index = ((struct thr_inp_data_s *)data)->thr_index;
	free(data);
	data = NULL;

	pool->_threads[thr_index].state = THREAD_FREE;
	for (;;) {
		s = atomic_load_explicit(&(pool->_shutdown),memory_order_relaxed);
		if (s) break;
		qe = dequeue(pool->_task_queue);
		if (!qe) {
			int i = 0;
			while (s == 0) {
				pool->_threads[thr_index].subscribed = true;
				sleeping_time = (i>pool->_alwd_busy_waits)?i*pool->_min_sleep_usec:0;
				slept_time += sleeping_time;
				slept_count++;
				if (pool->_threads[thr_index].subscribed) {
					if (i>pool->_alwd_busy_waits) usleep(sleeping_time);
					else pthread_yield_np();
				}
				pool->_threads[thr_index].subscribed = false;

				qe = dequeue(pool->_task_queue);
				if (qe) {
					if (!(qe->task_function)) {
						s = 1;
					}
					break;
				}
				s = atomic_load_explicit(&(pool->_shutdown),memory_order_relaxed);
				if ((sleeping_time + pool->_min_sleep_usec)<pool->_max_sleep_usec) i++;
			}
		}
		if (s) break;
		atomic_thread_fence(memory_order_acq_rel);
		pool->_threads[thr_index].state = THREAD_BUSY;
		(*(qe->task_function))(qe->arg);
		free(qe);
		atomic_fetch_add(&(pool->_threads[thr_index].task_count),1);
		pool->_threads[thr_index].state = THREAD_FREE;
	}

	//EV_DBGP("Slept for %d times and for %d us\n",slept_count,(slept_time));

	return NULL;
}

struct thread_pool_s * create_thread_pool(int num_threads)
{
	struct thread_pool_s * pool =NULL;
	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setschedpolicy(&attr,SCHED_RR);
	pthread_attr_setinheritsched(&attr,PTHREAD_EXPLICIT_SCHED);

	if (0>=num_threads) {
		errno = EINVAL;
		return NULL;
	}

	pool = malloc(sizeof(struct thread_pool_s));

	pool->_threads = malloc(num_threads*sizeof(struct thr_s));
	pool->_num_threads = 0;
	pool->_cond_count = 0;
	pool->_task_queue = create_ev_queue();
	pool->_shutdown = 0;
	pool->_enq_index = 0;
	pool->_free_thr_queue = create_ev_queue();
	pool->_min_sleep_usec = MIN_SLEEP_USEC;;
    pool->_max_sleep_usec = MAX_SLEEP_USEC;
    pool->_alwd_busy_waits = ALWD_BUSY_WAITS;
	{
		for (int i=0; i < num_threads;i++) {
			struct thr_inp_data_s * data = malloc(sizeof(struct thr_inp_data_s));
			data->pool = pool;
			data->thr_index = i;
			if (pthread_create(&pool->_threads[i].t,&attr,thread_loop,data)) {
				destroy_thread_pool(pool);
				return NULL;
			}
			pool->_threads[i].state = THREAD_FREE;
			pool->_threads[i].task_count = 0;
			pool->_num_threads++;
		}
	}

	pthread_attr_destroy(&attr);

	return pool;
}

void free_thread_pool(struct thread_pool_s *pool)
{
	struct task_s * qe = NULL;

    free(pool->_threads);
	destroy_ev_queue(pool->_task_queue);
	destroy_ev_queue(pool->_free_thr_queue);

	return;
}

static void wake_all_threads(struct thread_pool_s *pool, int immediate)
{
	struct task_s * qe = NULL;
	for (int i = 0; i < pool->_num_threads; i++) {
		qe = malloc(sizeof(struct task_s));
		qe->task_function = NULL;
		enqueue(pool->_task_queue,qe);
	}
	return;
}

static void wake_any_one_thread(struct thread_pool_s *pool)
{
	return;
}

void enqueue_task(struct thread_pool_s *pool, task_func_type func,
										task_argument_type argument)
{
	struct task_s * qe = NULL;
	qe = malloc(sizeof(struct task_s));
	qe->task_function = func;
	qe->arg = argument;
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

	s = atomic_load_explicit(&pool->_shutdown,memory_order_relaxed);
	if (s > 0) {
		errno = EBUSY;
		return -1;
	}

	atomic_store_explicit(&(pool->_shutdown),1,memory_order_release);

	wake_all_threads(pool,0);

	for (int i=0;i<pool->_num_threads ; i++) {
		pthread_join(pool->_threads[i].t,NULL);
	}

	free_thread_pool(pool);

	return 0;
}

#ifdef NEVER

#include <stdio.h>

void task_func(void * data)
{
	char * s = data;
	//usleep(100000);
	//usleep(25);
	//usleep(10);
	return;
}

struct __s {
	int N;
	struct thread_pool_s *pool;
};

void * enq(void *data)
{
	int i = 0;
	struct __s * sptr = data;
	int N = sptr->N;
	for (i=0; i< N/4; i++) {
		enqueue_task(sptr->pool, task_func, (void*)("WORLD HELLO"));;
		//usleep(100000);
	}
	//printf("ENQ complete [%p]\n",pthread_self());
	return NULL;
}

int main()
{
	struct thread_pool_s *pool = NULL;
	int N = 2000000;
	int THREADS = 4;
	int total = 0;
	//int N = 20;
	struct __s s;
	pthread_t t0,t1, t2, t3, t4;
	pool = create_thread_pool(THREADS);
	s.N = N;
	s.pool = pool;

	pthread_create(&t1, NULL, enq, &s);
	pthread_create(&t2, NULL, enq, &s);
	pthread_create(&t3, NULL, enq, &s);
	pthread_create(&t4, NULL, enq, &s);
	pthread_join(t1,NULL);
	pthread_join(t2,NULL);
	pthread_join(t3,NULL);
	pthread_join(t4,NULL);
	{
		int i = 0;
		total = 0;
		for (i=0;i<THREADS;i++) {
			total += atomic_load_explicit(&pool->_threads[i].task_count,memory_order_relaxed);
		}
	}

	while (total < N) {
		pthread_yield_np();
		{
			int i = 0;
			total = 0;
			for (i=0;i<THREADS;i++) {
				total += atomic_load_explicit(&pool->_threads[i].task_count,memory_order_relaxed);
			}
		}
	}

	printf("Number of exec times = [%d]\n", total);
	destroy_thread_pool(pool);

	return 0;
}

#endif
