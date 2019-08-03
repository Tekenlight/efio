#include <stdio.h>
#include <stdlib.h>
#include <ev_include.h>
#include <pthread.h>
#include <stdbool.h>
#include <CuTest.h>
#include <memory_buffer_list.h>


struct test_inp {
	memory_buffer_list * buf_list;
	long no;
};

void * producer(void * inp)
{
	long i = 0L, n = 0L;
	struct test_inp * inp_ptr = NULL;
	inp_ptr = (struct test_inp *)inp;
	char * buffer = NULL;

	n = inp_ptr->no;

	for (i=0; i < n; i++) {
		buffer = (char*)malloc(20);
		sprintf(buffer,"%ld",i);
		inp_ptr->buf_list->add_node((void*)buffer, 100);
		//printf("Added %ld th\n",i);
	}

	return NULL;
}

void * consumer(void * inp)
{
	char * ret_str = NULL;
	memory_buffer_list::node * np = 0;
	long i = 0L, n = 0L, j= 0L;
	struct test_inp * inp_ptr = NULL;
	inp_ptr = (struct test_inp *)inp;
	n = inp_ptr->no;

	while (i < n) {
		np = inp_ptr->buf_list->pop_head();
		if (np) {
			//printf("Got j=%ld, n=%ld\n",(long)np->get_buffer(), n);

			// This condition is a test case, which ensures the 
			// sequential nature of the queue.
			if (i != atol((char*)(np->get_buffer()))) ret_str = (char *)"FAILED";
			i++;
			delete np;
		}
	}
	return ret_str;
}

static void test_main(CuTest *tc)
{
	struct test_inp inp;
	pthread_t t1, t2;
	void * retptr = NULL;
	char * retstr = NULL;

	inp.no = 1000000;
	inp.buf_list = new memory_buffer_list();

	pthread_create(&t1, NULL, producer, &inp);
	pthread_create(&t2, NULL, consumer, &inp);

	pthread_join(t1, &retptr);
	pthread_join(t2, (void**)&retstr);

	CuAssertTrue(tc, (NULL == inp.buf_list->get_head()));
	CuAssertTrue(tc, (NULL == retstr));

	delete inp.buf_list;

	return;
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

int main(int argc, char* argv[])
{
	return run_ev_test();
}
