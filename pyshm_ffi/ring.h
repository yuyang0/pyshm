#ifndef _RING_H_
#define _RING_H_

/**
 * @file
 * RTE Ring
 *
 * The Ring Manager is a fixed-size queue, implemented as a table of
 * pointers. Head and tail pointers are modified atomically, allowing
 * concurrent access to it. It has the following features:
 *
 * - FIFO (First In First Out)
 * - Maximum size is fixed; the pointers are stored in a table.
 * - Lockless implementation.
 * - Multi- or single-consumer dequeue.
 * - Multi- or single-producer enqueue.
 * - Bulk dequeue.
 * - Bulk enqueue.
 *
 * Note: the ring implementation is not preemptable. A lcore must not
 * be interrupted by another task that uses the same ring.
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include <sys/queue.h>
#include <errno.h>

#include "atomic.h"
#include "utils.h"
#include "allocator.h"


enum ring_queue_behavior {
	RING_QUEUE_FIXED = 0, /* Enq/Deq a fixed number of items from a ring */
	RING_QUEUE_VARIABLE   /* Enq/Deq as many items as possible from ring */
};

#if CACHE_LINE_SIZE < 128
#define PROD_ALIGN (CACHE_LINE_SIZE * 2)
#define CONS_ALIGN (CACHE_LINE_SIZE * 2)
#else
#define PROD_ALIGN CACHE_LINE_SIZE
#define CONS_ALIGN CACHE_LINE_SIZE
#endif

/* structure to hold a pair of head/tail values and other metadata */
struct ring_headtail {
	volatile uint32_t head;  /**< Prod/consumer head. */
	volatile uint32_t tail;  /**< Prod/consumer tail. */
	uint32_t single;         /**< True if single prod/cons */
};

/**
 * An RTE ring structure.
 *
 * The producer and the consumer have a head and a tail index. The particularity
 * of these index is that they are not between 0 and size(ring). These indexes
 * are between 0 and 2^32, and we mask their value when we access the ring[]
 * field. Thanks to this assumption, we can do subtractions between 2 index
 * values in a modulo-32bit base: that's why the overflow of the indexes is not
 * a problem.
 */
struct ring {
	int flags;               /**< Flags supplied at creation. */
  shm_allocator_t *shm;
	uint32_t size;           /**< Size of ring. */
	uint32_t mask;           /**< Mask (size-1) of ring. */

	/** Ring producer status. */
	struct ring_headtail prod __aligned(PROD_ALIGN);

	/** Ring consumer status. */
	struct ring_headtail cons __aligned(CONS_ALIGN);
};

#define RING_F_SP_ENQ 0x0001 /**< The default enqueue is "single-producer". */
#define RING_F_SC_DEQ 0x0002 /**< The default dequeue is "single-consumer". */
#define RING_SZ_MASK  (unsigned)(0x0fffffff) /**< Ring size mask */

/* @internal defines for passing to the enqueue dequeue worker functions */
#define __IS_SP 1
#define __IS_MP 0
#define __IS_SC 1
#define __IS_MC 0

/**
 * Calculate the memory size needed for a ring
 *
 * This function returns the number of bytes needed for a ring, given
 * the number of elements in it. This value is the sum of the size of
 * the structure ring and the size of the memory needed by the
 * objects pointers. The value is aligned to a cache line size.
 *
 * @param count
 *   The number of elements in the ring (must be a power of 2).
 * @return
 *   - The memory size needed for the ring on success.
 *   - -EINVAL if count is not a power of 2.
 */
ssize_t ring_get_memsize(unsigned count);

