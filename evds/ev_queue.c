#include <stdio.h>
#include <ev_include.h>
#include <ev_queue.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdatomic.h>
#include <assert.h>
#include <unistd.h>

/* Practial to do things
 *
 * Practical recommendation (minimal disruption)
 *
 * Add node-level consumer_claimed CAS (best proof of “single node, single consumer”).
 * Keep your producer claim on inflight.
 * Keep quarantine (it’s good).
 * Run long enough / under higher concurrency to try to force the issue.
 *
 */

#define EVQ_MAGIC_LIVE  0xC0FFEE1234ABCDEFULL
#define EVQ_MAGIC_FREED 0xDEADF00DDEADF00DULL

#define EVQ_QUARANTINE_MAX 64

struct ev_queue_s {
    atomic_uintptr_t    head;
    atomic_uintptr_t    tail;
    /*
    atomic_long            count;
    */
    _Atomic int e_count;
    _Atomic int d_count;

    pthread_mutex_t q_mtx1;
    struct __s* quarantine[EVQ_QUARANTINE_MAX];
    size_t q_len;     // number of items in ring
    size_t q_head_i;  // ring head index
};

struct __s {
    void                    *data;
    atomic_uintptr_t        next;
    _Atomic int inflight;
    _Atomic uint64_t id;
    _Atomic uint64_t magic;
} ;

static _Atomic uint64_t gid = 1;
static _Atomic uint64_t tex_counter = 1;

static inline void evq_retire_node(struct ev_queue_s* q, struct __s* n)
{
    struct __s* to_free = NULL;

    pthread_mutex_lock(&q->q_mtx1);

    if (q->q_len < EVQ_QUARANTINE_MAX) {
        size_t idx = (q->q_head_i + q->q_len) % EVQ_QUARANTINE_MAX;
        q->quarantine[idx] = n;
        q->q_len++;
        pthread_mutex_unlock(&q->q_mtx1);
        return;
    }

    // ring full: evict oldest
    to_free = q->quarantine[q->q_head_i];
    q->quarantine[q->q_head_i] = n;
    q->q_head_i = (q->q_head_i + 1) % EVQ_QUARANTINE_MAX;

    pthread_mutex_unlock(&q->q_mtx1);

    free(to_free);
}


ev_queue_type create_ev_queue()
{
    ev_queue_type newptr = NULL;
    newptr = malloc(sizeof(struct ev_queue_s));
    atomic_init(&newptr->head, 0);
    atomic_init(&newptr->tail, 0);
    atomic_init(&newptr->e_count, 0);
    atomic_init(&newptr->d_count, 0);

    pthread_mutex_init(&newptr->q_mtx1, NULL);
    newptr->q_len = 0;
    newptr->q_head_i = 0;

    return newptr;

}

void wf_destroy_ev_queue(ev_queue_type q_ptr)
{
    void * qe = NULL;

    while((qe = dequeue(q_ptr)));
        //free(qe);

    free(q_ptr);
}

void destroy_ev_queue(ev_queue_type q_ptr)
{
    void * qe = NULL;

    while((qe = dequeue(q_ptr)))
        free(qe);

    free(q_ptr);
}

void debug_queue(ev_queue_type q_ptr, print_qnode_func_type pf)
{
    struct __s * qe = NULL;

    qe = (struct __s *)(q_ptr->head);
    while (qe) {
        pf(qe->data);
        qe = (struct __s *)qe->next;
    }
}

int peek(struct ev_queue_s * q_ptr)
{
    uintptr_t t = 0;
    if (!q_ptr) {
        printf("[%s:%d] Passed q_ptr is null\n",__FILE__,__LINE__);
        abort();
    }
    t = atomic_load_explicit(&(q_ptr->head),memory_order_acquire);
    return (t != 0);
}

