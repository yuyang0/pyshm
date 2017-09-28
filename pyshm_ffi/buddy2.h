#ifndef __BUDDY2_H__
#define __BUDDY2_H__

#include <stdlib.h>
#include <stdbool.h>

typedef struct buddy2 {
    bool needFree;
    unsigned size;
    unsigned longest[0];
} buddy2_t;

#define IS_POWER_OF_2(x) (!((x)&((x)-1)))
unsigned fixsize(unsigned size);

#define BUDDY2_DS_SIZE(sz) (sizeof(struct buddy2) + (2*(sz)-1)*sizeof(unsigned))

struct buddy2 *buddy2_new(int size, void *mem);

void buddy2_destroy(struct buddy2 *self);

int buddy2_alloc(struct buddy2 *self, unsigned size);

void buddy2_free(struct buddy2 *self, int offset);

int buddy2_size(struct buddy2 *self, int offset);

void buddy2_dump(buddy2_t *self);

#endif//__BUDDY2_H__