/**
 * Initialize a ring structure.
 *
 * Initialize a ring structure in memory pointed by "r". The size of the
 * memory area must be large enough to store the ring structure and the
 * object table. It is advised to use ring_get_memsize() to get the
 * appropriate size.
 *
 * The ring size is set to *count*, which must be a power of two. Water
 * marking is disabled by default. The real usable ring size is
 * *count-1* instead of *count* to differentiate a free ring from an
 * empty ring.
 *
 * The ring is not added in TAILQ_RING global list. Indeed, the
 * memory given by the caller may not be shareable among dpdk
 * processes.
 *
 * @param r
 *   The pointer to the ring structure followed by the objects table.
 * @param name
 *   The name of the ring.
 * @param count
 *   The number of elements in the ring (must be a power of 2).
 * @param flags
 *   An OR of the following:
 *    - RING_F_SP_ENQ: If this flag is set, the default behavior when
 *      using ``ring_enqueue()`` or ``ring_enqueue_bulk()``
 *      is "single-producer". Otherwise, it is "multi-producers".
 *    - RING_F_SC_DEQ: If this flag is set, the default behavior when
 *      using ``ring_dequeue()`` or ``ring_dequeue_bulk()``
 *      is "single-consumer". Otherwise, it is "multi-consumers".
 * @return
 *   0 on success, or a negative value on error.
 */
int ring_init(struct ring *r, unsigned count, unsigned flags);

/**
 * Create a new ring named *name* in memory.
 *
 * This function uses ``memzone_reserve()`` to allocate memory. Then it
 * calls ring_init() to initialize an empty ring.
 *
 * The new ring size is set to *count*, which must be a power of
 * two. Water marking is disabled by default. The real usable ring size
 * is *count-1* instead of *count* to differentiate a free ring from an
 * empty ring.
 *
 * The ring is added in TAILQ_RING list.
 *
 * @param name
 *   The name of the ring.
 * @param count
 *   The size of the ring (must be a power of 2).
 * @param socket_id
 *   The *socket_id* argument is the socket identifier in case of
 *   NUMA. The value can be *SOCKET_ID_ANY* if there is no NUMA
 *   constraint for the reserved zone.
 * @param flags
 *   An OR of the following:
 *    - RING_F_SP_ENQ: If this flag is set, the default behavior when
 *      using ``ring_enqueue()`` or ``ring_enqueue_bulk()``
 *      is "single-producer". Otherwise, it is "multi-producers".
 *    - RING_F_SC_DEQ: If this flag is set, the default behavior when
 *      using ``ring_dequeue()`` or ``ring_dequeue_bulk()``
 *      is "single-consumer". Otherwise, it is "multi-consumers".
 * @return
 *   On success, the pointer to the new allocated ring. NULL on error with
 *    errno set appropriately. Possible errno values include:
 *    - E_NO_CONFIG - function could not get pointer to config structure
 *    - E_SECONDARY - function was called from a secondary process instance
 *    - EINVAL - count provided is not a power of 2
 *    - ENOSPC - the maximum number of memzones has already been allocated
 *    - EEXIST - a memzone with the same name already exists
 *    - ENOMEM - no appropriate memory area found in which to create memzone
 */

struct ring *ring_create(shm_allocator_t *shm, unsigned count, unsigned flags);
/**
 * De-allocate all memory used by the ring.
 *
 * @param r
 *   Ring to free
 */
void ring_destroy(struct ring *r);

/**
 * Dump the status of the ring to a file.
 *
 * @param f
 *   A pointer to a file for output
 * @param r
 *   A pointer to the ring structure.
 */
void ring_dump(FILE *f, const struct ring *r);

/* the actual enqueue of pointers on the ring.
 * Placed here since identical code needed in both
 * single and multi producer enqueue functions */
#define ENQUEUE_PTRS(r, ring_start, prod_head, obj_table, n, obj_type) do { \
	unsigned int i; \
	const uint32_t size = (r)->size; \
	uint32_t idx = prod_head & (r)->mask; \
	obj_type *ring = (obj_type *)ring_start; \
	if (likely(idx + n < size)) { \
		for (i = 0; i < (n & ((~(unsigned)0x3))); i+=4, idx+=4) { \
			ring[idx] = obj_table[i]; \
			ring[idx+1] = obj_table[i+1]; \
			ring[idx+2] = obj_table[i+2]; \
			ring[idx+3] = obj_table[i+3]; \
		} \
		switch (n & 0x3) { \
		case 3: \
			ring[idx++] = obj_table[i++]; /* fallthrough */ \
		case 2: \
			ring[idx++] = obj_table[i++]; /* fallthrough */ \
		case 1: \
			ring[idx++] = obj_table[i++]; \
		} \
	} else { \
		for (i = 0; idx < size; i++, idx++)\
			ring[idx] = obj_table[i]; \
		for (idx = 0; i < n; i++, idx++) \
			ring[idx] = obj_table[i]; \
	} \
} while (0)