int try_dequeue(struct ev_queue_s * q_ptr , void ** retptrptr)
{
    uintptr_t    old_h =0;
    uintptr_t    new_h =0;
    uintptr_t    marked_new_h =0;
    uintptr_t    next = 0;
    void        *data = NULL;
    bool        flg = false;
    uintptr_t    old_t =0;

    *retptrptr = NULL;
    // List is empty if tail is null.
    if (!q_ptr) {
        printf("[%s:%d] Passed q_ptr is null\n",__FILE__,__LINE__);
        abort();
    }
    do {
        old_t = atomic_load_explicit(&(q_ptr->tail),memory_order_acquire);
        if (!old_t) {
            return 0;
        }

        old_h = atomic_load_explicit(&(q_ptr->head),memory_order_acquire);
        if (!old_h) {
            return 0;
        }
        if (!(1&old_h)) {
            new_h = old_h;
            new_h = (1|new_h);
            if (atomic_compare_exchange_strong_explicit(&(q_ptr->head),&old_h,new_h,memory_order_seq_cst,memory_order_seq_cst)) {
                marked_new_h = new_h;
                break;
            }
            else {
                return -1;
            }
        }
        else {
            return -1;
        }
        EV_YIELD();
    } while (true);

    new_h = old_h;
    next = atomic_load_explicit(&(((struct __s *)(new_h))->next),memory_order_acquire);

    old_t = atomic_load_explicit(&(q_ptr->tail),memory_order_acquire);
    // Assert at this point tail cannot be null.
    if (old_t == 0) { // Dont know what to do abort??
        return 0;
    }

    /* If next of the header is  null, the list has become empty. */
    if (0 == next) {
        bool flg = false;
        uintptr_t    new_h1 =0;
        new_h1 = new_h; // new_h1 to avaoid any side effect on new_h due to CAS below.
        flg = atomic_compare_exchange_strong_explicit(&(q_ptr->tail),&new_h1,0,memory_order_seq_cst,memory_order_seq_cst);
        // If old and new tails are not same, it means a new node has come into
        // existence.
        if (!flg) {
            // Repeat as long as next header is null.
            do {
                next = atomic_load_explicit(&(((struct __s *)(new_h))->next),memory_order_acquire);
                EV_YIELD();
            } while (!next);
        }
    }

    new_h = next;
    atomic_thread_fence(memory_order_acquire);
    if (!(1&old_h))
        old_h = (1|old_h);

    {
        uintptr_t oold_h = old_h;
        flg = atomic_compare_exchange_strong_explicit(&(q_ptr->head),&oold_h,new_h,memory_order_seq_cst,memory_order_seq_cst);
        if (!flg) {
            uintptr_t now_h = oold_h;
            EV_DBGP("CAS(head) failed: expected=%p actual=%p new=%p\n",
                    (void*)old_h, (void*)now_h, (void*)new_h);
            abort();
        }
    }
    atomic_thread_fence(memory_order_acquire);
    old_h = (~1&old_h);

    // Now old_h has what we want.
    data = ((struct __s *)old_h)->data;
    free(((struct __s *)old_h));

    *retptrptr = data;

    return 0;
}

