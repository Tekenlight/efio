// ev_queue_alloc.c
#include "ev_queue_alloc.h"
#include <sys/mman.h>
#include <unistd.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef EV_QUEUE_ARENA_BYTES
#define EV_QUEUE_ARENA_BYTES (256ULL * 1024 * 1024) // 256 MB default
#endif

static _Atomic(uintptr_t) g_base = 0;
static _Atomic(uintptr_t) g_end  = 0;
static _Atomic(uintptr_t) g_cur  = 0;

static void ev_queue_arena_init(void) {
    size_t bytes = (size_t)EV_QUEUE_ARENA_BYTES;
    void* p = mmap(NULL, bytes, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED) {
        perror("mmap(ev queue arena)");
        abort();
    }
    uintptr_t base = (uintptr_t)p;
    atomic_store(&g_base, base);
    atomic_store(&g_cur,  base);
    atomic_store(&g_end,  base + bytes);
}

void* ev_queue_node_alloc(size_t sz) {
    if (atomic_load(&g_base) == 0) ev_queue_arena_init();

    // align to 16
    sz = (sz + 15u) & ~15u;

    uintptr_t cur = atomic_fetch_add(&g_cur, sz);
    uintptr_t end = atomic_load(&g_end);
    if (cur + sz > end) {
        fprintf(stderr, "EVQ arena exhausted: need=%zu\n", sz);
        abort();
    }
    return (void*)cur;
}

// no free: deliberate
void ev_queue_node_free(void* p, size_t sz) {
    (void)p; (void)sz;
}

