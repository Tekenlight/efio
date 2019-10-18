#ifndef EV_QUEUE_H_INCLUDED
#define EV_QUEUE_H_INCLUDED

#include <sys/cdefs.h>

typedef struct ev_queue_s *ev_queue_type;
typedef void (*print_qnode_func_type)(void*);

__BEGIN_DECLS
int queue_empty(struct ev_queue_s * q_ptr);
int peek(struct ev_queue_s * q_ptr);
void * dequeue(ev_queue_type );
int try_dequeue(ev_queue_type, void** );
void enqueue(ev_queue_type ,void * data);
ev_queue_type create_ev_queue();
void destroy_ev_queue(ev_queue_type);
void wf_destroy_ev_queue(ev_queue_type);
void debug_queue(ev_queue_type , print_qnode_func_type );
__END_DECLS

#endif
