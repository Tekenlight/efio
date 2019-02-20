#include <stdio.h>
#ifdef NEVER
#include <stdio.h>
#endif

#include <ev_include.h>
#include <ev_paqueue.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdatomic.h>

struct qn_s {
	atomic_uintptr_t	head;
	atomic_uintptr_t	tail;
};

struct ev_paqueue_s {
	int						N;
	struct qn_s				*qa;
	atomic_int				enq_counter;
	atomic_int				deq_counter;
};

struct __s {
	void					*data;
	atomic_uintptr_t		next;
};

static int get_nearest_power_of_2(int n)
{
	int two_to_n = 1;

	int i = 0;
	for (i=0;i<32;i++) {
		if (two_to_n >= n) break;
		two_to_n <<= 1;
	}
	return two_to_n;
}

ev_paqueue_type create_evpaq(int n)
{
	ev_paqueue_type newptr = NULL;
	newptr = malloc(sizeof(struct ev_paqueue_s));
	newptr->N = get_nearest_power_of_2(n);
	newptr->qa = malloc(newptr->N*sizeof(struct qn_s));
	for (int index = 0; index < newptr->N; index++) {
		newptr->qa[index].head = 0;
		newptr->qa[index].tail = 0;
	}
	newptr->enq_counter = 0;
	newptr->deq_counter = 0;

	return newptr;
}

void wf_destroy_evpaq(ev_paqueue_type *q_ptr_ptr)
{
	void * qe = NULL;

    while((qe = dequeue_evpaq(*q_ptr_ptr)));

	free(*q_ptr_ptr);
	*q_ptr_ptr = NULL;
}


void destroy_evpaq(ev_paqueue_type *q_ptr_ptr)
{
	void * qe = NULL;

    while((qe = dequeue_evpaq(*q_ptr_ptr)))
		free(qe);

	free(*q_ptr_ptr);
	*q_ptr_ptr = NULL;
}

void debug_paq(ev_paqueue_type q_ptr, print_paqnode_func_type pf)
{
	struct __s * qe = NULL;

	for (int index = 0; index < q_ptr->N; index ++) {
		qe = (struct __s *)(q_ptr->qa[index].head);
		while (qe) {
			pf(index,qe->data);
			qe = (struct __s *)qe->next;
		}
	}
}

int peek_evpaq(struct ev_paqueue_s * q_ptr)
{
	uintptr_t t = 0;
	if (!q_ptr) {
		printf("[%s:%d] Passed q_ptr is null\n",__FILE__,__LINE__);
		abort();
	}
	
	for (int index = 0; index < q_ptr->N; index ++) {
		t = atomic_load_explicit(&(q_ptr->qa[index].head),memory_order_relaxed);
		if (t) break;
	}

	return (t != 0);
}

