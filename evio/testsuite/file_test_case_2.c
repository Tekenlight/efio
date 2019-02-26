#include "ef_io.h"
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <CuTest.h>
#include <sys/stat.h>
#include <string.h>
#include <ev_globals.h>

static int open_cb_happened[2] = {0,0};
static int read_cb_happened[2] = {0,0};
static int close_cb_happened[2] = {0,0};

void call_back_func(int fd, int oper,  void *cb_data)
{
	char * operation = NULL;
	switch (oper) {
		case 0:
			operation = "OPEN";
			open_cb_happened[fd] = 1;
			break;
		case 1:
			read_cb_happened[fd] = 1;
			operation = "READ";
			break;
		case 2:
			close_cb_happened[fd] = 1;
			operation = "CLOSE";
			break;

	}
	printf("File %s complete and ready for %d\n",operation,fd);
	return;
}

static int all_cb_happened()
{
	int val = 0;
	val = (	(open_cb_happened[0] == 1 && open_cb_happened[1] == 1) &&
			(read_cb_happened[0] == 1 && read_cb_happened[1] == 0) &&
			(close_cb_happened[0] == 1 && close_cb_happened[1] == 1));
	return val;
}

static void test_main(CuTest *tc)
{
	int retval = 0;
	int i = 0;
	int count = 0;
	int r_fd = -1;
	int w_fd = 0;
	int status = 0;

	struct timespec tp1, tp2;

	uint64_t	t1 = 0, t2 = 0;
	uint64_t	t3 = 0, t4 = 0;
	
	long long LT = 0;

	struct stat rf_data, wf_data;

	memset(&tp1,0,sizeof(struct timespec));
	memset(&tp2,0,sizeof(struct timespec));
	memset(&rf_data,0,sizeof(struct stat));
	memset(&wf_data,0,sizeof(struct stat));

	char data[10000] = {};

	ef_init();

	ef_set_cb_func(call_back_func,NULL);

	retval = system("rm -f writing_file");
	retval = system("cp tmpl writing_file");
	t1 = clock_gettime_nsec_np(CLOCK_REALTIME);
	r_fd = ef_open("reading_file",O_RDONLY);
	w_fd = ef_open("writing_file",O_WRONLY|O_CREAT|O_APPEND, S_IRUSR+S_IWUSR+S_IRGRP+S_IROTH);

	status = ef_open_status(r_fd);
	while (-1 == status && errno == EAGAIN) {
		usleep(1);
		status = ef_open_status(r_fd);
	}
	if (status == -1) {
		EV_ABORT("Some error");
	}

	status = ef_open_status(w_fd);
	while (-1 == status && errno == EAGAIN) {
		usleep(1);
		status = ef_open_status(w_fd);
	}
	if (status == -1) {
		EV_ABORT("Some error");
	}

	int ret = 0;
	int j = 0;
	while (1) {
		j++;
		ret = 0;
		while ((0>(ret = ef_read(r_fd,data,9999))) && errno == EAGAIN) {
		}
		if (!ret) break;
		ef_write(w_fd,data,ret);
		memset(data,0,4097);
	}
	

	t2 = clock_gettime_nsec_np(CLOCK_REALTIME);

	ef_close_immediate(w_fd);
	ef_close_immediate(r_fd);

	stat("writing_file",&wf_data);
	CuAssertTrue(tc,(wf_data.st_size == 5352882));
	CuAssertTrue(tc,(1 == all_cb_happened()));
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

