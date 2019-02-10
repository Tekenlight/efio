#include <stdio.h>

#include <ev.h>
#include <unistd.h>
#include <thread_pool.h>
#include <errno.h>
#include <stdatomic.h>
#include <signal.h>
#include <string.h>
#include <stdbool.h>
#include <ev_queue.h>
#include <pthread.h>

struct task_s {
	task_func_type task_function;
	task_argument_type * arg;
};

struct thr_s {
	pthread_t	t;
	atomic_int 	state;
	atomic_int	task_count;
	bool	subscribed;
	ev_queue_type		_task_queue;
	atomic_int			enqueued;
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
	atomic_int			_shutdown;
	struct  sigaction	*_o_sigh_ptr;
	ev_queue_type		_free_thr_queue;
	ev_queue_type		_pending_tasks;
	ev_queue_type		_monitor_tasks;
	pthread_t			_pend_task_monitor;
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
	puts("AWAKE");
	return;
}

static bool test_inc_retest(atomic_int *val)
{
	bool result = (atomic_load_explicit(val,memory_order_relaxed) == 0);
	if (result) {
		if (atomic_fetch_add_explicit(val,1,memory_order_relaxed) > 1) {
			atomic_fetch_add_explicit(val,-1,memory_order_relaxed);
			result = false;
		}
	}
	return result;
}

static bool test_dec_retest(atomic_int *val)
{
	bool result = (atomic_load_explicit(val,memory_order_relaxed) == 1);
	if (result) {
		if (atomic_fetch_add_explicit(val,-1,memory_order_relaxed) < 0) {
			atomic_fetch_add_explicit(val,1,memory_order_relaxed);
			result = false;
		}
	}
	return result;
}

static void subscribe_thread(struct thread_pool_s *pool, int t_id)
{
	struct thr_free_s *elem = malloc(sizeof(struct thr_free_s));
	elem->t = pthread_self();
	elem->thr_index = t_id;
	//if (test_inc_retest(&(pool->_threads[t_id].enqueued))) {
	//}
	//enqueue(pool->_free_thr_queue,elem);
	pool->_threads[t_id].subscribed = true;

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
	sigset_t				set;
	sigset_t				o_set;
	sigset_t				r_set;
	int						sig = 0;
	int						thr_index = 0;
	int i = 0;

	pool = ((struct thr_inp_data_s *)data)->pool;
	thr_index = ((struct thr_inp_data_s *)data)->thr_index;
	free(data);
	data = NULL;

	sigemptyset(&set);
	//sigemptyset(&o_set);
	sigaddset(&set,SIGUSR1);
	pthread_sigmask(SIG_BLOCK, &set, &o_set);

	//EV_DBGP("Enter the thread %d name %p\n",thr_index,pthread_self());

	pool->_threads[thr_index].state = THREAD_FREE;
	for (;;) {
		
		s = atomic_load_explicit(&(pool->_shutdown),memory_order_relaxed);
		if (s) break;
		qe = dequeue(pool->_threads[thr_index]._task_queue);
		while (!qe && s == 0) {
			pthread_sigmask(SIG_BLOCK, &set, &o_set);

			pool->_threads[thr_index].subscribed = false;

			//subscribe_thread(pool,thr_index);

			sig = 0;
			//if (pool->_threads[thr_index].subscribed) pthread_yield_np();
			//EV_DBGP("Hello world set = %d\n",set);
			//if (!sigpending(&set)) //EV_DBGP("Hello world set = %d\n",set);
			//if (pool->_threads[thr_index].subscribed) do { sig = 0; sigwait(&set,&sig); } while (sig == 0);
			//if (pool->_threads[thr_index].subscribed) { pthread_yield_np(); }
			{ pthread_yield_np(); }
			pool->_threads[thr_index].subscribed = false;
			i++;

			qe = dequeue(pool->_threads[thr_index]._task_queue);
			if (qe) {
				if (!(qe->task_function)) {
					s = 1;
				}
				break;
			}

			s = atomic_load_explicit(&(pool->_shutdown),memory_order_relaxed);
		}
		if (s) break;
		atomic_thread_fence(memory_order_seq_cst);
		pool->_threads[thr_index].state = THREAD_BUSY;
		(*(qe->task_function))(qe->arg);
		free(qe);
		atomic_thread_fence(memory_order_seq_cst);
		atomic_fetch_add_explicit(&(pool->_threads[thr_index].task_count),1,memory_order_seq_cst);
		//++(pool->_threads[thr_index].task_count);
		//atomic_thread_fence(memory_order_seq_cst);
		pool->_threads[thr_index].state = THREAD_FREE;
	}

	return NULL;
}