/* the actual copy of pointers on the ring to obj_table.
 * Placed here since identical code needed in both
 * single and multi consumer dequeue functions */
#define DEQUEUE_PTRS(r, ring_start, cons_head, obj_table, n, obj_type) do { \
	unsigned int i; \
	uint32_t idx = cons_head & (r)->mask; \
	const uint32_t size = (r)->size; \
	obj_type *ring = (obj_type *)ring_start; \
	if (likely(idx + n < size)) { \
		for (i = 0; i < (n & (~(unsigned)0x3)); i+=4, idx+=4) {\
			obj_table[i] = ring[idx]; \
			obj_table[i+1] = ring[idx+1]; \
			obj_table[i+2] = ring[idx+2]; \
			obj_table[i+3] = ring[idx+3]; \
		} \
		switch (n & 0x3) { \
		case 3: \
			obj_table[i++] = ring[idx++]; /* fallthrough */ \
		case 2: \
			obj_table[i++] = ring[idx++]; /* fallthrough */ \
		case 1: \
			obj_table[i++] = ring[idx++]; \
		} \
	} else { \
		for (i = 0; idx < size; i++, idx++) \
			obj_table[i] = ring[idx]; \
		for (idx = 0; i < n; i++, idx++) \
			obj_table[i] = ring[idx]; \
	} \
} while (0)

static inline __attribute__((always_inline)) void
update_tail(struct ring_headtail *ht, uint32_t old_val, uint32_t new_val,
		uint32_t single)
{
	/*
	 * If there are other enqueues/dequeues in progress that preceded us,
	 * we need to wait for them to complete
	 */
	if (!single)
		while (unlikely(ht->tail != old_val))
			shm_pause();

	ht->tail = new_val;
}

/**
 * @internal This function updates the producer head for enqueue
 *
 * @param r
 *   A pointer to the ring structure
 * @param is_sp
 *   Indicates whether multi-producer path is needed or not
 * @param n
 *   The number of elements we will want to enqueue, i.e. how far should the
 *   head be moved
 * @param behavior
 *   RING_QUEUE_FIXED:    Enqueue a fixed number of items from a ring
 *   RING_QUEUE_VARIABLE: Enqueue as many items as possible from ring
 * @param old_head
 *   Returns head value as it was before the move, i.e. where enqueue starts
 * @param new_head
 *   Returns the current/new head value i.e. where enqueue finishes
 * @param free_entries
 *   Returns the amount of free space in the ring BEFORE head was moved
 * @return
 *   Actual number of objects enqueued.
 *   If behavior == RING_QUEUE_FIXED, this will be 0 or n only.
 */
static inline __attribute__((always_inline)) unsigned int
__ring_move_prod_head(struct ring *r, int is_sp,
		unsigned int n, enum ring_queue_behavior behavior,
		uint32_t *old_head, uint32_t *new_head,
		uint32_t *free_entries)
{
	const uint32_t mask = r->mask;
	unsigned int max = n;
	int success;

	do {
		/* Reset n to the initial burst count */
		n = max;

		*old_head = r->prod.head;
		const uint32_t cons_tail = r->cons.tail;
		/* The subtraction is done between two unsigned 32bits value
		 * (the result is always modulo 32 bits even if we have
		 * *old_head > cons_tail). So 'free_entries' is always between 0
		 * and size(ring)-1. */
		*free_entries = (mask + cons_tail - *old_head);
    /* fprintf(stderr, "old_head %d, mask %d, cons_tail %d, n %d\n", */
    /*         *old_head, mask, cons_tail, n); */

		/* check that we have enough room in ring */
		if (unlikely(n > *free_entries))
			n = (behavior == RING_QUEUE_FIXED) ?
					0 : *free_entries;

		if (n == 0)
			return 0;

		*new_head = *old_head + n;
		if (is_sp)
			r->prod.head = *new_head, success = 1;
		else
			success = atomic32_cmpset(&r->prod.head,
					*old_head, *new_head);
	} while (unlikely(success == 0));
	return n;
}

