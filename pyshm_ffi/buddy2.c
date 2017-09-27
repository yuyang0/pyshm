#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "buddy2.h"

#define LEFT_LEAF(index) ((index) * 2 + 1)
#define RIGHT_LEAF(index) ((index) * 2 + 2)
#define PARENT(index) ( ((index) + 1) / 2 - 1)

#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define ALLOC malloc
#define FREE free

int floor2pow(int num) {
    int i = 0;
    for (i = 0; ; ++i) {
        num = num>>1;
        if (num == 0) {
            break;
        }
    }
    int ret = 1;
    for (int j = 0; j < i; ++j) {
        ret = ret << 1;
    }
    return ret;
}

unsigned fixsize(unsigned size) {
    size |= size >> 1;
    size |= size >> 2;
    size |= size >> 4;
    size |= size >> 8;
    size |= size >> 16;
    return size + 1;
}

struct buddy2 *buddy2_new(int size, void *mem) {
    struct buddy2 *self;
    unsigned node_size;
    int i;

    if (size < 1 || !IS_POWER_OF_2(size)) {
        fprintf(stderr, "the size(%d) is not power of 2", size);
        return NULL;
    }

    if (mem == NULL) {
        self = (buddy2_t *) ALLOC(BUDDY2_DS_SIZE(size));
        self->needFree = true;
    } else {
        self = (buddy2_t *) mem;
        self->needFree = false;
    }

    self->size = size;
    node_size = size * 2;

    for (i = 0; i < 2 * size - 1; ++i) {
        if (IS_POWER_OF_2(i + 1))
            node_size /= 2;
        self->longest[i] = node_size;
    }

    return self;
}

void buddy2_destroy(struct buddy2 *self) {
    if (self->needFree) {
        FREE(self);
    }
}

int buddy2_alloc(struct buddy2 *self, int size) {
    unsigned index = 0;
    unsigned node_size;
    unsigned offset = 0;

    if (self == NULL)
        return -1;

    if (size <= 0)
        size = 1;
    else if (!IS_POWER_OF_2(size))
        size = fixsize(size);

    if (self->longest[index] < size)
        return -1;

    for (node_size = self->size; node_size != size; node_size /= 2) {
        if (self->longest[LEFT_LEAF(index)] >= size)
            index = LEFT_LEAF(index);
        else
            index = RIGHT_LEAF(index);
    }

    self->longest[index] = 0;
    offset = (index + 1) * node_size - self->size;

    while (index) {
        index = PARENT(index);
        self->longest[index] =
                MAX(self->longest[LEFT_LEAF(index)], self->longest[RIGHT_LEAF(index)]);
    }

    return offset;
}

void buddy2_free(struct buddy2 *self, int offset) {
    unsigned node_size, index = 0;
    unsigned left_longest, right_longest;

    assert(self && offset >= 0 && offset < self->size);

    node_size = 1;
    index = offset + self->size - 1;

    for (; self->longest[index]; index = PARENT(index)) {
        node_size *= 2;
        if (index == 0)
            return;
    }

    self->longest[index] = node_size;

    while (index) {
        index = PARENT(index);
        node_size *= 2;

        left_longest = self->longest[LEFT_LEAF(index)];
        right_longest = self->longest[RIGHT_LEAF(index)];

        if (left_longest + right_longest == node_size)
            self->longest[index] = node_size;
        else
            self->longest[index] = MAX(left_longest, right_longest);
    }
}

int buddy2_size(struct buddy2 *self, int offset) {
    unsigned node_size, index = 0;

    assert(self && offset >= 0 && offset < self->size);

    node_size = 1;
    for (index = offset + self->size - 1; self->longest[index]; index = PARENT(index))
        node_size *= 2;

    return node_size;
}


void buddy2_dump(buddy2_t *self) {
    char canvas[65];
    int i, j;
    unsigned node_size, offset;

    if (self == NULL) {
        printf("buddy2_dump: (struct buddy2*)self == NULL");
        return;
    }

    if (self->size > 64) {
        printf("buddy2_dump: (struct buddy2*)self is too big to dump");
        return;
    }

    memset(canvas, '_', sizeof(canvas));
    node_size = self->size * 2;

    for (i = 0; i < 2 * self->size - 1; ++i) {
        if (IS_POWER_OF_2(i + 1))
            node_size /= 2;

        if (self->longest[i] == 0) {
            if (i >= self->size - 1) {
                canvas[i - self->size + 1] = '*';
            }
            else if (self->longest[LEFT_LEAF(i)] && self->longest[RIGHT_LEAF(i)]) {
                offset = (i + 1) * node_size - self->size;

                for (j = offset; j < offset + node_size; ++j)
                    canvas[j] = '*';
            }
        }
    }
    canvas[self->size] = '\0';
    puts(canvas);
}
