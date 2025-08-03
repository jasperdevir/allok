#ifndef ALLOK_H
#define ALLOK_H

#define ALLOK_DEFAULT_POOL_COUNT 0
#define ALLOK_DEFAULT_POOL_SIZE (8 * 1024)
#define ALLOK_DEFAULT_ALLOC_TYPE ALLOK_BEST_FIT
#define ALLOK_DEFAULT_ALLOC_DYNAMIC ALLOK_TRUE

#define ALLOK_NULL ((void *)0)
#define VPTR(p) ((void **)(&p))

#if defined(__x86_64__) || defined(_M_X64) || defined(__aarch64__)
typedef unsigned long AllokSize;
#else
typedef unsigned int AllokSize;
#endif

typedef unsigned char AllokByte;

typedef enum AllokBool {
    ALLOK_FALSE = 0,
    ALLOK_TRUE = 1,
} AllokBool;

typedef enum AllokResult {
    ALLOK_SUCCESS = 0,
    ALLOK_NOT_FOUND = 5,
    ALLOK_NULL_PARAM = 10,
    ALLOK_INVALID_SIZE = 11,
    ALLOK_INVALID_ADDR = 12,
    ALLOK_UNINITIALIZED = 15,
    ALLOK_INSUFFICIENT_ARENA_MEMORY = 100,
    ALLOK_INSUFFICIENT_POOL_MEMORY = 150,
    ALLOK_OS_MEMORY_ALLOC_FAILED = 1000
} AllokResult;

typedef enum AllokType {
    ALLOK_LINEAR_FIT = 0,
    ALLOK_FIRST_FIT,
    ALLOK_BEST_FIT,
    ALLOK_WORST_FIT
} AllokType;

typedef struct AkMemoryArena {
    AllokSize alloc_size;
    AllokSize size;
    void *p_start;
    void *p_current;
    struct AkMemoryArena *p_next;
    struct AkMemoryArena *p_prev;
} AkMemoryArena;

typedef struct AkMemoryPool AkMemoryPool;
typedef struct AkMemoryMap AkMemoryMap;

typedef struct AkMemoryBlock {
    AllokSize size;
    void *p_start;
    struct AkMemoryBlock *p_next;
    struct AkMemoryBlock *p_prev;
    AkMemoryPool *p_parent;
} AkMemoryBlock;

typedef struct AkMemoryPool {
    AllokSize alloc_size;
    AllokSize size;
    void *p_start;
    AkMemoryBlock *p_head;
    AkMemoryBlock *p_tail;
    AkMemoryPool *p_next;
    AkMemoryPool *p_prev;
    AkMemoryMap *p_parent_map;
} AkMemoryPool;

typedef struct AkMemoryMapParams {
    AllokType type;
    AllokBool is_dynamic;
} AkMemoryMapParams;

typedef struct AkMemoryMapMetadata {
    int blocks_created;
    int blocks_freed;
    int pools_created;
    int pools_freed;
} AkMemoryMapMetadata;

typedef struct AkMemoryMap {
    AkMemoryMapParams params;
    AkMemoryMapMetadata metadata;
    AllokSize pool_count;
    void *p_start;
    AkMemoryPool *p_pool_head;
    AkMemoryPool *p_pool_tail;
} AkMemoryMap;

/**
 * Set a region of memory to a specific value
 * @param pp_result A pointer to the start of memory to set
 * @param value The value to set each byte of memory
 * @param size The size of memory to set
 * @return AllocResult
 */
AllokResult akMemset(void **pp_result, const AllokByte value, const AllokSize size);

/**
 * Copy a region of memory to another
 * @param pp_result A pointer to the start of memory to copy to
 * @param p_src A pointer to the data to copy
 * @param size The size of memory to copy
 * @return AllocResult
 */
AllokResult akMemcpy(void **pp_result, const void *p_src, const AllokSize size);


/**
 * Allocate a specified amount of heap memory from the OS
 * @param pp_result A pointer to the pointer of a MemoryArena to initialize
 * @param size The amount of memory to allocate to the arena
 * @return AllocResult
 */
AllokResult akMemoryArenaAlloc(AkMemoryArena **pp_result, const AllokSize size);

/**
 * Claim a specified portion of memory from a MemoryArena
 * @param pp_result A pointer to the result address of the arena
 * @param p_arena The MemoryArena to claim memory from
 * @param size The amount of memory to claim
 * @return AllocResult
 */
AllokResult akMemoryArenaClaim(void **pp_result, AkMemoryArena *p_arena, const AllokSize size);

/**
 * Reset MemoryArena to initialized state, allowing it to overwrite previously claimed memory
 * @param p_arena The MemoryArena to reset
 * @return AllocResult
 */
AllokResult akMemoryArenaReset(AkMemoryArena *p_arena);

/**
 * Free a portion of memory within a MemoryArena, allowing it to overwrite it
 * @param pp_target A pointer to the start of memory to free from the arena
 * @param p_arena The MemoryArena to free memory from
 * @param size The amount of memory to free
 * @param auto_destroy If ALLOC_TRUE p_arena will be automatically destroyed if its size is 0 after this free
 * @return AllocResult
 */
AllokResult akMemoryArenaFree(void **pp_target, AkMemoryArena *p_arena, const AllokSize size, const AllokBool auto_destroy);

/**
 * Destroy a MemoryArena and return its allocated memory to the OS
 * Sets the arena to ALLOC_NULL
 * @param pp_arena A pointer to a pointer of the MemoryArena to destroy
 * @param recursive If ALLOC_TRUE each p_next MemoryArena will be destroyed as well
 */
