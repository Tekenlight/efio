#include <stdio.h>
#include <ev_include.h>
#include <ev_queue.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdatomic.h>
#include <assert.h>

struct ev_queue_s {
	atomic_uintptr_t	head;
	atomic_uintptr_t	tail;
	/*
	atomic_long			count;
	*/
};

struct __s {
	void					*data;
	atomic_uintptr_t		next;
} ;

ev_queue_type create_ev_queue()
{
	ev_queue_type newptr = NULL;
	newptr = malloc(sizeof(struct ev_queue_s));
	newptr->head = 0;
	newptr->tail = 0;

	return newptr;

}

void wf_destroy_ev_queue(ev_queue_type q_ptr)
{
	void * qe = NULL;

    while((qe = dequeue(q_ptr)));
		//free(qe);

	free(q_ptr);
}

void destroy_ev_queue(ev_queue_type q_ptr)
{
	void * qe = NULL;

    while((qe = dequeue(q_ptr)))
		free(qe);

	free(q_ptr);
}

void debug_queue(ev_queue_type q_ptr, print_qnode_func_type pf)
{
	struct __s * qe = NULL;

	qe = (struct __s *)(q_ptr->head);
	while (qe) {
		pf(qe->data);
		qe = (struct __s *)qe->next;
	}
}

int peek(struct ev_queue_s * q_ptr)
{
	uintptr_t t = 0;
	if (!q_ptr) {
		printf("[%s:%d] Passed q_ptr is null\n",__FILE__,__LINE__);
		abort();
	}
	t = atomic_load_explicit(&(q_ptr->head),memory_order_acquire);
	return (t != 0);
}

int try_dequeue(struct ev_queue_s * q_ptr , void ** retptrptr)
{
	uintptr_t	old_h =0;
	uintptr_t	new_h =0;
	uintptr_t	marked_new_h =0;
	uintptr_t	next = 0;
	void		*data = NULL;
	bool		flg = false;
	uintptr_t	old_t =0;

	*retptrptr = NULL;
	// List is empty if tail is null.
	if (!q_ptr) {
		printf("[%s:%d] Passed q_ptr is null\n",__FILE__,__LINE__);
		abort();
	}
	do {
		old_t = atomic_load_explicit(&(q_ptr->tail),memory_order_acquire);
		if (!old_t) {
			return 0;
		}

		old_h = atomic_load_explicit(&(q_ptr->head),memory_order_acquire);
		if (!old_h) {
			return 0;
		}
		if (!(1&old_h)) {
			new_h = old_h;
			new_h = (1|new_h);
			if (atomic_compare_exchange_strong_explicit(&(q_ptr->head),&old_h,new_h,memory_order_seq_cst,memory_order_seq_cst)) {
				marked_new_h = new_h;
				break;
			}
			else {
				return -1;
			}
		}
		else {
			return -1;
		}
		EV_YIELD();
	} while (true);

	new_h = old_h;
	next = atomic_load_explicit(&(((struct __s *)(new_h))->next),memory_order_acquire);

	old_t = atomic_load_explicit(&(q_ptr->tail),memory_order_acquire);
	// Assert at this point tail cannot be null.
	if (old_t == 0) { // Dont know what to do abort??
		return 0;
	}

	/* If next of the header is  null, the list has become empty. */
	if (0 == next) {
		bool flg = false;
		uintptr_t	new_h1 =0;
		new_h1 = new_h; // new_h1 to avaoid any side effect on new_h due to CAS below.
		flg = atomic_compare_exchange_strong_explicit(&(q_ptr->tail),&new_h1,0,memory_order_seq_cst,memory_order_seq_cst);
		// If old and new tails are not same, it means a new node has come into
		// existence.
		if (!flg) {
			// Repeat as long as next header is null.
			do {
				next = atomic_load_explicit(&(((struct __s *)(new_h))->next),memory_order_acquire);
				EV_YIELD();
			} while (!next);
		}
	}

	new_h = next;
	atomic_thread_fence(memory_order_acquire);
	if (!(1&old_h))
		old_h = (1|old_h);

	{
		uintptr_t oold_h = old_h;
		flg = atomic_compare_exchange_strong_explicit(&(q_ptr->head),&oold_h,new_h,memory_order_seq_cst,memory_order_seq_cst);
		if (!flg) {
			//EV_ABORT("");
		}
	}
	atomic_thread_fence(memory_order_acquire);
	old_h = (~1&old_h);

	// Now old_h has what we want.
	data = ((struct __s *)old_h)->data;
	free(((struct __s *)old_h));
	*retptrptr = data;
	return 0;
}