void * dequeue_evpaq(struct ev_paqueue_s * q_ptr)
{
	uintptr_t	old_h =0;
	uintptr_t	new_h =0;
	uintptr_t	marked_new_h =0;
	uintptr_t	next = 0;
	void		*data = NULL;
	bool		flg = false;
	uintptr_t	old_t =0;
	int try_count = 0;
	int index = 0;
	int d_count = 0;

	// List is empty if tail is null.
	if (!q_ptr) {
		printf("[%s:%d] Passed q_ptr is null\n",__FILE__,__LINE__);
		abort();
	}

	d_count = atomic_fetch_add(&(q_ptr->deq_counter),1);
	d_count--;
	while (try_count < q_ptr->N) {
		d_count++;
		index = d_count % q_ptr->N;
		++try_count;
		old_t = atomic_load_explicit(&(q_ptr->qa[index].tail),memory_order_relaxed);
		if (!old_t) {
			//pthread_yield_np();
			continue;	
		}
		/* Here we know that the queue is not empty. */
		try_count = 0;

		{
			old_h = atomic_load_explicit(&(q_ptr->qa[index].head),memory_order_relaxed);
			if (!old_h) {
				pthread_yield_np();
				continue;
			}
			if (!(1&old_h)) {
				new_h = old_h;
				new_h = (1|new_h);
				if (atomic_compare_exchange_strong_explicit(&(q_ptr->qa[index].head),&old_h,new_h,
														memory_order_relaxed,memory_order_relaxed)) {
					marked_new_h = new_h;
					//try_count = 0;
					break;
				}
			}
			pthread_yield_np();
		}
	}

	if (try_count >= q_ptr->N) return NULL;

	new_h = old_h;
	next = atomic_load_explicit(&(((struct __s *)(new_h))->next),memory_order_relaxed);

	old_t = atomic_load_explicit(&(q_ptr->qa[index].tail),memory_order_relaxed);
	// Assert at this point tail cannot be null.
	if (old_t == 0) { // Dont know what to do abort??
		return NULL;
	}

	/* If next of the header is  null, the list has become empty. */
	if (0 == next) {
		bool flg = false;
		uintptr_t	new_h1 =0;
		new_h1 = new_h; // new_h1 to avaoid any side effect on new_h due to CAS below.
		flg = atomic_compare_exchange_strong_explicit(&(q_ptr->qa[index].tail),&new_h1,0,memory_order_relaxed,memory_order_relaxed);
		// If old and new tails are not same, it means a new node has come into
		// existence.
		if (!flg) {
			// Repeat as long as next header is null.
			do {
				next = atomic_load_explicit(&(((struct __s *)(new_h))->next),memory_order_relaxed);
#ifdef TARGET_OS_OSX
		pthread_yield_np();
#elif defined _WIN32
		sleep(0);
#else
		pthread_yield();
#endif
			} while (!next);
		}
	}

	new_h = next;
	atomic_thread_fence(memory_order_relaxed);
	if (!(1&old_h))
		old_h = (1|old_h);

	{
		uintptr_t oold_h = old_h;
		flg = atomic_compare_exchange_strong_explicit(&(q_ptr->qa[index].head),&oold_h,new_h,memory_order_relaxed,memory_order_relaxed);
		if (!flg) {
			// Is this fatal. If so, needs to be handled with assert
			// It is not fatal.
			//
			// Or is it OK, even if it fails, it can happen because
			// while we have prevented other threads from deleting,
			// one of the threads has changed the head to something else
			// as part of the enqueue process, this will happen becuase
			// we would have set the tail to 0 and the enqueueing process
			// upon seeing null tail, will assume that the list is empty.
			// Hence our exchange will not go throug.
			//
			//return NULL;
		}
	}
	atomic_thread_fence(memory_order_relaxed);
	old_h = (~1&old_h);

	// Now old_h has what we want.
	data = ((struct __s *)old_h)->data;
	free(((struct __s *)old_h));
	return data;
}

void enqueue_evpaq(struct ev_paqueue_s * q_ptr,void * data)
{
	struct __s *	ptr = NULL;
	uintptr_t		old_tail = 0;
	uintptr_t		zero_head = 0;
	bool			flg = false;
	int				index = 0;
	int				e_count = 0;

	ptr = (struct __s*)malloc(sizeof(struct __s));
	ptr->data = data;
	ptr->next = (uintptr_t)0;

	e_count = atomic_fetch_add(&(q_ptr->enq_counter),1);
	index = e_count % q_ptr->N;
	old_tail = atomic_exchange_explicit(&(q_ptr->qa[index].tail),(uintptr_t)ptr,memory_order_relaxed);

	if (old_tail) {
		atomic_store_explicit(&(((struct __s *)old_tail)->next),
										(uintptr_t)ptr,memory_order_relaxed);
	}
	else {
		atomic_exchange_explicit(&(q_ptr->qa[index].head),(uintptr_t)ptr,memory_order_relaxed);
	}

	return;
}


#ifdef NEVER


struct thr_inp {
	ev_paqueue_type pq_ptr;
	int N;
	int slot;
};

