#include <ev_mcs_lock.h>

struct __s {
	atomic_int				locked;
	atomic_uintptr_t		next;
} ;

void  mcs_init(ev_mcs_lock_type *tail)
{
	*tail = 0;
}

void * mcs_get_access(ev_mcs_lock_type *tail)
{
	struct __s	*ptr = NULL;
	uintptr_t	pred = 0;

	ptr = malloc(sizeof(struct __s));
	ptr->next = 0;
	ptr->locked = 0;

	pred = atomic_exchange(tail,(uintptr_t)ptr);
	if (pred) {
		uintptr_t locked = 1;
		// There is a predecessor => Lock is held by someone => The current pointer is locked.
		atomic_store_explicit(&(ptr->locked),1,memory_order_release);
		atomic_store_explicit(&(((struct __s *)pred)->next),(uintptr_t)ptr,memory_order_release);

		// Wait aslong as current pointer is locked.
		do {
			locked = atomic_load_explicit(&(ptr->locked),memory_order_acquire);
#ifdef TARGET_OS_OSX
		pthread_yield_np();
#elif defined _WIN32
		sleep(0);
#else
		pthread_yield();
#endif
		} while (locked);
	}

	return ptr;

}

int mcs_relinquish(ev_mcs_lock_type *tail, void* access)
{
	struct __s *	ptr = NULL;
	bool			flg = false;
	uintptr_t		possible_tail = 0;

	ptr = (struct __s *)access;
	possible_tail = (uintptr_t)ptr;
	if(!ptr) return -1;

	// If ptr->next is null and ptr is indeed the tail
	// Nothing else to be done
	if (!(ptr->next) &&
			atomic_compare_exchange_strong(tail,&possible_tail,0)) {
	}
	else {
		// Either ptr->next is not null
		// OR it will not be null, soon.

		// If ptr is not the tail,
		// wait as long as next ptr is unknown
		while(!atomic_load_explicit(&(ptr->next),memory_order_acquire)) {
#ifdef TARGET_OS_OSX
		pthread_yield_np();
#elif defined _WIN32
		sleep(0);
#else
		pthread_yield();
#endif
		}

		// When the successor waiting for lock is known
		// Unlock the successor's node.
		atomic_store_explicit(&(((struct __s *)(ptr->next))->locked),0,memory_order_release);
	}

	free(ptr);

	return 0;
}
