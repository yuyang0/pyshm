#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/queue.h>

#include "log.h"
#include "ring.h"

/* true if x is a power of 2 */
#define POWEROF2(x) ((((x)-1) & (x)) == 0)

/* return the size of memory occupied by a ring */
ssize_t
ring_get_memsize(unsigned count)
{
	ssize_t sz;

	/* count must be a power of 2 */
	if ((!POWEROF2(count)) || (count > RING_SZ_MASK )) {
		LOG_ERR(
			"Requested size is invalid, must be power of 2, and "
			"do not exceed the size limit %u\n", RING_SZ_MASK);
		return -EINVAL;
	}

	sz = sizeof(struct ring) + count * sizeof(void *);
	sz = ALIGN(sz, CACHE_LINE_SIZE);
	return sz;
}

int
ring_init(struct ring *r, unsigned count, unsigned flags)
{

	/* compilation-time checks */
	BUILD_BUG_ON((sizeof(struct ring) &
			  CACHE_LINE_MASK) != 0);
	BUILD_BUG_ON((offsetof(struct ring, cons) &
			  CACHE_LINE_MASK) != 0);
	BUILD_BUG_ON((offsetof(struct ring, prod) &
			  CACHE_LINE_MASK) != 0);

	/* init the ring structure */
	memset(r, 0, sizeof(*r));

	r->flags = flags;
	r->prod.single = (flags & RING_F_SP_ENQ) ? __IS_SP : __IS_MP;
	r->cons.single = (flags & RING_F_SC_DEQ) ? __IS_SC : __IS_MC;
	r->size = count;
	r->mask = count - 1;
	r->prod.head = r->cons.head = 0;
	r->prod.tail = r->cons.tail = 0;

	return 0;
}

/* create the ring */
struct ring *
ring_create(shm_allocator_t *shm, unsigned count, unsigned flags)
{
	struct ring *r;
	ssize_t ring_size;

	ring_size = ring_get_memsize(count);
	if (ring_size < 0) {
		errno = ring_size;
		return NULL;
	}

	/* reserve a memory zone for this ring. If we can't get config or
	 * we are secondary process, the memzone_reserve function will set
	 * errno for us appropriately - hence no check in this this function */
	r = shm_alloc(shm, ring_size);
	if (r != NULL) {
		ring_init(r, count, flags);
    r->shm = shm;
	} else {
		r = NULL;
		LOG_ERR("Cannot allocate memory\n");
	}
	return r;
}

/* free the ring */
void
ring_destroy(struct ring *r)
{
	if (r == NULL)
		return;

	/*
	 * Ring was not created with ring_create,
	 * therefore, there is no allocator to free.
	 */
	if (r->shm == NULL) {
		LOG_ERR("Cannot destroy ring (not created with ring_create()");
		return;
	}

	ssize_t ring_size;

	ring_size = ring_get_memsize(r->size);

	shm_free(r->shm, r, ring_size);
}

/* dump the status of the ring on the console */
void
ring_dump(FILE *f, const struct ring *r)
{
	fprintf(f, "  flags=%x\n", r->flags);
	fprintf(f, "  size=%"PRIu32"\n", r->size);
	fprintf(f, "  ct=%"PRIu32"\n", r->cons.tail);
	fprintf(f, "  ch=%"PRIu32"\n", r->cons.head);
	fprintf(f, "  pt=%"PRIu32"\n", r->prod.tail);
	fprintf(f, "  ph=%"PRIu32"\n", r->prod.head);
	fprintf(f, "  used=%u\n", ring_count(r));
	fprintf(f, "  avail=%u\n", ring_free_count(r));
}
