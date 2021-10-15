#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <ev_stack.h>

struct __s {
	void 					*data;
	struct __s				*next;
};

struct ev_stack {
	atomic_uintptr_t head;
};

ev_stack_type create_ev_stack()
{
	struct ev_stack  *new_stack = NULL;
	new_stack = malloc(sizeof(struct ev_stack));
	new_stack->head = 0;
	return new_stack;
}
void destroy_ev_stack(ev_stack_type sptr)
{
	while (pop(sptr));
	free(sptr);
	return ;
}

void push(ev_stack_type stptr,void * data)
{
	struct __s			*sptr = NULL;
	uintptr_t			old_h=0;

	old_h = atomic_load(&stptr->head);
	sptr = malloc(sizeof(struct __s));
	sptr->data = data;
	// The header can be marked. We dont want marked values in the next pointer.
	sptr->next = (struct __s *)(old_h&~1);

	// It is required to ensure that the header that was seen just now has not changed
	// Else a branch can get created, if there is a concurrent thread trying to push
	// a record with the same starting state of the stack.
	while (!atomic_compare_exchange_strong_explicit(&stptr->head,&old_h,
				(uintptr_t)sptr,memory_order_seq_cst,memory_order_seq_cst))
		sptr->next = (struct __s *)(old_h&~1);
}

void * pop(ev_stack_type stptr)
{
	void		*data = NULL;
	uintptr_t	old_h = 0;
	uintptr_t	m_old_h = 0;
	uintptr_t	new_h = 0;

	do {
		old_h = atomic_load_explicit(&stptr->head,memory_order_acquire);
		if (!old_h) return NULL;
		if (old_h&1) continue;
		m_old_h = old_h + 1;
		if (!atomic_compare_exchange_strong_explicit(&stptr->head,&old_h,m_old_h,
						memory_order_seq_cst,memory_order_seq_cst)) continue;
		// Now no one else will pop it out. Therefore safe to dereference it.
		new_h = (uintptr_t)(((struct __s *)old_h)->next);
		// However some one else can push a node and change the head
		// If that has happened, start all over again.
		if (!atomic_compare_exchange_strong_explicit(&stptr->head,&m_old_h,new_h,
						memory_order_seq_cst,memory_order_seq_cst)) continue;
		data = ((struct __s *)old_h)->data;
		break;
	} while (true);
	return data;
}



#ifdef NEVER
#ifdef NEVER
#include <stdio.h>
#endif


void * thr1(void *param)
{
	int i = 0;
	int j = 0*1024*1024;
	ev_stack_type ev_stack = (ev_stack_type)param;
	for (i=0;i<1024*1024;i++,j++) {
		push(ev_stack,(void*)(long)j);
		pop(ev_stack);
		//printf("THREAD1: fd[%d] : %d\n",i,pop(ev_stack));
	}

	return NULL;
}

void * thr2(void *param)
{
	int i = 0;
	int j = 1*1024*1024;
	ev_stack_type ev_stack = (ev_stack_type)param;
	for (i=0;i<1024*1024;i++,j++) {
		push(ev_stack,(void*)(long)j);
		pop(ev_stack);
		//printf("THREAD2: fd[%d] : %d\n",i,pop(ev_stack));
	}

	return NULL;
}

void * thr3(void *param)
{
	int i = 0;
	int j = 2*1024*1024;
	ev_stack_type ev_stack = (ev_stack_type)param;
	for (i=0;i<1024*1024;i++,j++) {
		push(ev_stack,(void*)(long)j);
		pop(ev_stack);
		//printf("THREAD3: fd[%d] : %d\n",i,pop(ev_stack));
	}

	return NULL;
}

void * thr4(void *param)
{
	int i = 0;
	int j= 3*1024*1024;
	ev_stack_type ev_stack = (ev_stack_type)param;
	for (i=0;i<1024*1024;i++,j++) {
		push(ev_stack,(void*)(long)j);
		pop(ev_stack);
		//printf("THREAD4: fd[%d] : %d\n",i,pop(ev_stack));
	}

	return NULL;
}

void * thr_TEST(void *param)
{
	int i = 0;
	int j= 0;
	int fd = 0;
	ev_stack_type ev_stack = (ev_stack_type)param;
	for (i=0;i<1024*1024;i++,j++) {
		push(ev_stack,(void*)(long)j);
	}
	for (i=0;i<1024*1024;i++) {
		fd = (int)(long)pop(ev_stack);
		printf("THREAD_TEST:%d:%d\n",i,fd);//fflush(stdout);
	}

	return NULL;
}
int main()
{
	int i = -1;
	pthread_t t0, t1, t2, t3, t4;
	void *retptr = NULL;

	ev_stack_type ev_stack = NULL;

	ev_stack = create_ev_stack();

	// /*
	pthread_create(&t1, NULL, thr1, ev_stack);
	pthread_create(&t2, NULL, thr2, ev_stack);
	pthread_create(&t3, NULL, thr3, ev_stack);
	pthread_create(&t4, NULL, thr4, ev_stack);
	// */

	//pthread_create(&t0,NULL,thr_TEST,NULL);

	 ///*
	pthread_join(t1,&retptr);
	pthread_join(t2,&retptr);
	pthread_join(t3,&retptr);
	pthread_join(t4,&retptr);
	 //*/

	//pthread_join(t0,&retptr);
	{
		int i = 0;
		int fd = 0;
		while (NULL != pop(ev_stack)) {
			++i;
			printf("[MAIN]:%d:%d\n",i,fd);//fflush(stdout);
		}
	}

	return 0;
}

#endif
