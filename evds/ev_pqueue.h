#ifndef EV_PQUEUE_H_INCLUDED
#define EV_PQUEUE_H_INCLUDED

#include <sys/cdefs.h>

struct ev_pqueue_s;

typedef struct ev_pqueue_s *ev_pqueue_type;
typedef void (*print_pqnode_func_type)(int,void*);

__BEGIN_DECLS
int ev_pqueue_peek(struct ev_pqueue_s * pq_ptr);
void * dequeue_ev_pqueue(ev_pqueue_type  pq_ptr);
void enqueue_ev_pqueue(ev_pqueue_type pq_ptr,void * data);
ev_pqueue_type create_ev_pqueue(int );
void destroy_ev_pqueue(ev_pqueue_type *pq_ptr_ptr);
void wf_destroy_ev_pqueue(ev_pqueue_type *pq_ptr_ptr);
void debug_ev_pqueue(ev_pqueue_type  pq_ptr, print_pqnode_func_type dbg_func );

__END_DECLS









#endif
