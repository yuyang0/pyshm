/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include <sys/mman.h>

#include "buddy2.h"
#include "allocator.h"
#include "log.h"

/*
 * 内存以四字节为单位交给buddy处理.
 * @size: 被管理的内存的大小（以字节为单位），因为我们需要为pool，buddy2分配数据结构，
 *        所以实际需要的内存大小要通过POOL_ALL_MEM_SIZE来计算。
 */
shm_allocator_t *shm_create(int size) {
    shm_allocator_t *pp;
    pp = mmap(NULL, SHM_ALL_MEM_SIZE(size), PROT_READ|PROT_WRITE,
              MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    if (pp == MAP_FAILED) {
        LOG_ERR("mmap failed: %s", strerror(errno));
        return NULL;
    }
    int num_units = NUM_UNITS(size);

    pp->buddy = buddy2_new(num_units, (void *)(pp +1));
    if (pp->buddy == NULL) {
        LOG_ERR("buddy2 new failed: ");
        return NULL;
    }
    pp->size = size;
    pp->data = ((void*) pp) + SHM_ALL_DS_SIZE(size);

    pp->buddy2_ds_size = BUDDY2_DS_SIZE(num_units);
    return pp;
}

int shm_init(shm_allocator_t *pp, int size) {
    assert(pp);

    int num_units = NUM_UNITS(size);

    pp->buddy = buddy2_new(num_units, (void *)(pp+1));
    if (pp->buddy == NULL) {
        return -1;
    }
    pp->size = size;
    pp->data = ((void*)pp) + SHM_ALL_DS_SIZE(size);

    pp->buddy2_ds_size = BUDDY2_DS_SIZE(num_units);

    return 0;
}

int shm_destroy(shm_allocator_t *shm) {
    return munmap(shm, SHM_ALL_MEM_SIZE(shm->size));
}

void *shm_alloc(shm_allocator_t *pool, int bytes) {
    long num_units = NUM_UNITS(bytes);
    if (!IS_POWER_OF_2(num_units)) {
        num_units = fixsize(num_units);
    }
    int idx = buddy2_alloc(pool->buddy, num_units);
    if (idx < 0) {
        return NULL;
    }
    /*
     * update statistics..
     */
    pool->buddy2_used_mem += (num_units * SIZE_PER_UNIT);
    pool->buddy2_real_used_mem += bytes;

    void *ret = pool->data + idx*SIZE_PER_UNIT;
    return ret;
}
/*
 * because we need to update the statistics information,
 * so we must pass a size argument to this function.
 */
void shm_free(shm_allocator_t *pool, void *ptr, long sz) {
    // update statistics
    pool->buddy2_real_used_mem -= sz;
    long num_units = NUM_UNITS(sz);
    if (!IS_POWER_OF_2(num_units)) {
        num_units = fixsize(num_units);
    }
    pool->buddy2_used_mem -= (num_units*SIZE_PER_UNIT);
    // free the memory from the buddy system
    int idx = (ptr-pool->data)/SIZE_PER_UNIT;
    buddy2_free(pool->buddy, idx);
}

int shm_mmap(int memSize) {
    shm_allocator_t *pp;
    pp = mmap(NULL, SHM_ALL_MEM_SIZE(memSize), PROT_READ|PROT_WRITE,
              MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    if (pp == MAP_FAILED) {
        return -1;
    }
    return 0;
}
