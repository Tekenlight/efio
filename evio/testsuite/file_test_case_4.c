#include "ef_io.h"
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <CuTest.h>
#include <sys/stat.h>
#include <string.h>

void call_back_func(int fd, int oper,  void *cb_data)
{
	char * operation = NULL;
	switch (oper) {
		case 0:
			operation = "OPEN";
			break;
		case 1:
			operation = "READ";
			break;
		case 2:
			operation = "CLOSE";
			break;

	}
	//printf("File %s complete and ready for %d\n",operation,fd);
	return;
}

static void * thr(void *param)
{
	char str[10000] = {'\0'};
	int read_fd[100] = {0};
	int write_fd[100] = {0};
	struct stat rf_data, wf_data;

	int j = *(int*)(param);

	for (int i = j; i < j+10 ; i++) {
		char file_name[200] = {'\0'};
		sprintf(file_name,"test/%d_reading_file",i);
		read_fd[i] = ef_open(file_name,O_RDONLY);
		sprintf(file_name,"output/%d_writing_file",i);
		write_fd[i] = ef_open(file_name,O_RDWR|O_CREAT|O_APPEND,0644);
	}

	for (int i = j; i < j+10; i++) {
		int ret = 0;
		while ((-1 == (ret =  ef_open_status(read_fd[i]))) && errno == EAGAIN);
		if (-1 == ret) { perror("SOMETHING WENTN WRONG"); EV_DBGP("i = %d\n",i); abort(); }
		while ((-1 == (ret =  ef_open_status(write_fd[i]))) && errno == EAGAIN);
		if (-1 == ret) { perror("SOMETHING WENTN WRONG"); EV_DBG(); abort(); }
	}

	for (int i = j; i < j+10; i++) {
		while (1) {
			int ret = 0;
			while (-1 == (ret = ef_read(read_fd[i],str,9999)) && errno == EAGAIN);
			if (-1 == ret) { perror("SOMETHING WENTN WRONG"); EV_DBG(); abort(); }
			if (!ret) break;
			if (ret) ret = ef_write(write_fd[i], str, ret);
			if (-1 == ret) { perror("SOMETHING WENTN WRONG");EV_DBG(); abort(); }
		}
	}

	sleep(10);

	return NULL;
}

static void test_main(CuTest *tc)
{
	int retval = 0;
	char str[10000] = {'\0'};
	int read_fd[100] = {0};
	int write_fd[100] = {0};
	struct stat rf_data, wf_data;
	int i = 0, j = 10, k = 20, l = 30;
	pthread_t t0,t1, t2, t3, t4, t5;

	memset(&wf_data,0,sizeof(struct stat));

	retval = system("rm -f writing_file");
	retval = system("rm -Rf output");
	retval = system("mkdir output");

	ef_init();
	ef_set_cb_func(call_back_func,NULL);

	pthread_create(&t1, NULL, thr, &i);
	pthread_create(&t2, NULL, thr, &j);
	pthread_create(&t3, NULL, thr, &k);
	pthread_create(&t4, NULL, thr, &l);

	pthread_join(t1,NULL);
	pthread_join(t2,NULL);
	pthread_join(t3,NULL);
	pthread_join(t4,NULL);

	stat("output/3_writing_file",&wf_data);
	CuAssertTrue(tc,(wf_data.st_size == 5352871));
	stat("output/11_writing_file",&wf_data);
	CuAssertTrue(tc,(wf_data.st_size == 5352871));
	stat("output/25_writing_file",&wf_data);
	CuAssertTrue(tc,(wf_data.st_size == 5352871));
	stat("output/37_writing_file",&wf_data);
	CuAssertTrue(tc,(wf_data.st_size == 5352871));
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

