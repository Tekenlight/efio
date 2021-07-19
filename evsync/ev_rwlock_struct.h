#ifndef EV_RWLOCK_STRUCT_H_INCLUDED
#define EV_RWLOCK_STRUCT_H_INCLUDED


#include <stdatomic.h>


struct ev_rwlock_s {
	atomic_int rd_lock;
	atomic_int wr_lock;
};


#define EV_RW_LOCK_S_INIT(l) {\
	atomic_store(&((l).rd_lock), 0);\
	atomic_store(&((l).wr_lock), 0);\
}


#endif
