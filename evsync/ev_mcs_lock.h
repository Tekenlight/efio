#ifndef EV_MCS_LOCK_H_INCLUDED
#define EV_MCS_LOCK_H_INCLUDED


#include <stdlib.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <pthread.h>
#include <ev_include.h>


typedef atomic_uintptr_t ev_mcs_lock_type;

__BEGIN_DECLS
void  mcs_init(ev_mcs_lock_type *);
void * mcs_get_access(ev_mcs_lock_type *);
int mcs_relinquish(ev_mcs_lock_type *, void* );
__END_DECLS

#endif
