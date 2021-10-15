#include <ev_include.h>
#include <ev_queue.h>
#include <ev_piqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>


struct ev_list_s {
	atomic_uintptr_t	head;
	atomic_uintptr_t	tail;
};

struct ev_pqueue_gl_s {
	struct ev_list_s list;
	atomic_int e_guard;
	atomic_int d_guard;
};

struct ev_piqueue_s {
	ev_queue_type  *gl_array;
	int N; // Number of elements.
	int enq_counter; // To identify which list to enqueue
	int deq_counter; // To identify which list to dequeue
	int ub_count;
	int lb_count;
};

struct __pis {
    void                    *data;
    atomic_uintptr_t        next;
};

static void ev_nanosleep(long n)
{
	struct timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = n;

	nanosleep(&ts,NULL);
}

static void init_ev_piqueue_s(struct ev_piqueue_s * pq_ptr, int N)
{
	int prev_guard = 0;
	pq_ptr->gl_array = malloc(N * sizeof(ev_queue_type));
	pq_ptr->enq_counter = 0;
	pq_ptr->deq_counter = 0;
	pq_ptr->ub_count = 0;
	pq_ptr->lb_count = 0;

	for (int i = 0; i < N; i++) {
		pq_ptr->gl_array[i] = create_ev_queue();
	}
	pq_ptr->N = N;
}

static void FIFO_enter(atomic_int * guard, int counter, int N)
{
	int lcount = counter  - N;
	while (lcount != atomic_load_explicit(guard,memory_order_acquire)) {
		EV_YIELD();
		//ev_nanosleep(100);
	}
}

static void FIFO_exit(atomic_int * guard, int counter)
{
	atomic_store_explicit(guard,counter,memory_order_release);
}


int ev_piqueue_peek(struct ev_piqueue_s * pq_ptr)
{
	if(!pq_ptr) EV_ABORT("");
	return 0;
}

static atomic_long hit_count = 0;
static atomic_long miss_count = 0;
static atomic_long succ_count = 0;
static int st_dindex = 0;
void * dequeue_ev_piqueue(ev_piqueue_type  pq_ptr)
{
	struct __pis * first_element = NULL;
	struct __pis * second_element = NULL;
	int d_count = 0;
	int index = 0;
	void * return_data = NULL;

	if(!pq_ptr) EV_ABORT("");

	//if (ev_piqueue_is_empty(pq_ptr)) return NULL;

	/* Get the list for current dequeue, send the next dequeue
	 * to next list. */
	//d_count = atomic_fetch_add_explicit(&(pq_ptr->deq_counter),1,memory_order_acquire);
	//index = d_count % pq_ptr->N;


	/* Decrement ub_count to show one less element. */
	//atomic_fetch_add_explicit(&(pq_ptr->ub_count),-1,memory_order_acquire);

	int i = 0;
	index = st_dindex; st_dindex++;;
	index = index % pq_ptr->N;
	//index = 0;
	while(1) {
		/* If N empty queues are encountered, there is nothing. */
		if (i == pq_ptr->N) break;
		/* try_dequeu returns -1 only in case of it being busy.
		 * In all other cases it returns 0. */
		/* Data present in the piqueue. */
		/*
		*/
		int ret = try_dequeue(pq_ptr->gl_array[index],&return_data);
		if (-1 == ret && !return_data) {
		//if (-1 == ret) {
			//atomic_fetch_add_explicit(&miss_count,1,memory_order_acquire);
			EV_YIELD();
			i=0;
			//continue;
		}
		else {
			if (return_data) {
				atomic_fetch_add_explicit(&succ_count,-1,memory_order_seq_cst);
				break;
			}
			else {
				/* try_dequeue returned 0 and queue is empty,
				 * go on to the next queue. */
			}
		}
			/*
		*/
		//d_count = atomic_fetch_add_explicit(&(pq_ptr->deq_counter),1,memory_order_acquire);
		index++;
		index = index % pq_ptr->N;
		i++;
	}

	return return_data;
}
static int st_index = 0;
void enqueue_ev_piqueue(ev_piqueue_type pq_ptr,void * data)
{
	struct __pis * qe = NULL;
	int e_count = 0, index = 0;
	uintptr_t last = 0;

	if(!pq_ptr) EV_ABORT("");

	/* Signal the presence of another element in the queue. */
	//atomic_fetch_add_explicit(&(pq_ptr->ub_count),1,memory_order_acquire);
	/* Enq_counter indiacates, where the current insert should take place
	 * after the next one should go to the next list. */
	/* Here in below e_count will have the present value.
	 * Next time this line is executed, e_count will get the incremented value, while
	 * the memory location will be incremented. */
	//e_count = atomic_fetch_add_explicit(&(pq_ptr->enq_counter),1,memory_order_acquire);
	//index = e_count % pq_ptr->N;
	/* incr lb_count to permit another dequeue. */
	//atomic_fetch_add_explicit(&(pq_ptr->lb_count),1,memory_order_acquire);

	/* Enqueue the element on the speicified list, making sure that
	 * previous enqueue on this list has completed the critical section. */
	//FIFO_enter(&(pq_ptr->gl_array[index].e_guard),e_count,pq_ptr->N);
	//FIFO_exit(&(pq_ptr->gl_array[index].e_guard),e_count);


	enqueue(pq_ptr->gl_array[st_index],data);
	st_index++;
	st_index = st_index % pq_ptr->N;
	//EV_DBG(__FILE__,__LINE__);
	atomic_fetch_add_explicit(&succ_count,1,memory_order_seq_cst);

	return ;
}

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

ev_piqueue_type create_ev_piqueue(int n)
{
	int N = get_nearest_power_of_2(n);
	struct ev_piqueue_s * pq_ptr = NULL;

	/*
	if (n > 128) {
		errno = EINVAL;
		return NULL;;
	}
	*/

	pq_ptr = malloc(sizeof(struct ev_piqueue_s));

	init_ev_piqueue_s(pq_ptr,N);

	return pq_ptr;
}
void destroy_ev_piqueue(ev_piqueue_type * pq_ptr_ptr)
{
	if(!(*pq_ptr_ptr)) EV_ABORT("");
	printf("N = %d\n",(*pq_ptr_ptr)->N);
	for (int i=0;i<(*pq_ptr_ptr)->N;i++) {
		wf_destroy_ev_queue((*pq_ptr_ptr)->gl_array[i]);
	}
	free((*pq_ptr_ptr)->gl_array);
	(*pq_ptr_ptr)->gl_array = NULL;
	free((*pq_ptr_ptr));
	(*pq_ptr_ptr) = NULL;
	return;
}
void debug_ev_piqueue(ev_piqueue_type  pq_ptr, print_piqnode_func_type dbg_func )
{
	struct __pis * qe = NULL;

	if(!pq_ptr) EV_ABORT("");

	for (int i = 0; i < pq_ptr->N; i++) {
		debug_queue(pq_ptr->gl_array[i],dbg_func);
	}
	return;
}

