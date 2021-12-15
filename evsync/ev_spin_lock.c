#include <stdatomic.h>
#include <ev_spin_lock.h>

struct spin_lock {
	atomic_bool _lock;
};


spin_lock_p_type create_spin_lock()
{
	spin_lock_p_type sp = (spin_lock_p_type)malloc(sizeof(struct spin_lock));
	sp->_lock = false;

	return sp;
}


int ev_spin_try_lock(spin_lock_p_type s)
{
	bool old_val = false;
	int ret = 0;
	old_val = atomic_load_explicit(&(s->_lock),memory_order_acquire);
	if (false == old_val) {
		if (atomic_compare_exchange_strong_explicit(&(s->_lock),&old_val,true,
					memory_order_seq_cst,memory_order_seq_cst)) ret = 1;
	}
	else {
		ret =  0;
	}
	return ret;
}

void ev_spin_lock(spin_lock_p_type  s)
{
	bool old_val = false;
	old_val = atomic_load_explicit(&(s->_lock),memory_order_acquire);
	while (1) {
		if (!old_val) {
			if (atomic_compare_exchange_strong_explicit(&(s->_lock),&old_val,true,
						memory_order_seq_cst,memory_order_seq_cst))
				break;
		}
		__asm__("NOP");
		old_val = atomic_load_explicit(&(s->_lock),memory_order_acquire);
	}
	return;
}

void ev_spin_unlock(spin_lock_p_type  s)
{
	bool old_val = true;
	while (true == atomic_load(&(s->_lock))) {
	atomic_compare_exchange_strong_explicit(&(s->_lock),&old_val,false,
			memory_order_seq_cst,memory_order_seq_cst);
	}
	return;
}

