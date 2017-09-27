#define 	RING_F_SP_ENQ        ...
#define 	RING_F_SC_DEQ        ...
/* #define 	RING_F_EXACT_SZ      ... */
#define 	RING_SZ_MASK     ...

#define 	MEMPOOL_F_SP_PUT   ...
#define 	MEMPOOL_F_SC_GET   ...

typedef struct shm_s {...;} shm_allocator_t;
struct ring {
    ...;
};
struct mempool {
    ...;
};

shm_allocator_t *shm_create(int size);
int shm_init(shm_allocator_t *pp, int size);
void *shm_alloc(shm_allocator_t *pool, int bytes);
void shm_free(shm_allocator_t *pool, void *p, long bytes);
int shm_destroy(shm_allocator_t *pool);


ssize_t ring_get_memsize (unsigned count);

struct ring *ring_create(shm_allocator_t *shm, unsigned count, unsigned flags);
int ring_init(struct ring *r, unsigned count, unsigned flags);
void 	ring_destroy (struct ring *r);
void 	ring_dump (FILE *f, const struct ring *r);

static inline unsigned int
ring_mp_enqueue_bulk (struct ring *r, void *const *obj_table, unsigned int n, unsigned int *free_space);

static inline unsigned int
ring_sp_enqueue_bulk (struct ring *r, void *const *obj_table, unsigned int n, unsigned int *free_space);

static inline unsigned int
ring_enqueue_bulk (struct ring *r, void *const *obj_table, unsigned int n, unsigned int *free_space);

static inline int
ring_mp_enqueue (struct ring *r, void *obj);

static inline int
ring_sp_enqueue (struct ring *r, void *obj);

static inline int
ring_enqueue (struct ring *r, void *obj);

static inline unsigned int
ring_mc_dequeue_bulk (struct ring *r, void **obj_table, unsigned int n, unsigned int *available);

static inline unsigned int
ring_sc_dequeue_bulk (struct ring *r, void **obj_table, unsigned int n, unsigned int *available);

static inline unsigned int
ring_dequeue_bulk (struct ring *r, void **obj_table, unsigned int n, unsigned int *available);

static inline int
ring_mc_dequeue (struct ring *r, void **obj_p);

static inline int
ring_sc_dequeue (struct ring *r, void **obj_p);

static inline int
ring_dequeue (struct ring *r, void **obj_p);

static unsigned 	ring_count (const struct ring *r);
static unsigned 	ring_free_count (const struct ring *r);
static int 	ring_full (const struct ring *r);
static int 	ring_empty (const struct ring *r);
static unsigned int 	ring_get_size (const struct ring *r);

static inline unsigned
ring_mp_enqueue_burst (struct ring *r, void *const *obj_table, unsigned int n, unsigned int *free_space);

static inline unsigned
    ring_sp_enqueue_burst (struct ring *r, void *const *obj_table, unsigned int n, unsigned int *free_space);

static inline unsigned
ring_enqueue_burst (struct ring *r, void *const *obj_table, unsigned int n, unsigned int *free_space);

static inline unsigned
ring_mc_dequeue_burst (struct ring *r, void **obj_table, unsigned int n, unsigned int *available);

static inline unsigned
ring_sc_dequeue_burst (struct ring *r, void **obj_table, unsigned int n, unsigned int *available);

static inline unsigned
ring_dequeue_burst (struct ring *r, void **obj_table, unsigned int n, unsigned int *available);

// mempool

struct mempool *mempool_create(shm_allocator_t *shm, unsigned n,
                                unsigned elt_size, unsigned flags);
void mempool_destroy(struct mempool *mp);

static inline int
mempool_get(struct mempool *mp, void **obj_p);

static inline int
mempool_sc_get(struct mempool *mp, void **obj_p);

static inline int
mempool_mc_get(struct mempool *mp, void **obj_p);

static inline int
mempool_get_bulk(struct mempool *mp, void **obj_tbl, unsigned n);

static inline int
mempool_put(struct mempool *mp, void *obj);

static inline int
mempool_sp_put(struct mempool *mp, void *obj);

static inline int
mempool_mp_put(struct mempool *mp, void *obj);

static inline int
mempool_put_bulk(struct mempool *mp, void **obj_tbl, unsigned n);
