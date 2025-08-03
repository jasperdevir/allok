cmake_minimum_required(VERSION 3.20)

project(allok)
set(CMAKE_C_STANDARD 11)

set(SRC_DIR ./src)
set(INCLUDE_DIR ./include)
set(EXAMPLE_DIR ./example)
set(LIB_DIR ${CMAKE_SOURCE_DIR}/lib)
set(BIN_DIR ${CMAKE_SOURCE_DIR}/bin)

set(ALLOK_STATIC ON)
set(ALLOK_BUILD_EXAMPLE ON)

file(MAKE_DIRECTORY ${LIB_DIR})

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${LIB_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${LIB_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${LIB_DIR})

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${LIB_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${LIB_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${LIB_DIR})

if(ALLOK_STATIC)
    add_library(allok STATIC ${SRC_DIR}/allok.c)
else()
    add_library(allok SHARED ${SRC_DIR}/allok.c)
endif()

target_include_directories(allok PUBLIC ${INCLUDE_DIR})

if (CMAKE_C_COMPILER_ID STREQUAL "Clang" OR CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(allok PRIVATE -Wall -Wextra)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(allok PRIVATE -g -O0)
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(allok PRIVATE -O3 -DNDEBUG)
    endif()
elseif(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(allok PRIVATE /W4)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(allok PRIVATE /Od /Zi)
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(allok PRIVATE /O2 /DNDEBUG)
    endif()
endif()

if(ALLOK_BUILD_EXAMPLE)
    project(allok_example)

    file(MAKE_DIRECTORY ${BIN_DIR})

    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BIN_DIR})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${BIN_DIR})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${BIN_DIR})

    add_executable(allok_example ${EXAMPLE_DIR}/example.c)
    target_include_directories(allok_example PUBLIC ${INCLUDE_DIR})
    target_link_libraries(allok_example PUBLIC allok)
endif()
