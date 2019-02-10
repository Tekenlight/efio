#ifndef EV_STACK_H_INCLUDED
#define EV_STACK_H_INCLUDED

struct ev_stack;
typedef struct ev_stack * ev_stack_type;

__BEGIN_DECLS
ev_stack_type create_ev_stack();
void destroy_ev_stack(ev_stack_type);
void push(ev_stack_type,void * );
void * pop(ev_stack_type);
__END_DECLS


#endif