void akMemoryArenaDestroy(AkMemoryArena **pp_arena, const AllokBool recursive);


/**
 * Initialize a MemoryMap within a MemoryArena
 * @param pp_map_result A pointer to a pointer of the MemoryMap that will be initialized
 * @param pp_arena_result A pointer to a pointer of the MemoryArena that contains the MemoryMap
 * @param init_pool_count (Needs to be at least 1) The initial amount of MemoryPool's allocated within the MemoryMap
 * @param init_pool_size The size of each MemoryPool in bytes to be allocated
 * @param params Parameters to initialize the MemoryMap with
 * @return AllocResult
 */
AllokResult akMemoryMapAlloc(AkMemoryMap **pp_map_result, AkMemoryArena **pp_arena_result, const AllokSize init_pool_count, const AllokSize init_pool_size, const AkMemoryMapParams params);

/**
 * Initialize a MemoryPool of a specified size of heap memory from the OS
 * @param pp_result A pointer to a pointer of the MemoryPool that will be initialized
 * @param p_map (Optional) The parent MemoryMap that this MemoryPool belongs to
 * @param size This MemoryPool's allocated bytes of memory
 * @return AllocResult
 */
AllokResult akMemoryPoolAlloc(AkMemoryPool **pp_result, AkMemoryMap *p_map, const AllokSize size);

/**
 * Free a MemoryPool and return its allocated memory to the OS
 * Sets the pool to ALLOC_NULL
 * @param pp_pool A pointer to a pointer of the MemoryPool to free
 * @param recursive If ALLOC_TRUE each p_next MemoryPool will be destroyed as well
 * @return AllocResult
 */
AllokResult akMemoryPoolFree(AkMemoryPool **pp_pool, const AllokBool recursive);


/**
 * Initialize a MemoryBlock
 * @param pp_result A pointer to a pointer of the MemoryBlock to initialize
 * @param p_pool The parent MemoryPool that this MemoryBlock belongs to
 * @param size This MemoryBlock's allocated bytes of memory
 * @param offset The offset from the p_pool->p_start in memory to occupy
 * @return AllocResult
 */
AllokResult akMemoryBlockCreate(AkMemoryBlock **pp_result, AkMemoryPool *p_pool, const AllokSize size, const AllokSize offset);

/**
 * Search a MemoryMap and its associated MemoryPool's to find a MemoryBlock
 * @param pp_result A pointer to a pointer of the found MemoryBlock, will be ALLOC_NULL if not found
 * @param p_map The MemoryMap to search
 * @param ptr A pointer to the beginning of memory to find
 * @return AllocResult
 */
AllokResult akMemoryBlockFind(AkMemoryBlock **pp_result, const AkMemoryMap *p_map, const void *ptr);

/**
 * Free a MemoryBlock from its allocated memory within its parent MemoryPool
 * Sets the block to ALLOC_NULL
 * @param pp_block A pointer to a pointer of the MemoryBlock to free
 */
void akMemoryBlockFree(AkMemoryBlock **pp_block);

/**
 * Initialize the global MemoryMap used by Alloc, Realloc, Calloc, and Free
 * If this is not called then default values will be used to initialize when Alloc is first called
 * @param init_pool_count The initial amount of MemoryPool's allocated within the MemoryMap
 * @param init_pool_size The size of each MemoryPool in bytes to be allocated
 * @param params Parameters to initialize the MemoryMap with
 * @return AllocResult
 */
AllokResult akInit(const AllokSize init_pool_count, const AllokSize init_pool_size, const AkMemoryMapParams params);

/**
 * Allocate a specified amount of heap memory
 * @param pp_result A pointer to the starting address in memory that will be allocated
 * @param size The amount of bytes to allocate
 * @return AllocResult
 */
AllokResult akAlloc(void **pp_result, const AllokSize size);

/**
 * Reallocate a specified amount of heap memory, copying all data from the p_src
 * @param pp_result A pointer to the starting address in memory that has been reallocated
 * @param p_src A pointer to memory that has been previously allocated
 * @param size The amount of bytes to reallocate
 * @return AllocResult
 */
AllokResult akRealloc(void **pp_result, void *p_src, const AllokSize size);

/**
 * Allocate a specified amount of heap memory and set all its bytes to 0
 * @param pp_result A pointer to the starting address in memory that has been allocated
 * @param size The amount of bytes to allocate
 * @return AllocResult
 */
AllokResult akCalloc(void **pp_result, const AllokSize size);

/**
 * Calculate the total number of bytes that is currently globally allocated
 * @return The number of bytes allocated
 */
AllokSize akGetTotalAllocSize();

/**
 * Calculate the total number of MemoryPool's that are currently allocated for global memory
 * @return The number of pools
 */
AllokSize akGetTotalPoolCount();

/**
 * Calculate the total number of MemoryBlock's that are currently allocated for global memory
 * @return The number of blocks
 */
AllokSize akGetTotalBlockCount();

/**
 * Get the current metadata for the global MemoryMap
 * @return Global memory metadata
 */
AkMemoryMapMetadata akGetAllocMetadata();

/**
 * Free memory that was previously allocated by Alloc, Realloc, or Calloc
 * Sets the pointer to ALLOC_NULL
 * @param pp_target A pointer to the start of memory allocated
 * @return AllocResult
 */
AllokResult akFree(void **pp_target);

/**
 * Destroy all global memory, invalidating all previously allocated memory
 */
void akDump();

#endif //ALLOK_H
