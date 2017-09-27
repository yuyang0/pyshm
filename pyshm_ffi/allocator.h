#ifndef _ALLOCATOR_H_
#define _ALLOCATOR_H_ 1

#include <math.h>
#include "buddy2.h"

typedef struct shm_s {
    struct buddy2 *buddy;
    void *data;
    int size;

    // statistics
    long total_mem;
    long used_mem;
    long buddy2_used_mem;
    long buddy2_real_used_mem;

    long pool_ds_size;
    long buddy2_ds_size;
}shm_allocator_t;

#define SIZE_PER_UNIT 4
#define NUM_UNITS(sz) ((int)ceil(((double)(sz))/ SIZE_PER_UNIT))

#define SHM_ALL_MEM_SIZE(sz) (sizeof(shm_allocator_t) + \
                              BUDDY2_DS_SIZE(sz/SIZE_PER_UNIT) + (sz))
#define SHM_ALL_DS_SIZE(sz)  (sizeof(shm_allocator_t) + BUDDY2_DS_SIZE(sz/SIZE_PER_UNIT))

shm_allocator_t *shm_create(int size);
int shm_init(shm_allocator_t *pp, int size);
void *shm_alloc(shm_allocator_t *pool, int bytes);
void shm_free(shm_allocator_t *pool, void *p, long bytes);
int shm_destroy(shm_allocator_t *pool);

#endif /* _ALLOCATOR_H_ */
