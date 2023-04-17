#include <stdio.h>
#include <ev_include.h>
#include <ev_queue.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <CuTest.h>

struct thr_inp {
	ev_queue_type pq_ptr;
	int N;
	int slot;
};

void * thr(void *param)
{
	struct thr_inp * inp;
	int i = 0;
	int j = 0;
	inp = param;

	ev_queue_type pq_ptr = (ev_queue_type)inp->pq_ptr;;
	int N = inp->N;
	int slot = inp->slot;
	int COUNT = N/4;

	if (slot == 5) {
		j = 0;
		COUNT = N;
	}
	else {
		j = slot*N/4;
		COUNT = N/4;
	}

	for (i=0;i<COUNT;i++,j++) {
		enqueue(pq_ptr,(void*)(long)j);
		dequeue(pq_ptr);
	}

	return NULL;
}

void * thr_TEST(void *param)
{
	int i = 0; int j= 0;
	int fd = 0;
	ev_queue_type pq_ptr = (ev_queue_type)param;
	for (i=0;i<128;i++,j++) {
		enqueue(pq_ptr,(void*)(long)j);
		fd = (int)(long)dequeue(pq_ptr);
		printf("THREAD_TEST:%d:%d\n",i,fd);fflush(stdout);
	}

	return NULL;
}

static void test_main(CuTest *tc)
{
	int i = 0, fd = 0;;
	pthread_t t0,t1, t2, t3, t4, t5;
	void *retptr = NULL;
	struct thr_inp inp1 ;
	struct thr_inp inp2 ;
	struct thr_inp inp3 ;
	struct thr_inp inp4 ;
	struct thr_inp inp5 ;

	ev_queue_type q;

	q = create_ev_queue();

	inp1.pq_ptr = q;
	inp1.N = 64*1024*1024;
	inp2.pq_ptr = q;
	inp2.N = 64*1024*1024;
	inp3.pq_ptr = q;
	inp3.N = 64*1024*1024;
	inp4.pq_ptr = q;
	inp4.N = 64*1024*1024;
	inp5.pq_ptr = q;
	inp5.N = 64*1024*1024;

	/*
	pthread_create(&t0,NULL,thr_TEST,q);
	*/
	inp1.slot = 0;
	pthread_create(&t1, NULL, thr, &inp1);
	inp2.slot = 1;
	pthread_create(&t2, NULL, thr, &inp2);
	inp3.slot = 2;
	pthread_create(&t3, NULL, thr, &inp3);
	inp4.slot = 3;
	pthread_create(&t4, NULL, thr, &inp4);

	/*
	inp5.slot = 5;
	pthread_create(&t5, NULL, thr, &inp5);
	pthread_join(t0,&retptr);
	*/
	pthread_join(t1,&retptr);
	pthread_join(t2,&retptr);
	pthread_join(t3,&retptr);
	pthread_join(t4,&retptr);

	/*
	pthread_join(t5,&retptr);
	*/
	//printf("HELLO\n");

	{
		void * p;
		while (NULL != (p = dequeue(q))) {
			++i;
			fd = (int)(long)p;
			printf("[MAIN]:%d:%d\n",i,fd);//fflush(stdout);
		}
	}


	//destroy_ev_queue(&q);

	CuAssertTrue(tc, i==0);
}


static int run_ev_test()
{
	CuSuite* suite = CuSuiteNew();
	CuString *output = CuStringNew();

	SUITE_ADD_TEST(suite,test_main);

	CuSuiteRun(suite);
    CuSuiteSummary(suite, output);
    CuSuiteDetails(suite, output);
    printf("%s\n", output->buffer);
	printf("Count = %d\n",suite->count);
	int ret = suite->failCount;
	return ret;
}

int main()
{
	return run_ev_test();
}