struct task_monitor_s {
	struct thread_pool_s *pool;
};

union monitor_inp{
	int		thr_index;
	struct	task_s * qe;
};

typedef enum {
	monitor_shut_down=0,
} monitor_evt_enum;

struct monitor_task_s {
	monitor_evt_enum _evt;
};

static bool desubscribe_thread(struct thread_pool_s *pool, int t_id)
{
	return test_dec_retest(&(pool->_threads[t_id].enqueued));
}

static void wake_subscribed_thread(struct thread_pool_s *pool,struct thr_free_s * elem, struct task_s *qe)
{
	//EV_DBG();
	enqueue(pool->_threads[elem->thr_index]._task_queue,qe);
	//pthread_kill(pool->_threads[elem->thr_index].t,SIGUSR1);
	pthread_kill(elem->t,SIGUSR1);
}

static void * task_monitor(void * data)
{
	struct monitor_task_s * mt = NULL;
	struct task_monitor_s * ptr = data;
	struct thread_pool_s *pool = ptr->pool;
	struct task_s * qe = NULL;
	struct thr_free_s *elem = NULL;
	free(ptr);
	ptr = NULL;

	while (1) {
		if (atomic_load(&(pool->_shutdown))) break;
		if (!qe) {
			qe = (struct task_s *)dequeue(pool->_pending_tasks);
		}
		if (!qe) { pthread_yield_np();continue;}
		if (qe && !elem) elem = (struct thr_free_s *)dequeue(pool->_free_thr_queue);
		if (qe && !elem) { pthread_yield_np(); continue; }
		else if (!elem) { pthread_yield_np(); continue; }
		//EV_DBG();
		wake_subscribed_thread(pool,elem,qe);
		free(elem);
		elem = NULL;
		qe = NULL;
	}

	return NULL;
}

struct thread_pool_s * create_thread_pool(int num_threads)
{
	struct thread_pool_s * pool =NULL;
	struct  sigaction sigh;
	struct  sigaction * o_sigh_ptr = NULL;

	if (0>=num_threads) {
		errno = EINVAL;
		return NULL;
	}

	pool = malloc(sizeof(struct thread_pool_s));
	memset(pool,0,sizeof(struct thread_pool_s));

	pool->_threads = malloc(num_threads*sizeof(struct thr_s));
	pool->_num_threads = 0;
	pool->_cond_count = 0;
	pool->_shutdown = 0;
	pool->_free_thr_queue = create_ev_queue();
	pool->_pending_tasks = create_ev_queue();
	pool->_monitor_tasks = create_ev_queue();
	{
		sigset_t				set;
		sigset_t				o_set;
		int						sig = 0;
		sigemptyset(&set);
		sigemptyset(&o_set);
		sigaddset(&set,SIGUSR1);
		//pthread_sigmask(SIG_BLOCK, &set, &o_set);

		for (int i=0; i < num_threads;i++) {
			struct thr_inp_data_s * data = malloc(sizeof(struct thr_inp_data_s));
			data->pool = pool;
			data->thr_index = i;
			pool->_threads[i].state = THREAD_FREE;
			pool->_threads[i].task_count = 0;
			pool->_threads[i]._task_queue = create_ev_queue();
			pool->_num_threads++;
			if (pthread_create(&pool->_threads[i].t,NULL,thread_loop,data)) {
				destroy_thread_pool(pool);
				return NULL;
			}
		}
		{
			struct task_monitor_s * ptr = malloc(sizeof(struct task_monitor_s));
			ptr->pool = pool;
			pthread_create(&pool->_pend_task_monitor,NULL,task_monitor,ptr);
			ptr = NULL;
		}

		/*
		for (int i = 0; i < pool->_num_threads; i++)
			printf("CREATED %s:%d %d %p\n",__FILE__,__LINE__,i,pool->_threads[i].t);
		*/
		//o_sigh_ptr = malloc(sizeof(struct  sigaction));
		memset(&sigh,0,sizeof(struct  sigaction));
		sigh.sa_handler = thr_wakeup_sig_handler;
		//sigaction(SIGUSR1,&sigh,NULL);
		//pthread_sigmask(SIG_SETMASK, &o_set, NULL);
	}


