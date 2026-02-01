#ifndef EV_QUEUE_ALLOC_H_INCLUDED
#define EV_QUEUE_ALLOC_H_INCLUDED

#include <stddef.h>

void* ev_queue_node_alloc(size_t sz);
void  ev_queue_node_free(void* p, size_t sz);

#endif // EV_QUEUE_ALLOC_H_INCLUDED