/**
 * @internal Enqueue several objects on the ring
 *
  * @param r
 *   A pointer to the ring structure.
 * @param obj_table
 *   A pointer to a table of void * pointers (objects).
 * @param n
 *   The number of objects to add in the ring from the obj_table.
 * @param behavior
 *   RING_QUEUE_FIXED:    Enqueue a fixed number of items from a ring
 *   RING_QUEUE_VARIABLE: Enqueue as many items as possible from ring
 * @param is_sp
 *   Indicates whether to use single producer or multi-producer head update
 * @param free_space
 *   returns the amount of space after the enqueue operation has finished
 * @return
 *   Actual number of objects enqueued.
 *   If behavior == RING_QUEUE_FIXED, this will be 0 or n only.
 */
static inline __attribute__((always_inline)) unsigned int
__ring_do_enqueue(struct ring *r, void * const *obj_table,
		 unsigned int n, enum ring_queue_behavior behavior,
		 int is_sp, unsigned int *free_space)
{
	uint32_t prod_head, prod_next;
	uint32_t free_entries;

	n = __ring_move_prod_head(r, is_sp, n, behavior,
			&prod_head, &prod_next, &free_entries);
	if (n == 0)
		goto end;

	ENQUEUE_PTRS(r, &r[1], prod_head, obj_table, n, void *);
	smp_wmb();

	update_tail(&r->prod, prod_head, prod_next, is_sp);
end:
  /* LOG_DEBUG(stderr, "n is zero\n"); */
	if (free_space != NULL)
		*free_space = free_entries - n;
	return n;
}

/**
 * @internal This function updates the consumer head for dequeue
 *
 * @param r
 *   A pointer to the ring structure
 * @param is_sc
 *   Indicates whether multi-consumer path is needed or not
 * @param n
 *   The number of elements we will want to enqueue, i.e. how far should the
 *   head be moved
 * @param behavior
 *   RING_QUEUE_FIXED:    Dequeue a fixed number of items from a ring
 *   RING_QUEUE_VARIABLE: Dequeue as many items as possible from ring
 * @param old_head
 *   Returns head value as it was before the move, i.e. where dequeue starts
 * @param new_head
 *   Returns the current/new head value i.e. where dequeue finishes
 * @param entries
 *   Returns the number of entries in the ring BEFORE head was moved
 * @return
 *   - Actual number of objects dequeued.
 *     If behavior == RING_QUEUE_FIXED, this will be 0 or n only.
 */
static inline __attribute__((always_inline)) unsigned int
__ring_move_cons_head(struct ring *r, int is_sc,
		unsigned int n, enum ring_queue_behavior behavior,
		uint32_t *old_head, uint32_t *new_head,
		uint32_t *entries)
{
	unsigned int max = n;
	int success;

	/* move cons.head atomically */
	do {
		/* Restore n as it may change every loop */
		n = max;

		*old_head = r->cons.head;
		const uint32_t prod_tail = r->prod.tail;
		/* The subtraction is done between two unsigned 32bits value
		 * (the result is always modulo 32 bits even if we have
		 * cons_head > prod_tail). So 'entries' is always between 0
		 * and size(ring)-1. */
		*entries = (prod_tail - *old_head);

		/* Set the actual entries for dequeue */
		if (n > *entries)
			n = (behavior == RING_QUEUE_FIXED) ? 0 : *entries;

		if (unlikely(n == 0))
			return 0;

		*new_head = *old_head + n;
		if (is_sc)
			r->cons.head = *new_head, success = 1;
		else
			success = atomic32_cmpset(&r->cons.head, *old_head,
					*new_head);
	} while (unlikely(success == 0));
	return n;
}