void * thr(void *param)
{
	struct thr_inp * inp;
	int i = 0;
	int j = 0;
	inp = param;

	ev_paqueue_type pq_ptr = (ev_paqueue_type)inp->pq_ptr;;
	int N = inp->N;
	int slot = inp->slot;
	int COUNT = N/4;

	if (slot == 5) {
		j = 0;
		COUNT = N;
	}
	else {
		j = slot*N/4;
		COUNT = N/4;
	}

	for (i=0;i<COUNT;i++,j++) {
		//printf("THREAD1:0%d:%d\n",i,j);//fflush(stdout);
		enqueue_evpaq(pq_ptr,(void*)(long)j);
		//enqueue_evpaq(pq_ptr,(void*)(long)j);
		//usleep(1);
		dequeue_evpaq(pq_ptr);
		//dequeue_evpaq(pq_ptr);
		//j++;i++;
		//printf("THREAD1:0%d:%d\n",i,(int)dequeue_evpaq(pq_ptr));fflush(stdout);
	}

	/*
	for (i=0;i<COUNT;i++,j++) {
		//printf("THREAD1:0%d:%d\n",i,j);//fflush(stdout);
		//enqueue_evpaq(pq_ptr,(void*)(long)j);
		//usleep(1);
		dequeue_evpaq(pq_ptr);
		//printf("THREAD1:0%d:%d\n",i,(int)dequeue_evpaq(pq_ptr));fflush(stdout);
	}
	*/

	//printf("THR1 OVER\n");

	return NULL;
}

void * thr_TEST(void *param)
{
	int i = 0; int j= 0;
	int fd = 0;
	ev_paqueue_type pq_ptr = (ev_paqueue_type)param;
	for (i=0;i<128;i++,j++) {
		enqueue_evpaq(pq_ptr,(void*)(long)j);
		fd = (int)(long)dequeue_evpaq(pq_ptr);
		printf("THREAD_TEST:%d:%d\n",i,fd);fflush(stdout);
	}

	/*
	for (i=0;i<128;i++) {
		printf("THREAD_TEST:%d:%d\n",i,fd);//fflush(stdout);
	}
	*/
	return NULL;
}

int main()
{
	int i = -1, fd = 0;;
	pthread_t t0,t1, t2, t3, t4, t5;
	void *retptr = NULL;
	struct thr_inp inp1 ;
	struct thr_inp inp2 ;
	struct thr_inp inp3 ;
	struct thr_inp inp4 ;
	struct thr_inp inp5 ;

	ev_paqueue_type q;

	q = create_evpaq(4);

	inp1.pq_ptr = q;
	inp1.N = 4*1024*1024;
	inp2.pq_ptr = q;
	inp2.N = 4*1024*1024;
	inp3.pq_ptr = q;
	inp3.N = 4*1024*1024;
	inp4.pq_ptr = q;
	inp4.N = 4*1024*1024;
	inp5.pq_ptr = q;
	inp5.N = 4*1024*1024;

	/*
	pthread_create(&t0,NULL,thr_TEST,q);
	*/
	inp1.slot = 0;
	pthread_create(&t1, NULL, thr, &inp1);
	inp2.slot = 1;
	pthread_create(&t2, NULL, thr, &inp2);
	inp3.slot = 2;
	pthread_create(&t3, NULL, thr, &inp3);
	inp4.slot = 3;
	pthread_create(&t4, NULL, thr, &inp4);

	/*
	inp5.slot = 5;
	pthread_create(&t5, NULL, thr, &inp5);
	*/
	pthread_join(t0,&retptr);
	pthread_join(t1,&retptr);
	pthread_join(t2,&retptr);
	pthread_join(t3,&retptr);
	pthread_join(t4,&retptr);

	/*
	pthread_join(t5,&retptr);
	*/
	//printf("HELLO\n");

	{
		void * p;
		int i = 0;
		while (NULL != (p = dequeue_evpaq(q))) {
			++i;
			fd = (int)(long)p;
			printf("[MAIN]:%d:%d\n",i,fd);//fflush(stdout);
		}
	}


	//destroy_ev_queue(&q);

	return 0;
}

#endif
