#include <unistd.h>
#include <ev_globals.h>
#include <time.h>

int _ev_sys_page_size;

static int _ev_init_done = 0;

void ev_init_globals()
{
	if (_ev_init_done) return;
	_ev_init_done = 1;
	_ev_sys_page_size = (int)sysconf(_SC_PAGESIZE);
	return ;
}

size_t get_sys_pagesize()
{
	return _ev_sys_page_size;
}

#ifndef __APPLE__
uint64_t clock_gettime_nsec_np(clockid_t clock_id)
{
	struct timespec t;
	int ret = 0;
	uint64_t return_value = 0;
	ret = clock_gettime(clock_id, &t);
	return_value = (uint64_t)(t.tv_sec * 1000000000 + t.tv_nsec);

	return return_value;
}
#endif
