#ifndef _MEMPOOL_H_
#define _MEMPOOL_H_
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <inttypes.h>
#include <sys/queue.h>

#include "ring.h"

#ifdef __cplusplus
extern "C" {
#endif

#define 	MEMPOOL_F_SP_PUT   0x0004
#define 	MEMPOOL_F_SC_GET   0x0008

struct mempool {
    shm_allocator_t *shm;
    struct ring *r;
    int flags;
    unsigned max_eles;
    unsigned elt_size;
    char data[0];
};

struct mempool *mempool_create(shm_allocator_t *shm, unsigned n,
                                unsigned elt_size, unsigned flags);
void mempool_destroy(struct mempool *mp);

static inline int
mempool_get(struct mempool *mp, void **obj_p) {
    return ring_dequeue(mp->r, obj_p);
}

static inline int
mempool_sc_get(struct mempool *mp, void **obj_p) {
    return ring_sc_dequeue(mp->r, obj_p);
}

static inline int
mempool_mc_get(struct mempool *mp, void **obj_p) {
    return ring_mc_dequeue(mp->r, obj_p);
}

static inline int
mempool_get_bulk(struct mempool *mp, void **obj_tbl, unsigned n) {

}

static inline int
mempool_put(struct mempool *mp, void *obj) {
    return ring_enqueue(mp->r, obj);
}

static inline int
mempool_sp_put(struct mempool *mp, void *obj) {
    return ring_sp_enqueue(mp->r, obj);
}

static inline int
mempool_mp_put(struct mempool *mp, void *obj) {
    return ring_mp_enqueue(mp->r, obj);
}

static inline int
mempool_put_bulk(struct mempool *mp, void **obj_tbl, unsigned n) {

}
#ifdef __cplusplus
}
#endif

#endif /* _MEMPOOL_H_ */
