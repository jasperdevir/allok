#include <allok.h>

static AkMemoryMap *g_map;
static AkMemoryArena *g_map_arena;

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <memoryapi.h>
#elif defined(__APPLE__) || defined(__linux__)
#include <sys/mman.h>
#else
#error "Unsupported OS"
#endif

static inline AllokSize max_size(const AllokSize a, const AllokSize b) {
    return a > b ? a : b;
}

static inline AllokSize min_size(const AllokSize a, const AllokSize b) {
    return a < b ? a : b;
}

static inline AllokResult get_ptr_fragment(const AllokByte *a, const AllokSize size_a, const AllokByte *b, AllokSize *p_result) {
    const AllokByte *end_a = a + size_a;
    if (end_a > b) {
        return ALLOK_INVALID_ADDR;
    }
    *p_result = (AllokSize)(b - end_a);
    return ALLOK_SUCCESS;
}

AllokBool is_ptr_in_range(const void *ptr, const void *p_start, const AllokSize size) {
    if (ptr == ALLOK_NULL || p_start == ALLOK_NULL) {
        return ALLOK_FALSE;
    }

    const AllokByte *addr = (AllokByte *)ptr;
    const AllokByte *start = (AllokByte *)p_start;
    const AllokByte *end = (AllokByte *)p_start + size;

    return addr >= start && addr < end;
}

AllokResult akMemset(void **pp_result, const AllokByte value, const AllokSize size) {
    if (pp_result == ALLOK_NULL) {
        return ALLOK_NULL_PARAM;
    }

    AllokByte *result = (AllokByte *)(*pp_result);
    for (AllokSize i = 0; i < size; ++i) {
        result[i] = value;
    }
    return ALLOK_SUCCESS;
}

AllokResult akMemcpy(void **pp_result, const void *p_src, const AllokSize size) {
    if (pp_result == ALLOK_NULL) {
        return ALLOK_NULL_PARAM;
    }

    AllokByte *result = (AllokByte *)(*pp_result);
    const AllokByte *src = (AllokByte *)p_src;
    for (AllokSize i = 0; i < size; ++i) {
        result[i] = src[i];
    }

    return ALLOK_SUCCESS;
}

