#include <stdio.h>

#include <ev_include.h>
#include <unistd.h>
#include <thread_pool.h>
#include <errno.h>
#include <stdatomic.h>
#include <string.h>
#include <stdbool.h>
#include <ev_queue.h>


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

int run_ev_globals_test()
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
		total += thrpool_get_task_count(pool);
	}

	while (total < N) {
		pthread_yield_np();
		{
			int i = 0;
			total = 0;
			total = thrpool_get_task_count(pool);
		}
	}

	printf("Number of exec times = [%d]\n", total);
	destroy_thread_pool(pool);

	return (total == N)?0:1;
}

int main()
{
	return run_ev_globals_test();
}
