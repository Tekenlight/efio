#ifndef EV_QUEUE_H_INCLUDED
#define EV_QUEUE_H_INCLUDED

#include <sys/cdefs.h>

typedef struct ev_paqueue_s *ev_paqueue_type;
typedef void (*print_paqnode_func_type)(int,void*);

__BEGIN_DECLS
int peek_evpaq(struct ev_paqueue_s * q_ptr);
void * dequeue_evpaq(ev_paqueue_type );
void enqueue_evpaq(ev_paqueue_type ,void * data);
ev_paqueue_type create_evpaq(int n);
void destroy_evpaq(ev_paqueue_type *);
void debug_paq(ev_paqueue_type , print_paqnode_func_type );
__END_DECLS

#endif