	return pool;
}

void free_thread_pool(struct thread_pool_s *pool)
{
	struct task_s * qe = NULL;

	for (int i=0; i < pool->_num_threads;i++) {
		destroy_ev_queue(pool->_threads[i]._task_queue);
	}
    free(pool->_threads);
	destroy_ev_queue(pool->_free_thr_queue);

	return;
}

static void wake_all_threads(struct thread_pool_s *pool, int immediate)
{
	struct task_s * qe = NULL;
	for (int i = 0; i < pool->_num_threads; i++) {
		qe = malloc(sizeof(struct task_s));
		qe->task_function = NULL;
		enqueue(pool->_threads[i]._task_queue,qe);
		pthread_kill(pool->_threads[i].t,SIGUSR1);
	}
	return;
}

static atomic_int counter = 0;
static atomic_int enq_counter = 0;
static void wake_any_one_thread(struct thread_pool_s *pool, struct task_s * qe)
{
	int i = 0;
	int j = 0;
	j = atomic_fetch_add(&counter,1);
	i = j % pool->_num_threads;
	enqueue(pool->_threads[i]._task_queue,qe);
	//printf("i = %d num_threads = %d\n",i,pool->_num_threads);
	//printf("j = %d\n",j);
	/*
	struct monitor_task_s * mt = NULL;
	bool thread_found = false;
	struct thr_free_s *elem = NULL;

	while (1) {
		elem = (struct thr_free_s *)dequeue(pool->_free_thr_queue);
		atomic_thread_fence(memory_order_acq_rel);
		if (elem) {
			wake_subscribed_thread(pool,elem,qe);
			free(elem);
			thread_found = true;
		}
		break;
	}

	if (thread_found) {
	//EV_DBGP("Free thread found\n");
		return;
	}
	//EV_DBGP("Free thread notthere\n");

	//mt = malloc(sizeof(struct monitor_task_s));

	enqueue(pool->_pending_tasks,qe);
	//enqueue(pool->_monitor_tasks,mt);
	//
	*/

	return;
}

void enqueue_task(struct thread_pool_s *pool, task_func_type func,
										task_argument_type argument)
{
	struct task_s * qe = NULL;
	qe = malloc(sizeof(struct task_s));
	qe->task_function = func;
	qe->arg = argument;

	atomic_fetch_add(&pool->_cond_count,1);
	wake_any_one_thread(pool, qe);
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

	if (!atomic_compare_exchange_strong_explicit(&(pool->_shutdown),&s,s+1,memory_order_relaxed,memory_order_relaxed)) {
		errno = EBUSY;
		return -1;
	}

	//EV_DBG();
	wake_all_threads(pool,0);

	for (int i=0;i<pool->_num_threads ; i++) {
		pthread_join(pool->_threads[i].t,NULL);
	}
	pthread_join(pool->_pend_task_monitor,NULL);

	free_thread_pool(pool);

	return 0;
}

#ifdef NEVER

#include <stdio.h>

void task_func(void * data)
{
	char * s = data;
	//usleep(10);
	//usleep(25);
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
	struct thread_pool_s *pool = sptr->pool;
	for (i=0; i< N/4; i++) {
		enqueue_task(pool, task_func, (void*)("WORLD HELLO"));;
		//usleep(20);
	}
	return NULL;
}

int main()
{
	struct thread_pool_s *pool = NULL;
	int N = 2000000;
	//int N = 200;
	//int N = 4;
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
	//EV_DBG();
	pthread_join(t1,NULL);
	//EV_DBG();
	pthread_join(t2,NULL);
	//EV_DBG();
	pthread_join(t3,NULL);
	//EV_DBG();
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
				total += atomic_load_explicit(&(pool->_threads[i].task_count),memory_order_seq_cst);
			}
		}
	}

	printf("Number of exec times = [%d]\n", total);
	destroy_thread_pool(pool);

	return 0;
}

#endif
