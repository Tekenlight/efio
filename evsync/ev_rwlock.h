#ifndef EV_RWLOCK_H_INCLUDED
#define EV_RWLOCK_H_INCLUDED


#include <sys/cdefs.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <ev_include.h>

typedef struct ev_rwlock_s* ev_rwlock_type;

__BEGIN_DECLS
int ev_rwlock_rdlock(ev_rwlock_type s);
int ev_rwlock_rdunlock(ev_rwlock_type s);
int ev_rwlock_wrlock(ev_rwlock_type s);
int ev_rwlock_wrunlock(ev_rwlock_type s);
ev_rwlock_type ev_rwlock_init();
void ev_rwlock_destroy(ev_rwlock_type s);
__END_DECLS

#endif
