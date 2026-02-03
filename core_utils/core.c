#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define BD_MAGIC 0xB10BDADAu
#define BD_FREED 0xDEADFA11u
#define BD_TAIL  0xA5A5A5A5A5A5A5A5ull

typedef struct {
    uint32_t magic;
    uint32_t pad;
    size_t   size;
} BDHdr;

static inline BDHdr* bd_hdr(unsigned char *p)
{
    return (BDHdr*)(p - sizeof(BDHdr));
}

void free_binary_data(unsigned char *data)
{
    if (!data) return;
    BDHdr *h = bd_hdr(data);

    if (h->magic == BD_FREED) {
        fprintf(stderr, "%s:%d free_binary_data: DOUBLE FREE %p\n", __FILE__, __LINE__, (void*)data);
        abort();
    }
    if (h->magic != BD_MAGIC) {
        fprintf(stderr, "%s:%d free_binary_data: INVALID POINTER %p (magic=%08x)\n",
                __FILE__, __LINE__, (void*)data, h->magic);
        abort();
    }

    uint64_t tail = *(uint64_t*)(data + h->size + 1);
    if (tail != BD_TAIL) {
        fprintf(stderr, "%s:%d free_binary_data: BUFFER OVERRUN %p size=%zu tail=%llx\n",
                __FILE__, __LINE__, (void*)data, h->size, (unsigned long long)tail);
        abort();
    }

    // Optional poison to make UAF show up as 0xA5 patterns in cores
    memset(data, 0xA5, h->size + 1);

    h->magic = BD_FREED;
    free(h);
}

size_t binary_data_len(unsigned char *data)
{
    if (!data) return 0;
    BDHdr *h = bd_hdr(data);
    if (h->magic != BD_MAGIC) abort();
    return h->size;
}

void * alloc_binary_data_memory(size_t size)
{
    size_t total = sizeof(BDHdr) + size + 1 + sizeof(uint64_t);
    BDHdr *h = (BDHdr*)malloc(total);
    if (!h) return NULL;

    h->magic = BD_MAGIC;
    h->pad   = 0;
    h->size  = size;

    unsigned char *data = (unsigned char*)(h + 1);
    // We can skip zeoring
    memset(data, 0, size + 1);
    *(uint64_t*)(data + size + 1) = BD_TAIL;

    return data;
}

