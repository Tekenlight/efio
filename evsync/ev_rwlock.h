#ifndef EV_RWLOCK_H_INCLUDED
#define EV_RWLOCK_H_INCLUDED


#include <sys/cdefs.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <ev_include.h>

typedef struct ev_rwlock_s* ev_rwlock_type;

__BEGIN_DECLS

/*
 * ### ev_rwlock_type ev_rwlock_init(ev_rwlock_type s);
 * DESCRIPTION:
 *     Allocates and initializes a new rw-lock artifact
 *
 * INPUT:
 *     void
 *
 * RETURN:
 *     ev_rwlock_type : A new rw-lock artifact.
 */
ev_rwlock_type ev_rwlock_init();

/*
 * ### void ev_rwlock_destroy(ev_rwlock_type s);
 * DESCRIPTION:
 *     Clears up and frees a previously created lock.
 *
 * INPUT:
 *     ev_rwlock_type s : A lock that has been previously created using ev_rwlock_init
 *
 * RETURN:
 *     void
 */
void ev_rwlock_destroy(ev_rwlock_type s);

/*
 * ### int ev_rwlock_rdlock(ev_rwlock_type s);
 * DESCRIPTION:
 *     Sets the read lock in the given input rw-lock. This operation prevents
 *     other threads from locking the same rw-lock for write operation, while 
 *     other threads wanting a read permission on the same articat are still allowed.
 *     Read lock cannot be obtained while another thread holds a write lock.
 *     
 *     The operation spins until the read lock is attained
 *
 * INPUT:
 *     ev_rwlock_type s: An rw-lock artifact created through ***ev_rwlock_init***
 *
 * RETURN:
 *     0 indicating successful acquisition of lock
 */
int ev_rwlock_rdlock(ev_rwlock_type s);

/*
 * ### int ev_rwlock_rdunlock(ev_rwlock_type s);
 * DESCRIPTION:
 *     Unsets the read lock (releases the lock) in the given input rw-lock.
 *     If this is the last thread releasing the lock and if a thread is waiting for write lock
 *     the waiting thread will acquire write lock after this operation.
 *     
 * INPUT:
 *     ev_rwlock_type s: An rw-lock artifact created through ***ev_rwlock_init***
 *
 * RETURN:
 *     0 indicating successful acquisition of lock
 */
int ev_rwlock_rdunlock(ev_rwlock_type s);

/*
 * ### int ev_rwlock_wrlock(ev_rwlock_type s);
 * DESCRIPTION:
 *     Sets the write lock in the given rw-lock.
 *     If there are other threads having read/write lock then this operation blocks.
 *     While one thread is attempting to acquire a write lock, other threads will be prevented
 *     from further acquiring read/write lock.
 *     
 *     The operation spins until the read lock is attained
 *
 * INPUT:
 *     ev_rwlock_type s: An rw-lock artifact created through ***ev_rwlock_init***
 *
 * RETURN:
 *     0 indicating successful acquisition of lock
 */
int ev_rwlock_wrlock(ev_rwlock_type s);

/*
 * ### int ev_rwlock_wrunlock(ev_rwlock_type s);
 * DESCRIPTION:
 *     Releases the previously acquired write lock
 *     This will unblock other threads waiting to acquire read/write lock
 *     
 *     The operation spins until the read lock is attained
 *
 * INPUT:
 *     ev_rwlock_type s: An rw-lock artifact created through ***ev_rwlock_init***
 *
 * RETURN:
 *     0 indicating successful acquisition of lock
 */
int ev_rwlock_wrunlock(ev_rwlock_type s);

__END_DECLS

#endif
