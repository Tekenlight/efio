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

struct qe_s {
	void * data;
	struct qe_s * next;
};

struct basic_queue_s {
	struct qe_s * head;
	struct qe_s * tail;
	pthread_mutex_t m;
};

static struct basic_queue_s * create_basic_queue()
{
	struct basic_queue_s * q = malloc(sizeof(struct basic_queue_s));

	q->head  = NULL;
	q->tail  = NULL;
	pthread_mutex_init(&(q->m),NULL);

	return q;
}

static int basic_enqueue(struct basic_queue_s * q, void *data)
{
	struct qe_s * qe = NULL;
	if (pthread_mutex_trylock(&(q->m))) return -1;
	qe = malloc(sizeof(struct qe_s));
	qe->data = data;
	qe->next = NULL;
	if (q->tail == NULL) {
		q->tail = qe; q->head = qe;
	}
	else {
		q->tail->next = qe;
		q->tail = qe;
	}
	pthread_mutex_unlock(&(q->m));
	return 0;
}

static int basic_try_dequeue(struct basic_queue_s * q,void **return_data)
{
	*return_data = NULL;
	int ret = 0;
	ret = pthread_mutex_trylock(&q->m);
	if (ret != 0) { return -1; } 
	if (q->head != NULL) {
		*return_data = q->head->data;
		q->head = q->head->next;
		if (!(q->head)) q->tail = q->head; 
	}
	pthread_mutex_unlock(&(q->m));

	return 0;
}

/*
struct ev_mpiqueue_gl_s {
	struct ev_list_s list;
	atomic_int e_guard;
	atomic_int d_guard;
};
*/

