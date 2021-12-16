#ifndef EV_MCS_LOCK_H_INCLUDED
#define EV_MCS_LOCK_H_INCLUDED


#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <ev_include.h>

typedef struct spin_lock spin_lock_s_type;
typedef struct spin_lock *spin_lock_p_type;


__BEGIN_DECLS
spin_lock_p_type create_spin_lock();
void destroy_spin_lock(spin_lock_p_type);
void ev_spin_lock(spin_lock_p_type s);
void ev_spin_unlock(spin_lock_p_type s);
int ev_spin_try_lock(spin_lock_p_type s);
__END_DECLS

#endif
