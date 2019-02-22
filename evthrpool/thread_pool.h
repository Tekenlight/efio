#ifndef THREAD_POOL_H_INCLUDED
#define THREAD_POOL_H_INCLUDED

#include <sys/cdefs.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

#define MIN_SLEEP_USEC 10
#define MAX_SLEEP_USEC 200
#define ALWD_BUSY_WAITS 5

typedef struct thread_pool_s thread_pool_s_type;
typedef struct thread_pool_s *thread_pool_type;
typedef struct task_s task_s_type;
typedef void (*task_func_type)(void*);
typedef void *task_argument_type;

__BEGIN_DECLS
thread_pool_s_type * create_thread_pool(int num_threads);
int destroy_thread_pool(thread_pool_s_type *);
void enqueue_task(thread_pool_type,task_func_type,task_argument_type);
int thrpool_get_task_count(struct thread_pool_s *pool);
__END_DECLS

#endif