struct ev_piqueue_s {
	struct basic_queue_s ** gl_array;
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

static void init_ev_mpiqueue_s(struct ev_piqueue_s * pq_ptr, int N)
{
	int prev_guard = 0;
	pq_ptr->gl_array = malloc(N * sizeof(struct basic_queue_s *));
	pq_ptr->enq_counter = 0;
	pq_ptr->deq_counter = 0;
	pq_ptr->ub_count = 0;
	pq_ptr->lb_count = 0;

	for (int i = 0; i < N; i++) {
		pq_ptr->gl_array[i] = create_basic_queue();
	}
	pq_ptr->N = N;
}

static void FIFO_enter(atomic_int * guard, int counter, int N)
{
	int lcount = counter  - N;
	while (lcount != atomic_load_explicit(guard,memory_order_relaxed)) {
		EV_YIELD();
		//ev_nanosleep(100);
	}
}

static void FIFO_exit(atomic_int * guard, int counter)
{
	atomic_store_explicit(guard,counter,memory_order_relaxed);
}

int ev_mpiqueue_peek(struct ev_piqueue_s * pq_ptr)
{
	if(!pq_ptr) EV_ABORT("");
	return 0;
}

static atomic_long hit_count = 0;
static atomic_long miss_count = 0;
void * dequeue_ev_mpiqueue(ev_piqueue_type  pq_ptr)
{
	struct __pis * first_element = NULL;
	struct __pis * second_element = NULL;
	int d_count = 0;
	int index = 0;
	void * return_data = NULL;

	if(!pq_ptr) EV_ABORT("");

	int i = 0;
	index = 0;
	while(1) {
		/* If N empty queues are encountered, there is nothing. */
		if (i == pq_ptr->N) break;
		/* try_dequeu returns -1 only in case of it being busy.
		 * In all other cases it returns 0. */
		/* Data present in the piqueue. */
		/*
		*/
		if (-1 == basic_try_dequeue(pq_ptr->gl_array[index],&return_data)) {
			EV_YIELD();
		}
		else {
			if (return_data) {
				break;
			}
			else {
				/* try_dequeue returned 0 and queue is empty,
				 * go on to the next queue. */
			}
			i++;
		}
		/*
		*/
		//d_count = atomic_fetch_add_explicit(&(pq_ptr->deq_counter),1,memory_order_relaxed);
		++index;
		index = index % pq_ptr->N;
	}

	return return_data;
}
static int st_index = 0;
void enqueue_ev_mpiqueue(ev_piqueue_type pq_ptr,void * data)
{
	struct __pis * qe = NULL;
	int e_count = 0, index = 0;
	uintptr_t last = 0;

	if(!pq_ptr) EV_ABORT("");

	while (1) {
		st_index = st_index % pq_ptr->N;
		if (0 == basic_enqueue(pq_ptr->gl_array[st_index],data)) break;
		st_index++;
		EV_YIELD();
	}
	//EV_DBG(__FILE__,__LINE__);

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

ev_piqueue_type create_ev_mpiqueue(int n)
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

	init_ev_mpiqueue_s(pq_ptr,N);

	return pq_ptr;
}
void destroy_ev_mpiqueue(ev_piqueue_type * pq_ptr_ptr)
{
	if(!(*pq_ptr_ptr)) EV_ABORT("");
	printf("N = %d\n",(*pq_ptr_ptr)->N);
	for (int i=0;i<(*pq_ptr_ptr)->N;i++) {
		//destroy_ev_queue((*pq_ptr_ptr)->gl_array[i]);
	}
	free((*pq_ptr_ptr)->gl_array);
	(*pq_ptr_ptr)->gl_array = NULL;
	free((*pq_ptr_ptr));
	(*pq_ptr_ptr) = NULL;
	return;
}
void debug_ev_mpiqueue(ev_piqueue_type  pq_ptr, print_piqnode_func_type dbg_func )
{
	struct __pis * qe = NULL;

	if(!pq_ptr) EV_ABORT("");

	for (int i = 0; i < pq_ptr->N; i++) {
		//debug_queue(pq_ptr->gl_array[i],dbg_func);
	}
	return;
}

#ifdef NEVER

struct thr_inp {
	ev_piqueue_type pq_ptr;
	int N;
	int slot;
};

void * thr(void *param)
{
	struct thr_inp * inp;
	int i = 0;
	int j = 0;
	inp = param;

	ev_piqueue_type pq_ptr = (ev_piqueue_type)inp->pq_ptr;;
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
		enqueue_ev_piqueue(pq_ptr,(void*)(long)j);
		//enqueue_ev_piqueue(pq_ptr,(void*)(long)j);
		//usleep(1);
		dequeue_ev_piqueue(pq_ptr);
		//dequeue_ev_piqueue(pq_ptr);
		//j++;i++;
		//printf("THREAD1:0%d:%d\n",i,(int)dequeue_ev_piqueue(pq_ptr));fflush(stdout);
	}

	/*
	for (i=0;i<COUNT;i++,j++) {
		//printf("THREAD1:0%d:%d\n",i,j);//fflush(stdout);
		//enqueue_ev_piqueue(pq_ptr,(void*)(long)j);
		//usleep(1);
		dequeue_ev_piqueue(pq_ptr);
		//printf("THREAD1:0%d:%d\n",i,(int)dequeue_ev_piqueue(pq_ptr));fflush(stdout);
	}
	*/

	//printf("THR1 OVER\n");

	return NULL;
}

void * thr_TEST(void *param)
{
	int i = 0; int j= 0;
	int fd = 0;
	ev_piqueue_type pq_ptr = (ev_piqueue_type)param;
	for (i=0;i<128;i++,j++) {
		enqueue_ev_piqueue(pq_ptr,(void*)(long)j);
		fd = (int)(long)dequeue_ev_piqueue(pq_ptr);
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

	ev_piqueue_type q;

	q = create_ev_piqueue(4);

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
		while (NULL != (p = dequeue_ev_piqueue(q))) {
			++i;
			fd = (int)(long)p;
			printf("[MAIN]:%d:%d\n",i,fd);//fflush(stdout);
		}
	}


	//destroy_ev_queue(&q);

	return 0;
}

#endif
