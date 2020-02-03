#include <stdatomic.h>
#include <assert.h>
#include <ev_rwlock.h>


struct ev_rwlock_s {
	atomic_int rd_lock;
	atomic_int wr_lock;
};

ev_rwlock_type ev_rwlock_init()
{
	ev_rwlock_type s = malloc(sizeof (struct ev_rwlock_s));
	atomic_store(&(s->rd_lock), 0);
	atomic_store(&(s->wr_lock), 0);
	return s;
}

void ev_rwlock_destroy(ev_rwlock_type s)
{
	free(s);
}

int ev_rwlock_rdlock(ev_rwlock_type s)
{
	int wr = atomic_load(&(s->wr_lock));
	while (1) {
		if (0!=wr) {
			EV_YIELD();
			wr = atomic_load(&(s->wr_lock));
			continue;
		}
		else {
			atomic_fetch_add(&(s->rd_lock), 1);
			wr = atomic_load(&(s->wr_lock));
			if (0==wr) break;
			atomic_fetch_add(&(s->rd_lock), -1);
		}
	}
	return 0;
}

int ev_rwlock_wrlock(ev_rwlock_type s)
{
	int wr = 0;
	while (!atomic_compare_exchange_strong(&s->wr_lock, &wr, -1)) {
		wr = 0;
		EV_YIELD();
		continue;
	}
	/* Now no one else can make it 0 */
	int rd = atomic_load(&s->rd_lock);
	while (rd) {
		EV_YIELD();
		rd = atomic_load(&s->rd_lock);
	}
	atomic_store(&(s->wr_lock), 1);
	return 0;
}

int ev_rwlock_rdunlock(ev_rwlock_type s)
{
	int rd ;
	rd = atomic_load(&(s->rd_lock));
	assert((rd > 0));
	atomic_fetch_add(&(s->rd_lock), -1);
	return 0;
}

int ev_rwlock_wrunlock(ev_rwlock_type s)
{
	int wr = atomic_load(&(s->wr_lock));
	assert((wr==1));
	atomic_store(&(s->wr_lock), 0);
	return 0;
}

