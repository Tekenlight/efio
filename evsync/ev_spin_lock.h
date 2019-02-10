#ifndef EV_MCS_LOCK_H_INCLUDED
#define EV_MCS_LOCK_H_INCLUDED


#include <stdlib.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <pthread.h>
#include <ev.h>

typedef atomic_bool spin_lock_type;


#define ev_spin_lock(s)  { \
	bool old_val = false; \
	old_val = atomic_load_explicit(s,memory_order_relaxed); \
	while (1) { \
		if (!old_val) { \
			if (atomic_compare_exchange_strong_explicit(s,&old_val,true, \
						memory_order_relaxed,memory_order_relaxed)) \
				break; \
		} \
		__asm__("NOP"); \
		old_val = atomic_load_explicit(s,memory_order_relaxed); \
	} \
}

#define ev_spin_unlock(s) {\
	bool old_val = true; \
	while (true == atomic_load(s)) { \
	atomic_compare_exchange_strong_explicit(s,&old_val,false, \
			memory_order_relaxed,memory_order_relaxed); \
	}\
}

__BEGIN_DECLS
int ev_spin_try_lock(spin_lock_type *s);
__END_DECLS

#endif
