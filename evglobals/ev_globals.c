#include <unistd.h>
#include <ev_globals.h>

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