void *os_mem_alloc(const AllokSize size) {
#if _WIN32 || _WIN64
     return VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#elif __APPLE__ || __linux__
    return mmap(ALLOK_NULL, size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
#else
    return ALLOC_NULL;
#endif
}

void os_mem_free(void *ptr, const AllokSize size) {
#if _WIN32 || _WIN64
    VirtualFree(ptr, 0, MEM_RELEASE);
#elif __APPLE__ || __linux__
    munmap(ptr, size);
#endif
}

AllokResult akMemoryArenaAlloc(AkMemoryArena **pp_result, const AllokSize size) {
    AllokSize alloc_size = size + sizeof(AkMemoryArena);

    AkMemoryArena *arena = os_mem_alloc(alloc_size);
    if (arena == ALLOK_NULL) {
        return ALLOK_OS_MEMORY_ALLOC_FAILED;
    }

    arena->p_start = (char *)arena + sizeof(AkMemoryArena);
    arena->p_current = arena->p_start;
    arena->size = 0;
    arena->alloc_size = size;
    arena->p_next = ALLOK_NULL;
    arena->p_prev = ALLOK_NULL;

    *pp_result = arena;

    return ALLOK_SUCCESS;
}

AllokResult akMemoryArenaClaim(void **pp_result, AkMemoryArena *p_arena, const AllokSize size) {
    if (p_arena == ALLOK_NULL) {
        return ALLOK_NULL_PARAM;
    }

    if (p_arena->alloc_size < size || p_arena->size + size > p_arena->alloc_size) {
        return ALLOK_INSUFFICIENT_ARENA_MEMORY;
    }

    *pp_result = p_arena->p_current;
    p_arena->size += size;
    p_arena->p_current = (AllokByte *)p_arena->p_current + size;

    return ALLOK_SUCCESS;
}

AllokResult akMemoryArenaReset(AkMemoryArena *p_arena) {
    if (p_arena == ALLOK_NULL) {
        return ALLOK_NULL_PARAM;
    }

    p_arena->size = 0;
    p_arena->p_current = p_arena->p_start;

    return ALLOK_SUCCESS;
}

AllokResult akMemoryArenaFree(void **pp_target, AkMemoryArena *p_arena, const AllokSize size, const AllokBool auto_destroy) {
    if (pp_target == ALLOK_NULL || p_arena == ALLOK_NULL) {
        return ALLOK_NULL_PARAM;
    }

    if (p_arena->alloc_size < size) {
        return ALLOK_INVALID_SIZE;
    }

    if (is_ptr_in_range(*pp_target, p_arena->p_start, p_arena->size) == ALLOK_FALSE) {
        return ALLOK_INVALID_ADDR;
    }

    if ((AllokByte *)p_arena->p_current == (AllokByte *)pp_target + size) {
        p_arena->p_current = (AllokByte *)p_arena->p_current - size;
    }
    p_arena->size -= size;

    if (p_arena->size <= 0) {
        if (auto_destroy) {
            if (p_arena->p_prev) {
                p_arena->p_prev->p_next = p_arena->p_next;
            }

            if (p_arena->p_next) {
                p_arena->p_next->p_prev = p_arena->p_prev;
            }

            akMemoryArenaDestroy(&p_arena, ALLOK_FALSE);
        } else {
            p_arena->p_current = p_arena->p_start;
        }
    }

    *pp_target = ALLOK_NULL;

    return ALLOK_SUCCESS;

}

void akMemoryArenaDestroy(AkMemoryArena **pp_arena, const AllokBool recursive) {
    if (pp_arena == ALLOK_NULL) {
        return;
    }

    AkMemoryArena *arena = *pp_arena;
    if (arena == ALLOK_NULL) {
        return;
    }

    AkMemoryArena *next = arena->p_next;
    AkMemoryArena *prev = arena->p_prev;
    if (prev != ALLOK_NULL) {
        prev->p_next = next;
    }
    if (next != ALLOK_NULL) {
        next->p_prev = prev;
    }

    os_mem_free(arena, arena->alloc_size + sizeof(AkMemoryArena));

    if (recursive) {
        akMemoryArenaDestroy(&next, recursive);
    }

    *pp_arena = ALLOK_NULL;
}

AllokResult akMemoryBlockCreate(AkMemoryBlock **pp_result, AkMemoryPool *p_pool, const AllokSize size, const AllokSize offset) {
    if (pp_result == ALLOK_NULL || p_pool == ALLOK_NULL) {
        return ALLOK_NULL_PARAM;
    }

    AllokByte *block_start = (AllokByte *)p_pool->p_start + offset;

    if (offset + size + sizeof(AkMemoryBlock) > p_pool->alloc_size) {
        return ALLOK_INSUFFICIENT_POOL_MEMORY;
    }

    AkMemoryBlock *block = (AkMemoryBlock *)block_start;
    block->size = size;
    block->p_start = (char *)block + sizeof(AkMemoryBlock);
    block->p_parent = p_pool;

    AkMemoryBlock *current = p_pool->p_head;
    AkMemoryBlock *prev = ALLOK_NULL;

    while (current != ALLOK_NULL && (AllokByte *)current < (AllokByte *)block) {
        prev = current;
        current = current->p_next;
    }

    if (prev == ALLOK_NULL) {
        block->p_next = p_pool->p_head;
        if (p_pool->p_head != ALLOK_NULL) {
            p_pool->p_head->p_prev = block;
        }
        p_pool->p_head = block;

        if (p_pool->p_tail == ALLOK_NULL) {
            p_pool->p_tail = block;
        }
    } else {
        block->p_prev = prev;
        block->p_next = prev->p_next;

        if (prev->p_next != ALLOK_NULL) {
            prev->p_next->p_prev = block;
        } else {
            p_pool->p_tail = block;
        }

        prev->p_next = block;
    }

    p_pool->size += size + sizeof(AkMemoryBlock);
    if (p_pool->p_parent_map != ALLOK_NULL) {
        p_pool->p_parent_map->metadata.blocks_created++;
    }

    *pp_result = block;

    return ALLOK_SUCCESS;
}

AllokResult akMemoryBlockFind(AkMemoryBlock **pp_result, const AkMemoryMap *p_map, const void *ptr) {
    if (p_map == ALLOK_NULL || ptr == ALLOK_NULL) {
        return ALLOK_NULL_PARAM;
    }

    const AkMemoryPool *pool = p_map->p_pool_head;
    while (pool != ALLOK_NULL) {
        if (is_ptr_in_range(ptr, pool->p_start, pool->alloc_size)) {
            AkMemoryBlock *block = pool->p_head;
            while (block != ALLOK_NULL) {
                if ((AllokByte *)block->p_start == (AllokByte *)ptr) {
                    *pp_result = block;
                    return ALLOK_SUCCESS;
                }
                block = block->p_next;
            }
        }
        pool = pool->p_next;
    }

    return ALLOK_NOT_FOUND;
}

void akMemoryBlockFree(AkMemoryBlock **pp_block) {
    if (pp_block == ALLOK_NULL) {
        return;
    }

    const AkMemoryBlock *block = *pp_block;
    if (block == ALLOK_NULL) {
        return;
    }

    AkMemoryPool *pool = block->p_parent;
    AkMemoryBlock *prev = block->p_prev;
    AkMemoryBlock *next = block->p_next;

    if (prev == ALLOK_NULL) {
        pool->p_head = next;
    } else {
        prev->p_next = next;
    }

    if (next == ALLOK_NULL) {
        pool->p_tail = prev;
    } else {
        next->p_prev = prev;
    }

    pool->size -= block->size + sizeof(AkMemoryBlock);
    if (pool->p_parent_map != ALLOK_NULL) {
        pool->p_parent_map->metadata.blocks_freed++;
    }

    *pp_block = ALLOK_NULL;

    if (pool->size <= 0) {
        akMemoryPoolFree(&pool, ALLOK_FALSE);
    }
}

AllokResult akMemoryPoolAlloc(AkMemoryPool **pp_result, AkMemoryMap *p_map, const AllokSize size) {
    if (pp_result == ALLOK_NULL) {
        return ALLOK_NULL_PARAM;
    }

    AllokSize alloc_size = size + sizeof(AkMemoryPool);

    AkMemoryPool *pool = os_mem_alloc(alloc_size);
    if (pool == ALLOK_NULL) {
        return ALLOK_OS_MEMORY_ALLOC_FAILED;
    }

    pool->alloc_size = size;
    pool->size = 0;
    pool->p_start = (AllokByte *)pool + sizeof(AkMemoryPool);
    pool->p_next = ALLOK_NULL;
    pool->p_parent_map = p_map;
    pool->p_head = ALLOK_NULL;
    pool->p_tail = ALLOK_NULL;

    if (p_map != ALLOK_NULL) {
        if (p_map->p_pool_tail != ALLOK_NULL) {
            p_map->p_pool_tail->p_next = pool;
            pool->p_prev = p_map->p_pool_tail;
        } else {
            pool->p_prev = ALLOK_NULL;
            p_map->p_pool_head = pool;
        }
        p_map->p_pool_tail = pool;
        p_map->pool_count++;
        p_map->metadata.pools_created++;
    } else {
        pool->p_prev = ALLOK_NULL;
    }

    *pp_result = pool;

    return ALLOK_SUCCESS;
}

AllokResult akMemoryPoolFree(AkMemoryPool **pp_pool, const AllokBool recursive) {
    if (pp_pool == ALLOK_NULL) {
        return ALLOK_NULL_PARAM;
    }

    AkMemoryPool *pool = *pp_pool;
    if (pool == ALLOK_NULL) {
        return ALLOK_NULL_PARAM;
    }

    if (recursive) {
        akMemoryPoolFree(&pool->p_next, recursive);
    }

    AkMemoryPool *prev = pool->p_prev;
    AkMemoryPool *next = pool->p_next;
    AkMemoryMap *map = pool->p_parent_map;

    if (prev != ALLOK_NULL) {
        prev->p_next = next;
    } else if (map != ALLOK_NULL) {
        map->p_pool_head = next;
    }
    if (next != ALLOK_NULL) {
        next->p_prev = prev;
    } else if (map != ALLOK_NULL) {
        map->p_pool_tail = prev;
    }

    if (map != ALLOK_NULL) {
        map->pool_count--;
        map->metadata.pools_freed++;
    }

    os_mem_free(pool, pool->alloc_size + sizeof(AkMemoryPool));

    *pp_pool = ALLOK_NULL;

    return ALLOK_SUCCESS;
}

AllokResult akMemoryMapAlloc(AkMemoryMap **pp_map_result, AkMemoryArena **pp_arena_result, const AllokSize init_pool_count, const AllokSize init_pool_size, const AkMemoryMapParams params) {
    if (pp_map_result == ALLOK_NULL) {
        return ALLOK_NULL_PARAM;
    }

    AllokSize map_alloc_size = sizeof(AkMemoryMap);

    AkMemoryArena *arena;
    AllokResult result = akMemoryArenaAlloc(&arena, map_alloc_size);
    if (result != ALLOK_SUCCESS) {
        return result;
    }

    AkMemoryMap *map;
    result = akMemoryArenaClaim(VPTR(map), arena, map_alloc_size);
    if (result != ALLOK_SUCCESS) {
        return result;
    }

    map->pool_count = 1;
    map->p_pool_head = ALLOK_NULL;
    map->p_pool_tail = ALLOK_NULL;
    map->p_start = (AllokByte *)map + sizeof(AkMemoryMap);
    map->metadata = (AkMemoryMapMetadata){};
    map->params.type = params.type;
    map->params.is_dynamic = params.is_dynamic;

    for (AllokSize i = 0; i < init_pool_count; i++) {
        AkMemoryPool *pool;
        result = akMemoryPoolAlloc(&pool, map, init_pool_size);
        if (result != ALLOK_SUCCESS) {
            break;
        }
    }

    if (result != ALLOK_SUCCESS) {
        akMemoryArenaDestroy(&arena, ALLOK_FALSE);
        akMemoryPoolFree(&map->p_pool_head, ALLOK_TRUE);
        return result;
    }

    *pp_arena_result = arena;
    *pp_map_result = map;

    return ALLOK_SUCCESS;
}

AllokBool alloc_first_fit(const AkMemoryPool *p_pool, const AllokSize size, AllokSize *p_offset_result) {
    if (p_pool == ALLOK_NULL) {
        return ALLOK_FALSE;
    }

    const AllokSize alloc_size = sizeof(AkMemoryBlock) + size;

    AkMemoryBlock *head = p_pool->p_head;
    const AkMemoryBlock *tail = p_pool->p_tail;

    if (head == ALLOK_NULL) {
        *p_offset_result = 0;
        return p_pool->alloc_size >= alloc_size ? ALLOK_TRUE : ALLOK_FALSE;
    }

    AllokSize frag_size = 0;
    if (get_ptr_fragment(p_pool->p_start, 0, (AllokByte *)head, &frag_size) == ALLOK_SUCCESS) {
        if (frag_size >= alloc_size) {
            *p_offset_result = 0;
            return ALLOK_TRUE;
        }
    }

    AkMemoryBlock *prev = head;
    AkMemoryBlock *curr = head->p_next;

    while (curr != ALLOK_NULL) {
        if (get_ptr_fragment(prev->p_start, prev->size, (AllokByte *)curr, &frag_size) != ALLOK_SUCCESS) {
            continue;
        }
        if (frag_size >= alloc_size) {
            *p_offset_result = (AllokSize)((AllokByte *)prev->p_start + prev->size - (AllokByte *)p_pool->p_start);
            return ALLOK_TRUE;
        }
        prev = curr;
        curr = curr->p_next;
    }

    if (get_ptr_fragment(tail->p_start, tail->size, (AllokByte *)p_pool->p_start + p_pool->alloc_size, &frag_size) == ALLOK_SUCCESS) {
        if (frag_size >= alloc_size) {
            *p_offset_result = (AllokSize)((AllokByte *)tail->p_start + tail->size - (AllokByte *)p_pool->p_start);
            return ALLOK_TRUE;
        }
    }

    return ALLOK_FALSE;

}

AllokBool alloc_best_fit(const AkMemoryPool *p_pool, const AllokSize size, AllokSize *p_offset_result) {
    if (p_pool == ALLOK_NULL) {
        return ALLOK_FALSE;
    }

    const AllokSize alloc_size = sizeof(AkMemoryBlock) + size;

    AkMemoryBlock *head = p_pool->p_head;
    const AkMemoryBlock *tail = p_pool->p_tail;

    if (head == ALLOK_NULL) {
        *p_offset_result = 0;
        return p_pool->alloc_size >= alloc_size ? ALLOK_TRUE : ALLOK_FALSE;
    }

    AllokSize best_size = p_pool->alloc_size;
    AllokSize best_fit_offset = 0;

    AllokSize frag_size = 0;
    if (get_ptr_fragment(p_pool->p_start, 0, (AllokByte *)head, &frag_size) == ALLOK_SUCCESS) {
        if (frag_size >= alloc_size) {
            best_size = frag_size;
            best_fit_offset = 0;
        }
    }


    AllokBool block_found = ALLOK_FALSE;
    AkMemoryBlock *prev = head;
    AkMemoryBlock *curr = head->p_next;

    while (curr != ALLOK_NULL) {
        if (get_ptr_fragment(prev->p_start, prev->size, (AllokByte *)curr, &frag_size) != ALLOK_SUCCESS) {
            continue;
        }
        if (frag_size >= alloc_size && frag_size < best_size) {
            best_size = frag_size;
            best_fit_offset = (AllokSize)((AllokByte *)prev->p_start + prev->size - (AllokByte *)p_pool->p_start);
            block_found = ALLOK_TRUE;
        }
        prev = curr;
        curr = curr->p_next;
    }

    if (get_ptr_fragment(tail->p_start, tail->size, (AllokByte *)p_pool->p_start + p_pool->alloc_size, &frag_size) == ALLOK_SUCCESS) {
        if (frag_size >= alloc_size && frag_size < best_size) {
            *p_offset_result = (AllokSize)((AllokByte *)tail->p_start + tail->size - (AllokByte *)p_pool->p_start);
            return ALLOK_TRUE;
        }
    }

    if (block_found == ALLOK_TRUE) {
        *p_offset_result = best_fit_offset;
        return ALLOK_TRUE;
    }

    return ALLOK_FALSE;
}

AllokBool alloc_worst_fit(const AkMemoryPool *p_pool, const AllokSize size, AllokSize *p_offset_result) {
    if (p_pool == ALLOK_NULL) {
        return ALLOK_FALSE;
    }

    const AllokSize alloc_size = sizeof(AkMemoryBlock) + size;

    AkMemoryBlock *head = p_pool->p_head;
    const AkMemoryBlock *tail = p_pool->p_tail;

    if (head == ALLOK_NULL) {
        *p_offset_result = 0;
        return p_pool->alloc_size >= alloc_size ? ALLOK_TRUE : ALLOK_FALSE;
    }

    AllokSize worst_size = 0;
    AllokSize worst_fit_offset = 0;

    AllokSize frag_size = 0;
    if (get_ptr_fragment(p_pool->p_start, 0, (AllokByte *)head, &frag_size) == ALLOK_SUCCESS) {
        if (frag_size >= alloc_size) {
            worst_size = frag_size;
            worst_fit_offset = 0;
        }
    }

    AllokBool block_found = ALLOK_FALSE;
    AkMemoryBlock *prev = head;
    AkMemoryBlock *curr = head->p_next;

    while (curr != ALLOK_NULL) {
        if (get_ptr_fragment(prev->p_start, prev->size, (AllokByte *)curr, &frag_size) != ALLOK_SUCCESS) {
            continue;
        }
        if (frag_size >= alloc_size && frag_size > worst_size) {
            worst_size = frag_size;
            worst_fit_offset = (AllokSize)((AllokByte *)prev->p_start + prev->size - (AllokByte *)p_pool->p_start);
            block_found = ALLOK_TRUE;
        }
        prev = curr;
        curr = curr->p_next;
    }

    if (get_ptr_fragment(tail->p_start, tail->size, (AllokByte *)p_pool->p_start + p_pool->alloc_size, &frag_size) == ALLOK_SUCCESS) {
        if (frag_size >= alloc_size && frag_size > worst_size) {
            *p_offset_result = (AllokSize)((AllokByte *)tail->p_start + tail->size - (AllokByte *)p_pool->p_start);
            return ALLOK_TRUE;
        }
    }


    if (block_found == ALLOK_TRUE) {
        *p_offset_result = worst_fit_offset;
        return ALLOK_TRUE;
    }

    return ALLOK_FALSE;
}

AllokBool alloc_linear_fit(const AkMemoryPool *p_pool, const AllokSize size, AllokSize *p_offset_result) {
    if (p_pool == ALLOK_NULL) {
        return ALLOK_FALSE;
    }

    const AllokSize alloc_size = sizeof(AkMemoryBlock) + size;

    const AkMemoryBlock *tail = p_pool->p_tail;
    AllokSize block_offset = 0;
    if (tail != ALLOK_NULL) {
        block_offset = (AllokSize)((AllokByte *)tail->p_start + tail->size - ((AllokByte *)p_pool->p_start));
        if (block_offset + alloc_size > p_pool->alloc_size) {
            return ALLOK_FALSE;
        }
    }

    *p_offset_result = block_offset;
    return ALLOK_TRUE;
}

AllokBool find_block_fit(const AkMemoryPool *p_pool, const AllokSize size, AllokSize *p_offset_result) {
    switch (g_map->params.type) {
        case ALLOK_FIRST_FIT: {
            return alloc_first_fit(p_pool, size, p_offset_result);
        }
        case ALLOK_BEST_FIT: {
            return alloc_best_fit(p_pool, size, p_offset_result);
        }
        case ALLOK_WORST_FIT: {
            return alloc_worst_fit(p_pool, size, p_offset_result);
        }
        case ALLOK_LINEAR_FIT: {
            return alloc_linear_fit(p_pool, size, p_offset_result);
        }
        default: {
            return ALLOK_FALSE;
        }
    }
}

AllokResult akInit(const AllokSize init_pool_count, const AllokSize init_pool_size, const AkMemoryMapParams params) {
    if (g_map != ALLOK_NULL && g_map->p_pool_head != ALLOK_NULL) {
        akDump();
    }

    AllokResult result = akMemoryMapAlloc(&g_map, &g_map_arena, init_pool_count, init_pool_size, params);
    if (result != ALLOK_SUCCESS) {
        return result;
    }

    return ALLOK_SUCCESS;
}

AllokResult akAlloc(void **pp_result, const AllokSize size) {
    AllokResult result;
    if (g_map == ALLOK_NULL) {
        result = akInit(ALLOK_DEFAULT_POOL_COUNT, ALLOK_DEFAULT_POOL_SIZE, (AkMemoryMapParams){ALLOK_DEFAULT_ALLOC_TYPE, ALLOK_DEFAULT_ALLOC_DYNAMIC});
        if (result != ALLOK_SUCCESS) {
            return result;
        }
    }

    const AllokSize block_alloc_size = sizeof(AkMemoryBlock) + size;
    AllokBool offset_found = ALLOK_FALSE;

    AkMemoryPool *pool = g_map->p_pool_head;
    AllokSize block_offset = 0;
    while (pool != ALLOK_NULL) {
        if (pool->alloc_size - pool->size >= block_alloc_size) {
            if (find_block_fit(pool, size, &block_offset) == ALLOK_TRUE) {
                offset_found = ALLOK_TRUE;
                break;
            }
        }
        pool = pool->p_next;
    }

    if (offset_found == ALLOK_TRUE) {
        AkMemoryBlock *block;
        result = akMemoryBlockCreate(&block, pool, size, block_offset);
        if (result != ALLOK_SUCCESS) {
            return result;
        }

        *pp_result = block->p_start;
        return ALLOK_SUCCESS;

    }

    if (g_map->params.is_dynamic == ALLOK_FALSE) {
        return ALLOK_INSUFFICIENT_POOL_MEMORY;
    }

    AkMemoryPool *new_pool;
    const AllokSize alloc_size = max_size(ALLOK_DEFAULT_POOL_SIZE, block_alloc_size);
    result = akMemoryPoolAlloc(&new_pool, g_map, alloc_size);
    if (result != ALLOK_SUCCESS) {
        return result;
    }

    AkMemoryBlock *block;
    result = akMemoryBlockCreate(&block, new_pool, size, 0);
    if (result != ALLOK_SUCCESS) {
        return result;
    }

    *pp_result = block->p_start;

    return ALLOK_SUCCESS;
}

AllokResult akRealloc(void **pp_result, void *p_src, const AllokSize size) {
    if (g_map == ALLOK_NULL || p_src == ALLOK_NULL) {
        return ALLOK_NULL_PARAM;
    }

    AkMemoryBlock *block = ALLOK_NULL;
    AllokResult result = akMemoryBlockFind(&block, g_map, p_src);
    if (result != ALLOK_SUCCESS) {
        return result;
    }

    AkMemoryPool *pool = block->p_parent;
    AllokSize old_size = block->size;

    if (size <= old_size) {
        pool->size -= size - old_size;
        block->size = size;
        *pp_result = block->p_start;
        return ALLOK_SUCCESS;
    }

    if (pool->p_tail == block && pool->size + (size - old_size) <= pool->alloc_size) {
        pool->size += size - old_size;
        block->size = size;
        *pp_result = block->p_start;
        return ALLOK_SUCCESS;
    }

    result = akAlloc(pp_result, size);
    if (result != ALLOK_SUCCESS) {
        *pp_result = ALLOK_NULL;
        return result;
    }

    result = akMemcpy(pp_result, p_src, min_size(old_size, size));
    if (result != ALLOK_SUCCESS) {
        *pp_result = ALLOK_NULL;
        return result;
    }

    akFree(&p_src);

    return ALLOK_SUCCESS;
}

AllokResult akCalloc(void **pp_result, const AllokSize size) {
    const AllokResult result = akAlloc(pp_result, size);
    if (result != ALLOK_SUCCESS) {
        return result;
    }

    return akMemset(pp_result, 0, size);
}

AllokSize akGetTotalAllocSize() {
    AllokSize size = 0;
    if (g_map == ALLOK_NULL || g_map->p_pool_head == ALLOK_NULL) {
        return 0;
    }

    AkMemoryPool *pool = g_map->p_pool_head;
    while (pool != ALLOK_NULL) {
        size += pool->size;
        pool = pool->p_next;
    }

    return size;
}

AllokSize akGetTotalPoolCount() {
    if (g_map == ALLOK_NULL) {
        return 0;
    }

    AllokSize count = 0;
    const AkMemoryPool *pool = g_map->p_pool_head;
    while (pool != ALLOK_NULL) {
        count++;
        pool = pool->p_next;
    }

    return count;
}

AllokSize akGetTotalBlockCount() {
    if (g_map == ALLOK_NULL) {
        return 0;
    }

    AllokSize count = 0;
    const AkMemoryPool *pool = g_map->p_pool_head;
    while (pool != ALLOK_NULL) {
        const AkMemoryBlock *block = pool->p_head;
        while (block != ALLOK_NULL) {
            count++;
            block = block->p_next;
        }
        pool = pool->p_next;
    }

    return count;
}

AkMemoryMapMetadata akGetAllocMetadata() {
    if (g_map == ALLOK_NULL) {
        return (AkMemoryMapMetadata){};
    }

    return g_map->metadata;
}

AllokResult akFree(void **pp_target) {
    if (g_map == ALLOK_NULL || pp_target == ALLOK_NULL) {
        return ALLOK_UNINITIALIZED;
    }

    AkMemoryBlock *block = ALLOK_NULL;
    const AllokResult result = akMemoryBlockFind(&block, g_map, *pp_target);
    if (result != ALLOK_SUCCESS) {
        return result;
    }

    akMemoryBlockFree(&block);

    *pp_target = ALLOK_NULL;

    return ALLOK_SUCCESS;
}

void akDump() {
    if (g_map == ALLOK_NULL) {
        return;
    }

    akMemoryPoolFree(&g_map->p_pool_head, ALLOK_TRUE);
    g_map = ALLOK_NULL;

    akMemoryArenaDestroy(&g_map_arena, ALLOK_FALSE);

}