#include <istream>
#include <stdio.h>
#include <stdlib.h>
#include <ev_include.h>
#include <pthread.h>
#include <stdbool.h>
#include <CuTest.h>
#include <string.h>
#include <ev_buffered_stream.h>


void test_main_1(CuTest *tc)
{
	chunked_memory_stream cms;
	ev_buffered_stream buf(&cms, 1024);
	char * a = NULL;
	char * b = NULL;
	char * c = NULL;
	char * d = NULL;
	char s[20] = {'\0'};
	char s1[20] = {'\0'};

	puts("test case 2");
	std::ostream os(&buf);
	os << "01";
	os << "23";
	os << "45";
	os << "67";
	buf.sync();

	*s1 = '\0';
	std::istream is(&buf);
	int p = 0;
	//printf("First char = %c\n",buf.sgetc());
	while (EOF != (p = is.get())) {
		s1[strlen(s1)+1] = '\0';
		s1[strlen(s1)] = (char)(int)p;
	}
	puts(s1);

	CuAssertTrue(tc, (!strcmp(s1,"01234567")));
}

void test_main(CuTest *tc)
{
	chunked_memory_stream cms;
	ev_buffered_stream buf(&cms, 1024, 4);
	char * a = NULL;
	char * b = NULL;
	char * c = NULL;
	char * d = NULL;
	char s[20] = {'\0'};
	char s1[20] = {'\0'};

	a = (char*)calloc(1, 2);
	b = (char*)calloc(1, 2);
	c = (char*)calloc(1, 2);
	d = (char*)calloc(1, 2);

	memcpy(a, "01", 2);
	cms.push(a, 2);

	memcpy(b, "23", 2);
	cms.push(b, 2);

	memcpy(c, "45", 2);
	cms.push(c, 2);

	memcpy(d, "67", 2);
	cms.push(d, 2);

	int p = 0;
	//printf("First char = %c\n",buf.sgetc());
	while (EOF != (p = buf.sbumpc())) {
		s[strlen(s)+1] = '\0';
		s[strlen(s)] = (char)(int)p;
	}
	//CuAssertTrue(tc, (!strcmp(s,"01234567")));
	puts(s);

	*s1 = '\0';
	ev_buffered_stream buf1(&cms, 1024, 4);
	std::istream is(&buf1);
	p = 0;
	//printf("First char = %c\n",buf.sgetc());
	while (EOF != (p = is.get())) {
		s1[strlen(s1)+1] = '\0';
		s1[strlen(s1)] = (char)(int)p;
	}
	puts(s1);

	CuAssertTrue(tc, (!strcmp(s,"0123") && !strcmp(s1,"4567")));
}

static int run_ev_test()
{
    CuSuite* suite = CuSuiteNew();
    CuString *output = CuStringNew();

    SUITE_ADD_TEST(suite,test_main);
    SUITE_ADD_TEST(suite,test_main_1);

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