void * dequeue(struct ev_queue_s * q_ptr)
{
    uintptr_t    old_h =0;
    uintptr_t    new_h =0;
    uintptr_t    marked_new_h =0;
    uintptr_t    next = 0;
    void        *data = NULL;
    bool        flg = false;
    uintptr_t    old_t =0;

    _Atomic int c_tex;
    atomic_init(&c_tex, atomic_fetch_add(&tex_counter, 1));

    // List is empty if tail is null.
    if (!q_ptr) {
        STACK_TRACE();
        printf("[%s:%d] Passed q_ptr is null\n",__FILE__,__LINE__);
        abort();
    }

    do {
        old_t = atomic_load_explicit(&(q_ptr->tail),memory_order_acquire);
        if (!old_t) {
            return NULL;
        }
        old_h = atomic_load_explicit(&(q_ptr->head),memory_order_acquire);
        if (!old_h) {
            EV_YIELD();
            continue;
        }
        if (!(1&old_h)) {
            new_h = old_h;
            new_h = (1|new_h);
            if (atomic_compare_exchange_strong_explicit(&(q_ptr->head),&old_h,new_h,memory_order_seq_cst,memory_order_seq_cst)) {
                marked_new_h = new_h;
                break;
            }
        }
        EV_YIELD();
    } while (true);

    new_h = old_h;
    next = atomic_load_explicit(&(((struct __s *)(new_h))->next),memory_order_acquire);

    old_t = atomic_load_explicit(&(q_ptr->tail),memory_order_acquire);
    // Assert at this point tail cannot be null.
    assert(old_t != 0);
    if (old_t == 0) { // Dont know what to do abort??
        EV_DBGP("IMPOSSIBLE CONDITION IT HAS COME HERE BECAUSE TAIL IS NOT NULL\n");
        return NULL;
    }

    /* If next of the header is  null, the list has become empty. */
    if (0 == next) {
        bool flg = false;
        uintptr_t    new_h1 =0;
        new_h1 = new_h; // new_h1 to avaoid any side effect on new_h due to CAS below.
        flg = atomic_compare_exchange_strong_explicit(&(q_ptr->tail),&new_h1,0,memory_order_seq_cst,memory_order_seq_cst);
        // If old and new tails are not same, it means a new node has come into
        // existence.
        if (!flg) {
            // Repeat as long as next header is null.
            do {
                next = atomic_load_explicit(&(((struct __s *)(new_h))->next),memory_order_acquire);
                EV_YIELD();
            } while (!next);
        }
    }

    new_h = next;
    atomic_thread_fence(memory_order_acq_rel);
    if (!(1&old_h))
        old_h = (1|old_h);

    {
        uintptr_t oold_h = old_h;
        flg = atomic_compare_exchange_strong_explicit(&(q_ptr->head),&oold_h,new_h,memory_order_seq_cst,memory_order_seq_cst);
        if (!flg) {
            // Since this impossible, let's leave a Harakiri here.
            //
            uintptr_t now_h = oold_h;
            EV_DBGP("CAS(head) failed: expected=%p actual=%p new=%p\n",
                    (void*)old_h, (void*)now_h, (void*)new_h);
            abort();
        }
    }

    //EV_DBGP("Execution counter = [%d] q_ptr=[%p]\n", c_tex, q_ptr);
    //EV_DBGP("Tail = [%X] HEAD = [%X]\n", atomic_load(&q_ptr->tail), atomic_load(&q_ptr->head));
    atomic_thread_fence(memory_order_acq_rel);
    old_h = (~1&old_h);

    assert(old_h != 0);
    atomic_fetch_add(&(q_ptr->d_count), 1);


    // Now old_h has what we want.
    data = ((struct __s *)old_h)->data;
    if (atomic_load(&((struct __s*)old_h)->inflight)) {
        EV_DBGP("BUG: freeing node still inflight: %p\n", (void*)old_h);
        EV_DBGP("ecount = [%d] dcount=[%d]\n", atomic_load(&(q_ptr->e_count)), atomic_load(&(q_ptr->d_count)));
        abort();
    }
    uint64_t id = atomic_load(&((struct __s*)old_h)->id);
    //EV_DBGP("FREE node=%p id=%lu Execution counter = [%d] q_ptr=[%p]\n", (void*)old_h, (unsigned long)id, c_tex, q_ptr);
    uint64_t m = atomic_load(&(((struct __s *)old_h)->magic));
    if (m != EVQ_MAGIC_LIVE) {
        EV_DBGP("EVQ BUG: old_h magic invalid before retire: old_h=%p id=%lu magic=%lx q=%p\n",
                (struct __s*)old_h, ((struct __s*)old_h)->id, m, q_ptr);
        abort();
    }

    atomic_store(&(((struct __s*)old_h)->magic), EVQ_MAGIC_FREED);
    memset((struct __s *)old_h, 0xA5, sizeof(struct __s));


    free(((struct __s *)old_h));
    //evq_retire_node(q_ptr, (struct __s*)old_h);

    return data;
}