void * dequeue(struct ev_queue_s * q_ptr)
{
	uintptr_t	old_h =0;
	uintptr_t	new_h =0;
	uintptr_t	marked_new_h =0;
	uintptr_t	next = 0;
	void		*data = NULL;
	bool		flg = false;
	uintptr_t	old_t =0;

	// List is empty if tail is null.
	if (!q_ptr) {
		STACK_TRACE();
		printf("[%s:%d] Passed q_ptr is null\n",__FILE__,__LINE__);
		abort();
	}

	do {
		old_t = atomic_load_explicit(&(q_ptr->tail),memory_order_acquire);
		if (!old_t) {
			return NULL;
		}
		old_h = atomic_load_explicit(&(q_ptr->head),memory_order_acquire);
		if (!old_h) {
			EV_YIELD();
			continue;
		}
		if (!(1&old_h)) {
			new_h = old_h;
			new_h = (1|new_h);
			if (atomic_compare_exchange_strong_explicit(&(q_ptr->head),&old_h,new_h,memory_order_seq_cst,memory_order_seq_cst)) {
				marked_new_h = new_h;
				break;
			}
		}
		EV_YIELD();
	} while (true);

	new_h = old_h;
	next = atomic_load_explicit(&(((struct __s *)(new_h))->next),memory_order_acquire);

	old_t = atomic_load_explicit(&(q_ptr->tail),memory_order_acquire);
	// Assert at this point tail cannot be null.
	assert(old_t != 0);
	if (old_t == 0) { // Dont know what to do abort??
		EV_DBG();
		return NULL;
	}

	/* If next of the header is  null, the list has become empty. */
	if (0 == next) {
		bool flg = false;
		uintptr_t	new_h1 =0;
		new_h1 = new_h; // new_h1 to avaoid any side effect on new_h due to CAS below.
		flg = atomic_compare_exchange_strong_explicit(&(q_ptr->tail),&new_h1,0,memory_order_seq_cst,memory_order_seq_cst);
		// If old and new tails are not same, it means a new node has come into
		// existence.
		if (!flg) {
			// Repeat as long as next header is null.
			do {
				next = atomic_load_explicit(&(((struct __s *)(new_h))->next),memory_order_acquire);
				EV_YIELD();
			} while (!next);
		}
	}

	new_h = next;
	atomic_thread_fence(memory_order_acq_rel);
	if (!(1&old_h))
		old_h = (1|old_h);

	{
		uintptr_t oold_h = old_h;
		flg = atomic_compare_exchange_strong_explicit(&(q_ptr->head),&oold_h,new_h,memory_order_seq_cst,memory_order_seq_cst);
		if (!flg) {
			// Is this fatal. If so, needs to be handled with assert
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
	atomic_thread_fence(memory_order_acq_rel);
	old_h = (~1&old_h);

	assert(old_h != 0);

	// Now old_h has what we want.
	data = ((struct __s *)old_h)->data;
	free(((struct __s *)old_h));
	return data;
}

int queue_empty(struct ev_queue_s * q_ptr)
{
	uintptr_t		tail = 0;
	if (!q_ptr) {
		printf("[%s:%d] Passed q_ptr is null\n",__FILE__,__LINE__);
		abort();
	}
	tail = atomic_load(&(q_ptr->tail));
	return (int)(tail == 0);
}

void enqueue(struct ev_queue_s * q_ptr,void * data)
{
	struct __s *	ptr = NULL;
	uintptr_t		old_tail = 0;
	uintptr_t		zero_head = 0;
	bool			flg = false;

	ptr = (struct __s*)malloc(sizeof(struct __s));
	ptr->data = data;
	ptr->next = (uintptr_t)0;

	old_tail = atomic_exchange_explicit(&(q_ptr->tail),(uintptr_t)ptr,memory_order_seq_cst);

	if (old_tail) {
		atomic_store_explicit(&(((struct __s *)old_tail)->next),
										(uintptr_t)ptr,memory_order_release);
	}
	else {
		atomic_exchange_explicit(&(q_ptr->head),(uintptr_t)ptr,memory_order_acq_rel);
	}

	return;
}


#ifdef NEVER


struct thr_inp {
	ev_queue_type pq_ptr;
	int N;
	int slot;
};

void * thr(void *param)
{
	struct thr_inp * inp;
	int i = 0;
	int j = 0;
	inp = param;

	ev_queue_type pq_ptr = (ev_queue_type)inp->pq_ptr;;
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
		enqueue(pq_ptr,(void*)(long)j);
		//enqueue(pq_ptr,(void*)(long)j);
		//usleep(1);
		dequeue(pq_ptr);
		//dequeue(pq_ptr);
		//j++;i++;
		//printf("THREAD1:0%d:%d\n",i,(int)dequeue(pq_ptr));fflush(stdout);
	}

	/*
	for (i=0;i<COUNT;i++,j++) {
		//printf("THREAD1:0%d:%d\n",i,j);//fflush(stdout);
		//enqueue(pq_ptr,(void*)(long)j);
		//usleep(1);
		dequeue(pq_ptr);
		//printf("THREAD1:0%d:%d\n",i,(int)dequeue(pq_ptr));fflush(stdout);
	}
	*/

	//printf("THR1 OVER\n");

	return NULL;
}

void * thr_TEST(void *param)
{
	int i = 0; int j= 0;
	int fd = 0;
	ev_queue_type pq_ptr = (ev_queue_type)param;
	for (i=0;i<128;i++,j++) {
		enqueue(pq_ptr,(void*)(long)j);
		fd = (int)(long)dequeue(pq_ptr);
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

	ev_queue_type q;

	q = create_ev_queue();

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
		while (NULL != (p = dequeue(q))) {
			++i;
			fd = (int)(long)p;
			printf("[MAIN]:%d:%d\n",i,fd);//fflush(stdout);
		}
	}


	//destroy_ev_queue(&q);

	return 0;
}

#endif
