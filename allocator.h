enum allocKind { AK_STD, AK_STACK };

struct allocAllocator;

void * alloc_malloc(struct allocAllocator * a, size_t size);
void * alloc_realloc(struct allocAllocator * a,
        void * mem, size_t old_size, size_t new_size);
void alloc_free(struct allocAllocator * a, void * mem, size_t size);

struct allocAllocator * alloc_create_allocator(enum allocKind kind);
void alloc_free_allocator(enum allocKind kind, struct allocAllocator * a);

