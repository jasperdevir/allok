#include <allok.h>

#include <stdio.h>
#include <stdlib.h>

#define MAX_ARR_SIZE 2000

#define MAX_ARRAY_AMT 100

void print_allok_metadata();
void print_allok_stats(AllokSize array_count);

int main(void) {
    char input[BUFSIZ];

    int **array_ptrs;
    AllokSize array_ptrs_count = 0;

    AllokSize arena_alloc_size = MAX_ARRAY_AMT * sizeof(int*);

    AkMemoryArena *array_ptr_arena;
    AllokResult result = akMemoryArenaAlloc(&array_ptr_arena, arena_alloc_size);
    if (result != ALLOK_SUCCESS) {
        printf("[%d] akMemoryArenaAlloc failed.\n", result);
        return 1;
    }

    result = akMemoryArenaClaim(VPTR(array_ptrs), array_ptr_arena, arena_alloc_size);
    if (result != ALLOK_SUCCESS) {
        printf("[%d] akMemoryArenaClaim failed.\n", result);
        return 1;
    }

    result = akMemset(VPTR(array_ptrs), 0, arena_alloc_size);
    if (result != ALLOK_SUCCESS) {
        printf("[%d] akMemset failed.\n", result);
        return 1;
    }

    printf("======== allok Example ========\n");

    while (1) {
        printf("\nEnter length of array (1-%d)\n", MAX_ARR_SIZE);
        printf("Enter -2 to view Allok stats\n");
        printf("Enter -1 to quit\n");

        fgets(input, BUFSIZ, stdin);

        int size = (int)strtol(input, NULL, 10);
        if (size < 1 || size > MAX_ARR_SIZE) {
            if (size == -2) {
                print_allok_stats(array_ptrs_count);
                continue;
            }
            if (size == -1) {
                break;
            }
            printf("Invalid input. Length must be between 1 and %d.\n", MAX_ARR_SIZE);
            continue;
        }

        int *arr;
        AllokSize alloc_size = size * sizeof(int *);
        result = akAlloc(VPTR(arr), alloc_size);
        if (result != ALLOK_SUCCESS) {
            printf("Error allocating memory.\n");
            continue;
        }

        array_ptrs[array_ptrs_count++] = arr;

        printf("Array of %d elements and %zu bytes allocated!\n", size, alloc_size);

        if(array_ptrs_count > MAX_ARRAY_AMT) {
            printf("Maximum number of arrays exceeded.\n");
            break;
        }
    }

    printf("Exiting...\n");

    if (array_ptrs_count > 0) {
        printf("\nFreeing %lu arrays from memory\n", array_ptrs_count);
        for (int i = 0; i < array_ptrs_count; i++) {
            if (array_ptrs[i] != NULL) {
                result = akFree(VPTR(array_ptrs[i]));
                if (result != ALLOK_SUCCESS) {
                    printf("Error freeing memory.\n");
                }
            }
        }
    }

    print_allok_metadata();

    akMemoryArenaDestroy(&array_ptr_arena, ALLOK_FALSE);

    return 0;
}

void print_allok_metadata() {
    const AkMemoryMapMetadata metadata = akGetAllocMetadata();
    printf("\n================================\n");
    printf("Pools Created         : %d\n", metadata.pools_created);
    printf("Pools Freed           : %d\n", metadata.pools_freed);
    printf("Blocks Created        : %d\n", metadata.blocks_created);
    printf("Blocks Freed          : %d\n", metadata.blocks_freed);
    printf("=================================\n");
}

void print_allok_stats(AllokSize array_count) {
    printf("=================================\n");
    printf("Array Count      : %lu\n", array_count);
    printf("Memory Allocated : %lu bytes\n", akGetTotalAllocSize());
    printf("MemoryPool Count : %lu\n", akGetTotalPoolCount());
    printf("MemoryBlock Count: %lu\n", akGetTotalBlockCount());
    printf("=================================\n");

    print_allok_metadata();
}
