#include "../include/memory.h"

// Define MEMORY_TRACKING to enable memory tracking
#define MEMORY_TRACKING

// Memory tracking record
typedef struct MemoryRecord {
    void* address;
    size_t size;
    const char* file;
    int line;
    struct MemoryRecord* next;
} MemoryRecord;

// Global variables
static MemoryRecord* g_memory_records = NULL;
static MemoryStats g_memory_stats = {0};
static CRITICAL_SECTION g_memory_lock;
static BOOL g_memory_initialized = FALSE;

// Initialize memory tracking system
void mem_init(void) {
    if (!g_memory_initialized) {
        InitializeCriticalSection(&g_memory_lock);
        memset(&g_memory_stats, 0, sizeof(MemoryStats));
        g_memory_initialized = TRUE;
        printf("Memory tracking system initialized\n");
    }
}

// Free all remaining allocations and shut down memory tracking
void mem_shutdown(void) {
    if (!g_memory_initialized) return;
    
    EnterCriticalSection(&g_memory_lock);
    
    // Report leaks
    if (g_memory_stats.alloc_count > 0) {
        printf("WARNING: %u memory leaks detected (%zu bytes)\n", 
               g_memory_stats.alloc_count, g_memory_stats.total_allocated);
        
        MemoryRecord* current = g_memory_records;
        while (current) {
            printf("  Leak: %p - %zu bytes [%s:%d]\n", 
                   current->address, current->size, current->file, current->line);
            current = current->next;
        }
    }
    
    // Free memory record list
    while (g_memory_records) {
        MemoryRecord* temp = g_memory_records;
        g_memory_records = g_memory_records->next;
        free(temp);
    }
    
    LeaveCriticalSection(&g_memory_lock);
    DeleteCriticalSection(&g_memory_lock);
    
    g_memory_initialized = FALSE;
    printf("Memory tracking system shut down\n");
}

// Add a memory allocation record
static void add_memory_record(void* address, size_t size, const char* file, int line) {
    if (!g_memory_initialized) mem_init();
    
    EnterCriticalSection(&g_memory_lock);
    
    // Create a new record
    MemoryRecord* record = (MemoryRecord*)malloc(sizeof(MemoryRecord));
    if (record) {
        record->address = address;
        record->size = size;
        record->file = file;
        record->line = line;
        record->next = g_memory_records;
        g_memory_records = record;
        
        // Update statistics
        g_memory_stats.total_allocated += size;
        g_memory_stats.alloc_count++;
        g_memory_stats.total_allocs++;
        
        if (g_memory_stats.total_allocated > g_memory_stats.peak_allocated) {
            g_memory_stats.peak_allocated = g_memory_stats.total_allocated;
        }
    }
    
    LeaveCriticalSection(&g_memory_lock);
}

// Remove a memory allocation record
static size_t remove_memory_record(void* address) {
    if (!g_memory_initialized) return 0;
    
    size_t size = 0;
    
    EnterCriticalSection(&g_memory_lock);
    
    MemoryRecord** pp = &g_memory_records;
    MemoryRecord* current = g_memory_records;
    
    // Find the record
    while (current) {
        if (current->address == address) {
            // Remove it from the list
            *pp = current->next;
            size = current->size;
            
            // Update statistics
            g_memory_stats.total_allocated -= size;
            g_memory_stats.alloc_count--;
            g_memory_stats.total_frees++;
            
            free(current);
            break;
        }
        
        pp = &current->next;
        current = current->next;
    }
    
    LeaveCriticalSection(&g_memory_lock);
    return size;
}

// Memory allocation tracking
void* mem_alloc(size_t size, const char* file, int line) {
    void* ptr = malloc(size);
    
    if (ptr) {
        add_memory_record(ptr, size, file, line);
    }
    
    return ptr;
}

void* mem_calloc(size_t num, size_t size, const char* file, int line) {
    void* ptr = calloc(num, size);
    
    if (ptr) {
        add_memory_record(ptr, num * size, file, line);
    }
    
    return ptr;
}

void* mem_realloc(void* ptr, size_t size, const char* file, int line) {
    if (ptr) {
        // Remove the old record
        remove_memory_record(ptr);
    }
    
    void* new_ptr = realloc(ptr, size);
    
    if (new_ptr) {
        add_memory_record(new_ptr, size, file, line);
    }
    
    return new_ptr;
}

void mem_free(void* ptr, const char* file, int line) {
    if (!ptr) return;
    
    // Remove the record
    size_t size = remove_memory_record(ptr);
    
    if (size == 0) {
        // This might be a double-free or freeing unallocated memory
        printf("WARNING: Freeing untracked memory at %p [%s:%d]\n", ptr, file, line);
    }
    
    free(ptr);
}

char* mem_strdup(const char* str, const char* file, int line) {
    if (!str) return NULL;
    
    size_t size = strlen(str) + 1;
    char* new_str = (char*)mem_alloc(size, file, line);
    
    if (new_str) {
        memcpy(new_str, str, size);
    }
    
    return new_str;
}

// Generate memory usage report
void mem_report(void) {
    if (!g_memory_initialized) return;
    
    EnterCriticalSection(&g_memory_lock);
    
    printf("Memory Usage Report:\n");
    printf("  Current allocations: %u (%zu bytes)\n", 
           g_memory_stats.alloc_count, g_memory_stats.total_allocated);
    printf("  Peak memory usage:   %zu bytes\n", g_memory_stats.peak_allocated);
    printf("  Total allocations:   %u\n", g_memory_stats.total_allocs);
    printf("  Total frees:         %u\n", g_memory_stats.total_frees);
    
    // Uncomment to print detailed allocation records
    /*
    if (g_memory_stats.alloc_count > 0) {
        printf("\nActive allocations:\n");
        MemoryRecord* current = g_memory_records;
        while (current) {
            printf("  %p - %zu bytes [%s:%d]\n", 
                   current->address, current->size, current->file, current->line);
            current = current->next;
        }
    }
    */
    
    LeaveCriticalSection(&g_memory_lock);
}

// Get memory statistics
MemoryStats mem_get_stats(void) {
    MemoryStats stats = {0};
    
    if (g_memory_initialized) {
        EnterCriticalSection(&g_memory_lock);
        stats = g_memory_stats;
        LeaveCriticalSection(&g_memory_lock);
    }
    
    return stats;
} 