/**
 * @internal Dequeue several objects from the ring
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj_table
 *   A pointer to a table of void * pointers (objects).
 * @param n
 *   The number of objects to pull from the ring.
 * @param behavior
 *   RING_QUEUE_FIXED:    Dequeue a fixed number of items from a ring
 *   RING_QUEUE_VARIABLE: Dequeue as many items as possible from ring
 * @param is_sc
 *   Indicates whether to use single consumer or multi-consumer head update
 * @param available
 *   returns the number of remaining ring entries after the dequeue has finished
 * @return
 *   - Actual number of objects dequeued.
 *     If behavior == RING_QUEUE_FIXED, this will be 0 or n only.
 */
static inline __attribute__((always_inline)) unsigned int
__ring_do_dequeue(struct ring *r, void **obj_table,
		 unsigned int n, enum ring_queue_behavior behavior,
		 int is_sc, unsigned int *available)
{
	uint32_t cons_head, cons_next;
	uint32_t entries;

	n = __ring_move_cons_head(r, is_sc, n, behavior,
			&cons_head, &cons_next, &entries);
	if (n == 0)
		goto end;

	DEQUEUE_PTRS(r, &r[1], cons_head, obj_table, n, void *);
	smp_rmb();

	update_tail(&r->cons, cons_head, cons_next, is_sc);

end:
	if (available != NULL)
		*available = entries - n;
	return n;
}

/**
 * Enqueue several objects on the ring (multi-producers safe).
 *
 * This function uses a "compare and set" instruction to move the
 * producer index atomically.
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj_table
 *   A pointer to a table of void * pointers (objects).
 * @param n
 *   The number of objects to add in the ring from the obj_table.
 * @param free_space
 *   if non-NULL, returns the amount of space in the ring after the
 *   enqueue operation has finished.
 * @return
 *   The number of objects enqueued, either 0 or n
 */
static inline unsigned int __attribute__((always_inline))
ring_mp_enqueue_bulk(struct ring *r, void * const *obj_table,
			 unsigned int n, unsigned int *free_space)
{
	return __ring_do_enqueue(r, obj_table, n, RING_QUEUE_FIXED,
			__IS_MP, free_space);
}

/**
 * Enqueue several objects on a ring (NOT multi-producers safe).
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj_table
 *   A pointer to a table of void * pointers (objects).
 * @param n
 *   The number of objects to add in the ring from the obj_table.
 * @param free_space
 *   if non-NULL, returns the amount of space in the ring after the
 *   enqueue operation has finished.
 * @return
 *   The number of objects enqueued, either 0 or n
 */
static inline unsigned int __attribute__((always_inline))
ring_sp_enqueue_bulk(struct ring *r, void * const *obj_table,
			 unsigned int n, unsigned int *free_space)
{
	return __ring_do_enqueue(r, obj_table, n, RING_QUEUE_FIXED,
			__IS_SP, free_space);
}

/**
 * Enqueue several objects on a ring.
 *
 * This function calls the multi-producer or the single-producer
 * version depending on the default behavior that was specified at
 * ring creation time (see flags).
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj_table
 *   A pointer to a table of void * pointers (objects).
 * @param n
 *   The number of objects to add in the ring from the obj_table.
 * @param free_space
 *   if non-NULL, returns the amount of space in the ring after the
 *   enqueue operation has finished.
 * @return
 *   The number of objects enqueued, either 0 or n
 */
static inline unsigned int __attribute__((always_inline))
ring_enqueue_bulk(struct ring *r, void * const *obj_table,
		      unsigned int n, unsigned int *free_space)
{
	return __ring_do_enqueue(r, obj_table, n, RING_QUEUE_FIXED,
			r->prod.single, free_space);
}

/**
 * Enqueue one object on a ring (multi-producers safe).
 *
 * This function uses a "compare and set" instruction to move the
 * producer index atomically.
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj
 *   A pointer to the object to be added.
 * @return
 *   - 0: Success; objects enqueued.
 *   - -ENOBUFS: Not enough room in the ring to enqueue; no object is enqueued.
 */
static inline int __attribute__((always_inline))
ring_mp_enqueue(struct ring *r, void *obj)
{
	return ring_mp_enqueue_bulk(r, &obj, 1, NULL) ? 0 : -ENOBUFS;
}

