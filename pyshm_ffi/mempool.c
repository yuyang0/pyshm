#include "mempool.h"

/* true if x is a power of 2 */
#define POWEROF2(x) ((((x)-1) & (x)) == 0)

static inline unsigned
next_pow_of_2(unsigned size) {
    size |= size >> 1;
    size |= size >> 2;
    size |= size >> 4;
    size |= size >> 8;
    size |= size >> 16;
    return size + 1;
}

struct mempool *
mempool_create(shm_allocator_t *shm, unsigned n, unsigned elt_size, unsigned flags) {
    int res = 0;
    int ring_flags = 0;
    unsigned totalsize = sizeof(struct mempool) + (n * elt_size);
    unsigned ring_count = n+1;
    struct mempool *mp = shm_alloc(shm, totalsize);
    if (mp == NULL) {
        return NULL;
    }
    mp->shm = shm;
    mp->max_eles = n;
    mp->elt_size = elt_size;

    if (flags & MEMPOOL_F_SP_PUT) ring_flags |= RING_F_SP_ENQ;
    if (flags & MEMPOOL_F_SC_GET) ring_flags |= RING_F_SC_DEQ;
    if (! POWEROF2(ring_count)) {
        ring_count = next_pow_of_2(ring_count);
    }

    mp->r = ring_create(shm, ring_count, ring_flags);
    if (mp->r == NULL) {
        perror("ring create: ");
        shm_free(shm, mp, totalsize);
        return NULL;
    }
    for (unsigned i = 0; i < n; i++) {
        char *ptr = mp->data + (i * elt_size);
        res = ring_enqueue(mp->r, ptr);
        if (res != 0) {
            perror("ring enqueue: ");
            ring_destroy(mp->r);
            shm_free(shm, mp, totalsize);
            return NULL;
        }
    }
    return mp;
}

void
mempool_destroy(struct mempool *mp) {
    unsigned totalsize = sizeof(*mp) + (mp->max_eles * mp->elt_size);
    ring_destroy(mp->r);
    shm_free(mp->shm, mp, totalsize);
}
