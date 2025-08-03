# Allok

Allok is a simple C library that features a dynamic heap memory
allocator, as well as memory management data structures. It's 
implemented without the C standard library and only requires 
OS specific functions.

### Currently tested platforms:
- Windows 11 _(x86_64)_
- Mac OS Sonoma 14.5 _(M1)_
- Debian 12 _(x86_64)_

## Contents
- [Methodology](#methodology)
- [Build](#build)
- [Usage](#usage)
- [Types & Macros](#types--macros)

## Methodology

The goal with this project was ultimately gain a deeper 
understanding of dynamic memory allocation in C. With no
external libraries, I wanted to create everything myself
from scratch, which has the added benefit of portability.

The layout of memory is constructed in a hierarchy to allow
intuitive control of memory allocation, access, and deallocation.

The diagrams below show the relationships between the different
data structures:

### Memory Map
![Memory Map Diagram](https://github.com/user-attachments/assets/ba3afe01-e21e-4dad-b103-4ffa96941283)
_Memory Maps_ are a linked list of _MemoryPools_ that contain a
linked list of _MemoryBlocks_, these _MemoryBlocks_ contain
the actual space of allocate memory and metadata.

When new memory is needed the pools can be searched with different
techniques such as _"best fit"_, _"worst fit"_, or _"first fit"_, 
attempting to minimise fragmentation. They are useful for dynamic
memory usage, but come with increased allocation time and memory use.

### Memory Arena
![Memory Arena Diagram](https://github.com/user-attachments/assets/469bd609-91d8-49b0-bb38-057fc958c1e7)
_MemoryArenas_ are linear memory buffers. When 
new memory is needed it can either be placed at the end of the
arenas current memory or a new arena can be allocated and linked 
to the existing arenas.

They are well suited for predictable memory usage that can be 
released simultaneously, allowing for better performance but with
a higher risk for fragmentation.

## Build

To build the library with `cmake` execute the following commands:
```bash
git clone https://github.com/jasperdevir/allok.git
cd allok
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

**Shared Library** - Set the `cmake` flag `ALLOK_STATIC=OFF`

**Example Program** - Set the `cmake` flag `ALLOK_BUILD_EXAMPLE=ON`

---
The results of the build process should now be in `allok/lib`
and `allok/bin`

## Usage

The typical pattern of calling with library looks like the 
following:
```c++
if (akAlloc(VPTR(ptr), size) != ALLOK_SUCCESS) {
    // HANDLE ERROR
}
```

All functions either return `void` or `AllokResult`, and
outputs of functions are passed by reference, this is done
to clearly indicate errors.

The following functions can be used to manage the global
`AkMemoryMap` of heap memory:
```c++
AllokResult akAlloc(void **pp_result, const AllokSize size);
AllokResult akRealloc(void **pp_result, void *p_src, const AllokSize size);
AllokResult akCalloc(void **pp_result, const AllokSize size);
AllokResult akFree(void **pp_target);
void akDump();
```

Other data structure related functions can be used to create
custom memory management systems outside of this libraries 
global allocator.

## Types & Macros

### Data Types
- `AllokSize`
  - `unsigned long` _(64 bit)_
  - `unsigned int` _(32 bit)_


- `AllokByte`
  - `unsigned char`


- `AllokBool` **enum**
  - `ALLOK_TRUE` = `0`
  - `ALLOK_FALSE` = `1`


- `AllokResult` **enum**
  - `ALLOK_SUCCESS` = `0`
  - **Warnings** _1 - 99_
    - `ALLOK_NOT_FOUND` = `5`
    - `ALLOK_NULL_PARAM` = `10`
    - `ALLOK_INVALID_SIZE` = `11`
    - `ALLOK_INVALID_ADDR` = `12`
    - `ALLOK_UNINITIALIZED` = `15`
  - **Serious Warnings** _100 - 999_
    - `ALLOK_INSUFFICIENT_ARENA_MEMORY` = `100`
    - `ALLOK_INSUFFICIENT_POOL_MEMORY` = `150`
  - **Errors** _>1000_ 
    - `ALLOK_OS_MEMORY_ALLOC_FAILED` = `1000`


- `AllokType`**enum**
  - `ALLOK_LINEAR_FIT` = `0`
  - `ALLOK_FIRST_FIT` = `1`
  - `ALLOK_BEST_FIT` = `2`
  - `ALLOK_WORST_FIT` = `3`


- `AkMemoryArena`
- `AkMemoryBlock`
- `AkMemoryPool`


- `AkMemoryMap`
- `AkMemoryMapParams`
- `AkMemoryMapMetadata`

### Macros
- `ALLOK_DEFAULT_POOL_COUNT` = `0`
- `ALLOK_DEFAULT_POOL_SIZE` = `(8 * 1024)`
- `ALLOK_DEFAULT_ALLOC_TYPE` = `ALLOK_BEST_FIT`
- `ALLOK_DEFAULT_ALLOC_DYNAMIC` = `ALLOK_TRUE`
- `ALLOK_NULL` = `((void *)0)`
- `VPTR(p)` = `((void **)(&p))`