int queue_empty(struct ev_queue_s * q_ptr)
{
    uintptr_t        tail = 0;
    if (!q_ptr) {
        printf("[%s:%d] Passed q_ptr is null\n",__FILE__,__LINE__);
        abort();
    }
    tail = atomic_load(&(q_ptr->tail));
    return (int)(tail == 0);
}

void enqueue(struct ev_queue_s * q_ptr,void * data)
{
    struct __s *    ptr = NULL;
    uintptr_t        old_tail = 0;
    uintptr_t        zero_head = 0;
    bool            flg = false;

    ptr = (struct __s*)malloc(sizeof(struct __s));
    ptr->data = data;
    atomic_init(&ptr->next, 0);
    atomic_init(&ptr->inflight, 0);
    atomic_init(&ptr->id, atomic_fetch_add(&gid, 1));
    atomic_store(&ptr->magic, EVQ_MAGIC_LIVE);

    old_tail = atomic_exchange_explicit(&(q_ptr->tail),(uintptr_t)ptr,memory_order_seq_cst);
    atomic_fetch_add(&(q_ptr->e_count), 1);

    if (old_tail) {
        atomic_store(&((struct __s*)old_tail)->inflight, 1);
        struct __s *t = (struct __s*)old_tail;
        uint64_t m = atomic_load(&t->magic);
        if (m != EVQ_MAGIC_LIVE) {
            EV_DBGP("EVQ BUG: enqueue linking from non-live node: tail=%p id=%lu magic=%lx q=%p\n", t, t->id, m, q_ptr);
            abort();
        }

        atomic_store_explicit(&(((struct __s *)old_tail)->next),
                                        (uintptr_t)ptr,memory_order_release);
        atomic_store(&((struct __s*)old_tail)->inflight, 0);
    }
    else {
        atomic_exchange_explicit(&(q_ptr->head),(uintptr_t)ptr,memory_order_acq_rel);
    }

    return;
}


#ifdef NEVER


struct thr_inp {
    ev_queue_type pq_ptr;
    int N;
    int slot;
};

void * thr(void *param)
{
    struct thr_inp * inp;
    int i = 0;
    int j = 0;
    inp = param;

    ev_queue_type pq_ptr = (ev_queue_type)inp->pq_ptr;;
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
        if (i%1000 == 0) printf("THREAD1:0%d:%d\n",i,j);//fflush(stdout);
        enqueue(pq_ptr,(void*)(long)j);
        //enqueue(pq_ptr,(void*)(long)j);
        //usleep(1);
        dequeue(pq_ptr);
        //dequeue(pq_ptr);
        //j++;i++;
        if (i%1000 == 0) printf("THREAD1:0%d:%d\n",i,(int)dequeue(pq_ptr));fflush(stdout);
    }

    /*
    for (i=0;i<COUNT;i++,j++) {
        //printf("THREAD1:0%d:%d\n",i,j);//fflush(stdout);
        //enqueue(pq_ptr,(void*)(long)j);
        //usleep(1);
        dequeue(pq_ptr);
        //printf("THREAD1:0%d:%d\n",i,(int)dequeue(pq_ptr));fflush(stdout);
    }
    */

    //printf("THR1 OVER\n");

    return NULL;
}

void * thr_TEST_PRODUCER_1(void *param)
{
    int i = 0; int j= 1024*0+1;
    int fd = 0;
    ev_queue_type pq_ptr = (ev_queue_type)param;
    for (i=0;i<1024;i++,j++) {
        enqueue(pq_ptr,(void*)(long)j);
        printf("THREAD_TEST_PRODUCER:%d\n",j);fflush(stdout);
    }

    return NULL;
}

