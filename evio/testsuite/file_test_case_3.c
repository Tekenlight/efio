#include "ef_io.h"
#include <stdio.h>
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

static void test_main(CuTest *tc)
{
	char str[10000] = {'\0'};
	int read_fd[100] = {0};
	int write_fd[100] = {0};
	struct stat rf_data, wf_data;

	memset(&wf_data,0,sizeof(struct stat));

	system("rm -f writing_file");
	system("rm -Rf output");
	system("mkdir output");

	ef_init();
	ef_set_cb_func(call_back_func,NULL);
	for (int i = 0; i < 50 ; i++) {
		char file_name[200] = {'\0'};
		sprintf(file_name,"test/%d_reading_file",i);
		read_fd[i] = ef_open(file_name,O_RDONLY);
		sprintf(file_name,"output/%d_writing_file",i);
		write_fd[i] = ef_open(file_name,O_RDWR|O_CREAT|O_APPEND,0644);
	}

	for (int i = 0; i < 50; i++) {
		int ret = 0;
		while ((-1 == (ret =  ef_open_status(read_fd[i]))) && errno == EAGAIN);
		if (-1 == ret) { perror("SOMETHING WENTN WRONG"); EV_DBGP("i = %d\n",i); abort(); }
		while ((-1 == (ret =  ef_open_status(write_fd[i]))) && errno == EAGAIN);
		if (-1 == ret) { perror("SOMETHING WENTN WRONG"); EV_DBG(); abort(); }
	}

	for (int i = 0; i < 50; i++) {
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