/**
 * Enqueue one object on a ring (NOT multi-producers safe).
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj
 *   A pointer to the object to be added.
 * @return
 *   - 0: Success; objects enqueued.
 *   - -ENOBUFS: Not enough room in the ring to enqueue; no object is enqueued.
 */
static inline int __attribute__((always_inline))
ring_sp_enqueue(struct ring *r, void *obj)
{
	return ring_sp_enqueue_bulk(r, &obj, 1, NULL) ? 0 : -ENOBUFS;
}

/**
 * Enqueue one object on a ring.
 *
 * This function calls the multi-producer or the single-producer
 * version, depending on the default behaviour that was specified at
 * ring creation time (see flags).
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj
 *   A pointer to the object to be added.
 * @return
 *   - 0: Success; objects enqueued.
 *   - -ENOBUFS: Not enough room in the ring to enqueue; no object is enqueued.
 */
static inline int __attribute__((always_inline))
ring_enqueue(struct ring *r, void *obj)
{
	return ring_enqueue_bulk(r, &obj, 1, NULL) ? 0 : -ENOBUFS;
}

/**
 * Dequeue several objects from a ring (multi-consumers safe).
 *
 * This function uses a "compare and set" instruction to move the
 * consumer index atomically.
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj_table
 *   A pointer to a table of void * pointers (objects) that will be filled.
 * @param n
 *   The number of objects to dequeue from the ring to the obj_table.
 * @param available
 *   If non-NULL, returns the number of remaining ring entries after the
 *   dequeue has finished.
 * @return
 *   The number of objects dequeued, either 0 or n
 */
static inline unsigned int __attribute__((always_inline))
ring_mc_dequeue_bulk(struct ring *r, void **obj_table,
		unsigned int n, unsigned int *available)
{
	return __ring_do_dequeue(r, obj_table, n, RING_QUEUE_FIXED,
			__IS_MC, available);
}

/**
 * Dequeue several objects from a ring (NOT multi-consumers safe).
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj_table
 *   A pointer to a table of void * pointers (objects) that will be filled.
 * @param n
 *   The number of objects to dequeue from the ring to the obj_table,
 *   must be strictly positive.
 * @param available
 *   If non-NULL, returns the number of remaining ring entries after the
 *   dequeue has finished.
 * @return
 *   The number of objects dequeued, either 0 or n
 */
static inline unsigned int __attribute__((always_inline))
ring_sc_dequeue_bulk(struct ring *r, void **obj_table,
		unsigned int n, unsigned int *available)
{
	return __ring_do_dequeue(r, obj_table, n, RING_QUEUE_FIXED,
			__IS_SC, available);
}

/**
 * Dequeue several objects from a ring.
 *
 * This function calls the multi-consumers or the single-consumer
 * version, depending on the default behaviour that was specified at
 * ring creation time (see flags).
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj_table
 *   A pointer to a table of void * pointers (objects) that will be filled.
 * @param n
 *   The number of objects to dequeue from the ring to the obj_table.
 * @param available
 *   If non-NULL, returns the number of remaining ring entries after the
 *   dequeue has finished.
 * @return
 *   The number of objects dequeued, either 0 or n
 */
static inline unsigned int __attribute__((always_inline))
ring_dequeue_bulk(struct ring *r, void **obj_table, unsigned int n,
		unsigned int *available)
{
	return __ring_do_dequeue(r, obj_table, n, RING_QUEUE_FIXED,
				r->cons.single, available);
}

/**
 * Dequeue one object from a ring (multi-consumers safe).
 *
 * This function uses a "compare and set" instruction to move the
 * consumer index atomically.
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj_p
 *   A pointer to a void * pointer (object) that will be filled.
 * @return
 *   - 0: Success; objects dequeued.
 *   - -ENOENT: Not enough entries in the ring to dequeue; no object is
 *     dequeued.
 */
static inline int __attribute__((always_inline))
ring_mc_dequeue(struct ring *r, void **obj_p)
{
	return ring_mc_dequeue_bulk(r, obj_p, 1, NULL)  ? 0 : -ENOBUFS;
}

