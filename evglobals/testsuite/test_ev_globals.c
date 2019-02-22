#include <stdio.h>
#include <unistd.h>
#include <CuTest.h>
#include <ev_globals.h>

void page_size_before_init_case(CuTest *tc)
{
	int page_size = (int)sysconf(_SC_PAGESIZE);
	int got_from_func = get_sys_pagesize();
	CuAssertTrue(tc, 0 == got_from_func);
}

void page_size_case(CuTest *tc)
{
	ev_init_globals();
	int page_size = (int)sysconf(_SC_PAGESIZE);
	int got_from_func = get_sys_pagesize();
	CuAssertTrue(tc, page_size == got_from_func);
}

int run_ev_globals_test()
{
	CuSuite* suite = CuSuiteNew();
	CuString *output = CuStringNew();

	SUITE_ADD_TEST(suite,page_size_before_init_case);
	SUITE_ADD_TEST(suite,page_size_case);

	CuSuiteRun(suite);
    CuSuiteSummary(suite, output);
    CuSuiteDetails(suite, output);
    printf("%s\n", output->buffer);
	printf("Count = %d\n",suite->count);
	return suite->failCount;
}

int main()
{
	return run_ev_globals_test();
}
