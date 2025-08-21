#include "Memory.h"
#include "Config.h"
#include "Port.h"
#include "Task.h"

#define MEMORYPOOL_SIZE (CONFIG_MEMORYPOOL_SIZE)

#if (!USE_DYNAMIC_MEMORY_ALLOCATION)
#error please set USE_DYNAMIC_MEMORY_ALLOCATION to 1 or remove this file from your project.
#endif

typedef unsigned char byte;

struct MemoryBlockInfo {
    struct MemoryBlockInfo *next_mem_block;
    size_t size;
};

static byte memory_pool[MEMORYPOOL_SIZE];
static byte *memory_pool_end = NULL;
static size_t memory_pool_size = MEMORYPOOL_SIZE;

static struct MemoryBlockInfo *memory_block_head = NULL;

/* 内存块信息结构体大小(已对齐) */
const static size_t MEMORY_BLOCK_STRUCT_SIZE = (sizeof(struct MemoryBlockInfo) + (BYTE_ALIGNMENT - 1)) & ~(BYTE_ALIGNMENT - 1);

/* 内存池初始化 */
static __forceinline void MemoryPool_init() {
    size_t memory_pool_addr = (size_t)memory_pool;

    /* 内存对齐 */
    if (memory_pool_addr & (BYTE_ALIGNMENT - 1)) {
        memory_pool_addr = (memory_pool_addr + (BYTE_ALIGNMENT - 1)) & ~(BYTE_ALIGNMENT - 1);
        memory_pool_size -= memory_pool_addr - (size_t)memory_pool;
    }

    struct MemoryBlockInfo *const mem_block_info = (struct MemoryBlockInfo *)memory_pool_addr;
    mem_block_info->next_mem_block = NULL;
    mem_block_info->size = MEMORY_BLOCK_STRUCT_SIZE;
    memory_block_head = mem_block_info;

    /* 获取内存池末尾地址 */
    memory_pool_addr += memory_pool_size;
    memory_pool_addr = (memory_pool_addr - MEMORY_BLOCK_STRUCT_SIZE) & ~(BYTE_ALIGNMENT - 1);
    memory_pool_end = (byte *)memory_pool_addr;
}

/* 使用首次适应算法分配内存 */
void *Memory_alloc(size_t size) {
    if (memory_block_head == NULL) {
        Task_suspendScheduling();
        MemoryPool_init();
        Task_resumeScheduling();
    }

    if (size == 0) {
        return NULL;
    }

    size += MEMORY_BLOCK_STRUCT_SIZE;

    if (size & (BYTE_ALIGNMENT - 1)) {
        size += BYTE_ALIGNMENT - (size & (BYTE_ALIGNMENT - 1));
    }

    if (memory_pool_size < size) {
        return NULL;
    }

    Task_suspendScheduling();

    struct MemoryBlockInfo *current_mem_block = memory_block_head;

    while (current_mem_block->next_mem_block != NULL) {
        const byte *const next = (byte *)(current_mem_block->next_mem_block);
        const byte *const current = (byte *)current_mem_block + current_mem_block->size;
        if (next - current >= size) {
            break;
        }
        current_mem_block = current_mem_block->next_mem_block;
    }

    byte *const allocated_mem_head = (byte *)current_mem_block + current_mem_block->size;

    /* 找不到可用的内存碎片空间且末尾空间不足 */
    if ((current_mem_block->next_mem_block == NULL) &&
        (allocated_mem_head + size > memory_pool_end)) {
        Task_resumeScheduling();
        return NULL;
    }

    struct MemoryBlockInfo *const mem_block_info = (struct MemoryBlockInfo *)allocated_mem_head;

    mem_block_info->next_mem_block = current_mem_block->next_mem_block;
    mem_block_info->size = size;
    current_mem_block->next_mem_block = mem_block_info;
    memory_pool_size -= size;

    Task_resumeScheduling();

    return (void *)(allocated_mem_head + MEMORY_BLOCK_STRUCT_SIZE);
}

/* 释放内存 */
void Memory_free(void *const ptr) {
    if (ptr == NULL) {
        return;
    }

    const struct MemoryBlockInfo *const mem_block_info = (struct MemoryBlockInfo *)((byte *)ptr - MEMORY_BLOCK_STRUCT_SIZE);

    Task_suspendScheduling();

    struct MemoryBlockInfo *prev_mem_block = memory_block_head;
    while (prev_mem_block->next_mem_block != mem_block_info) {
        prev_mem_block = prev_mem_block->next_mem_block;
    }

    prev_mem_block->next_mem_block = mem_block_info->next_mem_block;
    memory_pool_size += mem_block_info->size;

    Task_resumeScheduling();
}

size_t Memory_getFreeSize() {
    return memory_pool_size;
}
