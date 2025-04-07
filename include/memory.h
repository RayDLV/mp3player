#ifndef MEMORY_H
#define MEMORY_H

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Memory tracking statistics
typedef struct {
    size_t total_allocated;     // Total memory currently allocated
    size_t peak_allocated;      // Peak memory usage
    unsigned int alloc_count;   // Number of active allocations
    unsigned int total_allocs;  // Total number of allocations made
    unsigned int total_frees;   // Total number of frees made
} MemoryStats;

// Memory tracking functions
void* mem_alloc(size_t size, const char* file, int line);
void* mem_calloc(size_t num, size_t size, const char* file, int line);
void* mem_realloc(void* ptr, size_t size, const char* file, int line);
void mem_free(void* ptr, const char* file, int line);
char* mem_strdup(const char* str, const char* file, int line);

// Memory management functions
void mem_init(void);
void mem_shutdown(void);
void mem_report(void);
MemoryStats mem_get_stats(void);

// Helper macros to automatically include file and line info
#ifdef MEMORY_TRACKING
    #define MEM_ALLOC(size) mem_alloc(size, __FILE__, __LINE__)
    #define MEM_CALLOC(num, size) mem_calloc(num, size, __FILE__, __LINE__)
    #define MEM_REALLOC(ptr, size) mem_realloc(ptr, size, __FILE__, __LINE__)
    #define MEM_FREE(ptr) mem_free(ptr, __FILE__, __LINE__)
    #define MEM_STRDUP(str) mem_strdup(str, __FILE__, __LINE__)
#else
    #define MEM_ALLOC(size) malloc(size)
    #define MEM_CALLOC(num, size) calloc(num, size)
    #define MEM_REALLOC(ptr, size) realloc(ptr, size)
    #define MEM_FREE(ptr) free(ptr)
    #define MEM_STRDUP(str) _strdup(str)
#endif

#endif // MEMORY_H 