void * thr_TEST_PRODUCER_2(void *param)
{
    int i = 0; int j= 1024*1+1;
    int fd = 0;
    ev_queue_type pq_ptr = (ev_queue_type)param;
    for (i=0;i<1024;i++,j++) {
        enqueue(pq_ptr,(void*)(long)j);
        printf("THREAD_TEST_PRODUCER:%d\n",j);fflush(stdout);
    }

    return NULL;
}

void * thr_TEST_PRODUCER_3(void *param)
{
    int i = 0; int j= 1024*2+1;
    int fd = 0;
    ev_queue_type pq_ptr = (ev_queue_type)param;
    for (i=0;i<1024;i++,j++) {
        enqueue(pq_ptr,(void*)(long)j);
        printf("THREAD_TEST_PRODUCER:%d\n",j);fflush(stdout);
    }

    return NULL;
}

void * thr_TEST_PRODUCER_4(void *param)
{
    int i = 0; int j= 1024*3+1;
    int fd = 0;
    ev_queue_type pq_ptr = (ev_queue_type)param;
    for (i=0;i<1024;i++,j++) {
        enqueue(pq_ptr,(void*)(long)j);
        printf("THREAD_TEST_PRODUCER:%d\n",j);fflush(stdout);
    }

    return NULL;
}

void * thr_TEST_CONSUMER_1(void *param)
{
    int i = 0;
    int fd = 0;
    ev_queue_type pq_ptr = (ev_queue_type)param;
    for (i=0;i<1024*2;i++) {
        fd = 0;
        do {
            fd = (int)(long)dequeue(pq_ptr);
        } while (fd == 0);
        printf("THREAD_TEST_CONSUMER:%d\n",fd);fflush(stdout);
    }

    return NULL;
}

void * thr_TEST_CONSUMER_2(void *param)
{
    int i = 0;
    int fd = 0;
    ev_queue_type pq_ptr = (ev_queue_type)param;
    for (i=0;i<1024*2;i++) {
        fd = 0;
        do {
            fd = (int)(long)dequeue(pq_ptr);
        } while (fd == 0);
        printf("THREAD_TEST_CONSUMER:%d\n",fd);fflush(stdout);
    }

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

    ev_queue_type q;

    q = create_ev_queue();

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

    pthread_create(&t0,NULL,thr_TEST_PRODUCER_1,q);
    pthread_create(&t1,NULL,thr_TEST_PRODUCER_2,q);
    pthread_create(&t2,NULL,thr_TEST_PRODUCER_3,q);
    pthread_create(&t3,NULL,thr_TEST_PRODUCER_4,q);

    usleep(50);

    pthread_create(&t4,NULL,thr_TEST_CONSUMER_1,q);
    pthread_create(&t5,NULL,thr_TEST_CONSUMER_2,q);

    pthread_join(t0,&retptr);
    pthread_join(t1,&retptr);
    pthread_join(t2,&retptr);
    pthread_join(t3,&retptr);
    pthread_join(t4,&retptr);
    pthread_join(t5,&retptr);
    /*
    */

    /*
    inp1.slot = 0;
    pthread_create(&t1, NULL, thr, &inp1);
    inp2.slot = 1;
    pthread_create(&t2, NULL, thr, &inp2);
    inp3.slot = 2;
    pthread_create(&t3, NULL, thr, &inp3);
    inp4.slot = 3;
    pthread_create(&t4, NULL, thr, &inp4);
    */

    /*
    inp5.slot = 5;
    pthread_create(&t5, NULL, thr, &inp5);
    */
    /*
    pthread_join(t0,&retptr);
    pthread_join(t1,&retptr);
    pthread_join(t2,&retptr);
    pthread_join(t3,&retptr);
    pthread_join(t4,&retptr);
    */

    /*
    pthread_join(t5,&retptr);
    */
    //printf("HELLO\n");

    {
        void * p;
        int i = 0;
        while (NULL != (p = dequeue(q))) {
            ++i;
            fd = (int)(long)p;
            printf("[MAIN]:%d:%d\n",i,fd);//fflush(stdout);
        }
    }


    //destroy_ev_queue(&q);

    return 0;
}

#endif
