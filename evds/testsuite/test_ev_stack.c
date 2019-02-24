#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <ev_stack.h>
#include <CuTest.h>

void * thr1(void *param)
{
	int i = 0;
	int j = 0*1024*1024;
	ev_stack_type ev_stack = (ev_stack_type)param;
	for (i=0;i<1024*1024;i++,j++) {
		push(ev_stack,(void*)(long)j);
		pop(ev_stack);
	}

	return NULL;
}

void * thr2(void *param)
{
	int i = 0;
	int j = 1*1024*1024;
	ev_stack_type ev_stack = (ev_stack_type)param;
	for (i=0;i<1024*1024;i++,j++) {
		push(ev_stack,(void*)(long)j);
		pop(ev_stack);
	}

	return NULL;
}

void * thr3(void *param)
{
	int i = 0;
	int j = 2*1024*1024;
	ev_stack_type ev_stack = (ev_stack_type)param;
	for (i=0;i<1024*1024;i++,j++) {
		push(ev_stack,(void*)(long)j);
		pop(ev_stack);
	}

	return NULL;
}

void * thr4(void *param)
{
	int i = 0;
	int j= 3*1024*1024;
	ev_stack_type ev_stack = (ev_stack_type)param;
	for (i=0;i<1024*1024;i++,j++) {
		push(ev_stack,(void*)(long)j);
		pop(ev_stack);
	}

	return NULL;
}

void * thr_TEST(void *param)
{
	int i = 0;
	int j= 0;
	int fd = 0;
	ev_stack_type ev_stack = (ev_stack_type)param;
	for (i=0;i<1024*1024;i++,j++) {
		push(ev_stack,(void*)(long)j);
	}
	for (i=0;i<1024*1024;i++) {
		fd = (int)(long)pop(ev_stack);
		printf("THREAD_TEST:%d:%d\n",i,fd);//fflush(stdout);
	}

	return NULL;
}

static void test_main(CuTest *tc)
{
	int i = 0;
	pthread_t t0, t1, t2, t3, t4;
	void *retptr = NULL;

	ev_stack_type ev_stack = NULL;

	ev_stack = create_ev_stack();

	// /*
	pthread_create(&t1, NULL, thr1, ev_stack);
	pthread_create(&t2, NULL, thr2, ev_stack);
	pthread_create(&t3, NULL, thr3, ev_stack);
	pthread_create(&t4, NULL, thr4, ev_stack);
	// */

	//pthread_create(&t0,NULL,thr_TEST,NULL);

	 ///*
	pthread_join(t1,&retptr);
	pthread_join(t2,&retptr);
	pthread_join(t3,&retptr);
	pthread_join(t4,&retptr);
	 //*/

	//pthread_join(t0,&retptr);
	{
		int fd = 0;
		while (NULL != pop(ev_stack)) {
			++i;
			printf("[MAIN]:%d:%d\n",i,fd);//fflush(stdout);
		}
	}

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
	return suite->failCount;
}

int main()
{
	return run_ev_test();
}
