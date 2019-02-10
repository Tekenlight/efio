#ifndef EV_PIQUEUE_H_INCLUDED
#define EV_PIQUEUE_H_INCLUDED

#include <sys/cdefs.h>

struct ev_piqueue_s;

typedef struct ev_piqueue_s *ev_piqueue_type;
typedef void (*print_piqnode_func_type)(void*);

__BEGIN_DECLS
int ev_piqueue_peek(struct ev_piqueue_s * pq_ptr);
void * dequeue_ev_piqueue(ev_piqueue_type  pq_ptr);
void enqueue_ev_piqueue(ev_piqueue_type pq_ptr,void * data);
ev_piqueue_type create_ev_piqueue(int );
void destroy_ev_piqueue(ev_piqueue_type *pq_ptr_ptr);
void debug_ev_piqueue(ev_piqueue_type  pq_ptr, print_piqnode_func_type dbg_func );

__END_DECLS









#endif