/**
 * Dequeue one object from a ring (NOT multi-consumers safe).
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj_p
 *   A pointer to a void * pointer (object) that will be filled.
 * @return
 *   - 0: Success; objects dequeued.
 *   - -ENOENT: Not enough entries in the ring to dequeue, no object is
 *     dequeued.
 */
static inline int __attribute__((always_inline))
ring_sc_dequeue(struct ring *r, void **obj_p)
{
	return ring_sc_dequeue_bulk(r, obj_p, 1, NULL) ? 0 : -ENOBUFS;
}

/**
 * Dequeue one object from a ring.
 *
 * This function calls the multi-consumers or the single-consumer
 * version depending on the default behaviour that was specified at
 * ring creation time (see flags).
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj_p
 *   A pointer to a void * pointer (object) that will be filled.
 * @return
 *   - 0: Success, objects dequeued.
 *   - -ENOENT: Not enough entries in the ring to dequeue, no object is
 *     dequeued.
 */
static inline int __attribute__((always_inline))
ring_dequeue(struct ring *r, void **obj_p)
{
	return ring_dequeue_bulk(r, obj_p, 1, NULL) ? 0 : -ENOENT;
}

/**
 * Test if a ring is full.
 *
 * @param r
 *   A pointer to the ring structure.
 * @return
 *   - 1: The ring is full.
 *   - 0: The ring is not full.
 */
static inline int
ring_full(const struct ring *r)
{
	uint32_t prod_tail = r->prod.tail;
	uint32_t cons_tail = r->cons.tail;
	return ((cons_tail - prod_tail - 1) & r->mask) == 0;
}

/**
 * Test if a ring is empty.
 *
 * @param r
 *   A pointer to the ring structure.
 * @return
 *   - 1: The ring is empty.
 *   - 0: The ring is not empty.
 */
static inline int
ring_empty(const struct ring *r)
{
	uint32_t prod_tail = r->prod.tail;
	uint32_t cons_tail = r->cons.tail;
	return !!(cons_tail == prod_tail);
}

/**
 * Return the number of entries in a ring.
 *
 * @param r
 *   A pointer to the ring structure.
 * @return
 *   The number of entries in the ring.
 */
static inline unsigned
ring_count(const struct ring *r)
{
	uint32_t prod_tail = r->prod.tail;
	uint32_t cons_tail = r->cons.tail;
	return (prod_tail - cons_tail) & r->mask;
}

/**
 * Return the number of free entries in a ring.
 *
 * @param r
 *   A pointer to the ring structure.
 * @return
 *   The number of free entries in the ring.
 */
static inline unsigned
ring_free_count(const struct ring *r)
{
	uint32_t prod_tail = r->prod.tail;
	uint32_t cons_tail = r->cons.tail;
	return (cons_tail - prod_tail - 1) & r->mask;
}

/**
 * Return the size of the ring.
 *
 * @param r
 *   A pointer to the ring structure.
 * @return
 *   The number of elements which can be stored in the ring.
 */
static inline unsigned int
ring_get_size(const struct ring *r)
{
	return r->size;
}

/**
 * Enqueue several objects on the ring (multi-producers safe).
 *
 * This function uses a "compare and set" instruction to move the
 * producer index atomically.
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj_table
 *   A pointer to a table of void * pointers (objects).
 * @param n
 *   The number of objects to add in the ring from the obj_table.
 * @param free_space
 *   if non-NULL, returns the amount of space in the ring after the
 *   enqueue operation has finished.
 * @return
 *   - n: Actual number of objects enqueued.
 */
static inline unsigned __attribute__((always_inline))
ring_mp_enqueue_burst(struct ring *r, void * const *obj_table,
			 unsigned int n, unsigned int *free_space)
{
	return __ring_do_enqueue(r, obj_table, n,
			RING_QUEUE_VARIABLE, __IS_MP, free_space);
}

