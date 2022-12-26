#ifndef EV_MCS_LOCK_H_INCLUDED
#define EV_MCS_LOCK_H_INCLUDED


#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <ev_include.h>

typedef struct spin_lock spin_lock_s_type;
typedef struct spin_lock *spin_lock_p_type;


__BEGIN_DECLS
/*
 * ### spin_lock_p_type create_spin_lock(ev_rwlock_type s);
 * DESCRIPTION:
 *     Allocates and initializes a new spin-lock artifact
 *
 * INPUT:
 *     void
 *
 * RETURN:
 *     spin_lock_p_type : A new spin-lock artifact.
 */
spin_lock_p_type create_spin_lock();

/*
 * ### void destroy_spin_lock(spin_lock_p_type s);
 * DESCRIPTION:
 *     Clears up and frees a previously created spin-lock.
 *
 * INPUT:
 *     spin_lock_p_type s : A lock that has been previously created using create_spin_lock
 *
 * RETURN:
 *     void
 */
void destroy_spin_lock(spin_lock_p_type);

/*
 * ### int ev_spin_try_lock(spin_lock_p_type s);
 * DESCRIPTION:
 *     Sets the spin-lock structure as locked.
 *     It will not be possible for another thread to lock while one thread has
 *     locked the structure.
 *     
 *     The operation does not spin while the lock is unavailable, instead it returns
 *     immediately.
 *
 * INPUT:
 *     spin_lock_p_type s: An spin-lock artifact created through ***create_spin_lock***
 *
 * RETURN:
 *     0 indicating unsuccessful acquisition of lock
 *     1 indicating successful acquisition of lock
 */
int ev_spin_try_lock(spin_lock_p_type s);

/*
 * ### void ev_spin_lock(spin_lock_p_type s);
 * DESCRIPTION:
 *     Sets the spin-lock structure as locked.
 *     It will not be possible for another thread to lock while one thread has
 *     locked the structure.
 *     
 *     The operation does not return while the lock is unavailable
 *
 * INPUT:
 *     spin_lock_p_type s: An spin-lock artifact created through ***create_spin_lock***
 *
 * RETURN:
 *     void
 */
void ev_spin_lock(spin_lock_p_type s);

/*
 * ### void ev_spin_unlock(spin_lock_p_type s);
 * DESCRIPTION:
 *     Unsets the lock (releases the lock) in the given input spin-lock.
 *     
 * INPUT:
 *     spin_lock_p_type s: An spin-lock artifact created through ***create_spin_lock***
 *     and locked through either ev_spin_lock or ev_spin_try_lock
 *
 * RETURN:
 *     void
 */
void ev_spin_unlock(spin_lock_p_type s);
__END_DECLS

#endif
