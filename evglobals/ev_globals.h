#ifndef EV_GLOBALS_H_INCLUDED
#define EV_GLOBALS_H_INCLUDED

#include  <stdint.h>
#include <sys/types.h>
#include <sys/cdefs.h>
#include <time.h>


__BEGIN_DECLS

void ev_init_globals();
size_t get_sys_pagesize();

#ifndef __APPLE__
uint64_t clock_gettime_nsec_np(clockid_t clock_id);
#endif
__END_DECLS


#endif