/**
 * Enqueue several objects on a ring (NOT multi-producers safe).
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj_table
 *   A pointer to a table of void * pointers (objects).
 * @param n
 *   The number of objects to add in the ring from the obj_table.
 * @param free_space
 *   if non-NULL, returns the amount of space in the ring after the
 *   enqueue operation has finished.
 * @return
 *   - n: Actual number of objects enqueued.
 */
static inline unsigned __attribute__((always_inline))
ring_sp_enqueue_burst(struct ring *r, void * const *obj_table,
			 unsigned int n, unsigned int *free_space)
{
	return __ring_do_enqueue(r, obj_table, n,
			RING_QUEUE_VARIABLE, __IS_SP, free_space);
}

/**
 * Enqueue several objects on a ring.
 *
 * This function calls the multi-producer or the single-producer
 * version depending on the default behavior that was specified at
 * ring creation time (see flags).
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj_table
 *   A pointer to a table of void * pointers (objects).
 * @param n
 *   The number of objects to add in the ring from the obj_table.
 * @param free_space
 *   if non-NULL, returns the amount of space in the ring after the
 *   enqueue operation has finished.
 * @return
 *   - n: Actual number of objects enqueued.
 */
static inline unsigned __attribute__((always_inline))
ring_enqueue_burst(struct ring *r, void * const *obj_table,
		      unsigned int n, unsigned int *free_space)
{
	return __ring_do_enqueue(r, obj_table, n, RING_QUEUE_VARIABLE,
			r->prod.single, free_space);
}

/**
 * Dequeue several objects from a ring (multi-consumers safe). When the request
 * objects are more than the available objects, only dequeue the actual number
 * of objects
 *
 * This function uses a "compare and set" instruction to move the
 * consumer index atomically.
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj_table
 *   A pointer to a table of void * pointers (objects) that will be filled.
 * @param n
 *   The number of objects to dequeue from the ring to the obj_table.
 * @param available
 *   If non-NULL, returns the number of remaining ring entries after the
 *   dequeue has finished.
 * @return
 *   - n: Actual number of objects dequeued, 0 if ring is empty
 */
static inline unsigned __attribute__((always_inline))
ring_mc_dequeue_burst(struct ring *r, void **obj_table,
		unsigned int n, unsigned int *available)
{
	return __ring_do_dequeue(r, obj_table, n,
			RING_QUEUE_VARIABLE, __IS_MC, available);
}

/**
 * Dequeue several objects from a ring (NOT multi-consumers safe).When the
 * request objects are more than the available objects, only dequeue the
 * actual number of objects
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj_table
 *   A pointer to a table of void * pointers (objects) that will be filled.
 * @param n
 *   The number of objects to dequeue from the ring to the obj_table.
 * @param available
 *   If non-NULL, returns the number of remaining ring entries after the
 *   dequeue has finished.
 * @return
 *   - n: Actual number of objects dequeued, 0 if ring is empty
 */
static inline unsigned __attribute__((always_inline))
ring_sc_dequeue_burst(struct ring *r, void **obj_table,
		unsigned int n, unsigned int *available)
{
	return __ring_do_dequeue(r, obj_table, n,
			RING_QUEUE_VARIABLE, __IS_SC, available);
}

/**
 * Dequeue multiple objects from a ring up to a maximum number.
 *
 * This function calls the multi-consumers or the single-consumer
 * version, depending on the default behaviour that was specified at
 * ring creation time (see flags).
 *
 * @param r
 *   A pointer to the ring structure.
 * @param obj_table
 *   A pointer to a table of void * pointers (objects) that will be filled.
 * @param n
 *   The number of objects to dequeue from the ring to the obj_table.
 * @param available
 *   If non-NULL, returns the number of remaining ring entries after the
 *   dequeue has finished.
 * @return
 *   - Number of objects dequeued
 */
static inline unsigned __attribute__((always_inline))
ring_dequeue_burst(struct ring *r, void **obj_table,
		unsigned int n, unsigned int *available)
{
	return __ring_do_dequeue(r, obj_table, n,
				RING_QUEUE_VARIABLE,
				r->cons.single, available);
}

#ifdef __cplusplus
}
#endif

#endif /* _RING_H_ */
