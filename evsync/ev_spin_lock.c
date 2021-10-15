#include <ev_spin_lock.h>

int ev_spin_try_lock(spin_lock_type *s)
{
	bool old_val = false;
	int ret = 0;
	old_val = atomic_load_explicit(s,memory_order_acquire);
	if (false == old_val) {
		if (atomic_compare_exchange_strong_explicit(s,&old_val,true,
					memory_order_seq_cst,memory_order_seq_cst)) ret = 1;
	}
	else {
		ret =  0;
	}
	return ret;
}
