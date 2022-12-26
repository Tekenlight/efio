#ifndef EV_MCS_LOCK_H_INCLUDED
#define EV_MCS_LOCK_H_INCLUDED


#include <stdlib.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <pthread.h>
#include <ev_include.h>
#include <stdint.h>


/*
 * Mellor Crummey and Scott presented a spinlock that avoids network contention by having
 * processors spin on local memory locations Their algorithm is equivalent to a lockfree queue
 * with a special access pattern The authors provide a complex and unintuitive proof of the
 * correctness of their algorithm
 */

typedef atomic_uintptr_t ev_mcs_lock_type;

__BEGIN_DECLS
/*
 * ### void mcs_init(ev_mcs_lock_type *p);
 * DESCRIPTION:
 *     Initializes the ev_mcs_lock_type input
 *
 * INPUT:
 *     ev_mcs_lock_type * p : lock variable
 *
 * RETURN:
 *     void
 */
void  mcs_init(ev_mcs_lock_type *);

/*
 * ### int mcs_relinquish(ev_mcs_lock_type *, void* );
 * DESCRIPTION:
 *     Relinquishes a previously acquired lock
 *
 * INPUT:
 *     ev_mcs_lock_type * : A lock that has been previously initialized using ***mcs_init***
 *     void * access: The ticket (access) that was returned when *** mcs_get_access *** was called
 *
 * RETURN:
 *     int : 0
 */
int mcs_relinquish(ev_mcs_lock_type *, void* );

/*
 * ### void * mcs_get_access(ev_mcs_lock_type *);
 * DESCRIPTION:
 *     Acquires the lock through mcs algorithm
 *     It will not be possible for another thread to lock while one thread has
 *     locked the structure.
 *     
 *     The operation does not return while the lock is unavailable
 *
 * INPUT:
 *     ev_mcs_lock_type *: A lock that has been previously initialized using ***mcs_init***
 *
 * RETURN:
 *     void * access: The ticket which the caller should return at the time of relinquishment
 */
void * mcs_get_access(ev_mcs_lock_type *);

__END_DECLS

#endif
