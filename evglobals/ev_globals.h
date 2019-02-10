#ifndef EV_GLOBALS_H_INCLUDED
#define EV_GLOBALS_H_INCLUDED

#include <sys/types.h>
#include <sys/cdefs.h>


__BEGIN_DECLS

void ev_init_globals();
size_t get_sys_pagesize();

__END_DECLS


#endif
