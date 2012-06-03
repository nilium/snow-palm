/*
  Memory pool system
  Written by Noel Cower

  See LICENSE.md for license information
*/

#include "memory_pool.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if defined(__cplusplus)
extern "C"
{
#endif

#define USE_MEMORY_GUARD 1

/*!
 * The minimum size of a memory pool.  This number is kind of arbitrary, but by
 * default the minimum is the size of four minimum-size blocks.
 */
#define MIN_POOL_SIZE (MIN_BLOCK_SIZE * 4)
/*! Default memory pool size for main pools. */
#define DEFAULT_POOL_SIZE (8/*mb*/ * 1024/*kb*/ * 1024/*b*/)
/*! Alignment for memory blocks.  Must be a power of two minus one. */
#define BLOCK_ALIGNMENT (16)
#if USE_MEMORY_GUARD
#define MEMORY_GUARD_SIZE (sizeof(uint32_t))
#else
#define MEMORY_GUARD_SIZE (0)
#endif /* USE_MEMORY_GUARD */
/*! Macro for quickly getting the actual size of a block given a requested size. */
#define BLOCK_SIZE(SZ) ((buffersize_t)((SZ) + sizeof(block_head_t) + (BLOCK_ALIGNMENT) & ~(BLOCK_ALIGNMENT - 1) + MEMORY_GUARD_SIZE))
/*! Minimum allocation size - defaults to larger of a pointer or size_t. */
#define MIN_ALLOC_SIZE (sizeof(void *) >= sizeof(size_t) ? sizeof(void *) : sizeof(size_t))
/*! Size of a block for a minimum-size allocation. */
#define MIN_BLOCK_SIZE BLOCK_SIZE(MIN_ALLOC_SIZE)
/*! Memory guard value - used to determine if something has written outside the
  bounds of a block. */
#define MEMORY_GUARD (0xD3ADBE3F)

#if !defined(MAIN_POOL_SIZE)
#define MAIN_POOL_SIZE DEFAULT_POOL_SIZE
#endif

/*! The main memory pool. */
static memory_pool_t g_main_pool;


/*!
 * Prints a block's properties to stderr.
 * @param block The block to write to stderr.
 */
static inline void dbg_print_block(const block_head_t *block);
/*!
 * Checks a pool's blocks for problems.
 * @param pool The pool to check.
 */
static void mem_check_pool(const memory_pool_t *pool);
/*!
 * Checks a block for problems (incorrect size, corrupt memory guard, NULL
 * pool/neighboring block pointers).
 * @param block The block to check.
 */
static void mem_check_block(const block_head_t *block, int debug_block);



void sys_mem_init(void)
{
  if (g_main_pool.head.used) return;
  
  mem_init_pool(&g_main_pool, MAIN_POOL_SIZE, MAIN_POOL_TAG);
  g_main_pool.refs = 0;
}


void sys_mem_shutdown(void)
{
  if (!g_main_pool.head.used) return;
  
  mem_destroy_pool(&g_main_pool);
}


void mem_init_pool(memory_pool_t *pool, buffersize_t size)
{
  if (!pool->buffer) {
    if (size < MIN_POOL_SIZE) {
      size = MIN_POOL_SIZE;
    }
    log_note("Initializing memory pool (%p) with size %zu\n", (const void *)pool, size);

    mutex_init(&pool->lock, YES);
    mutex_lock(&pool->lock);

    buffersize_t buffer_size = (size + BLOCK_ALIGNMENT) & ~(BLOCK_ALIGNMENT - 1);
    if (size < MIN_POOL_SIZE)
      size = MIN_POOL_SIZE;

    pool->size = buffer_size;
    pool->buffer = (char *)malloc(buffer_size);

    block_head_t *block = (block_head_t *)(((unsigned long)pool->buffer + (BLOCK_ALIGNMENT)) & ~(BLOCK_ALIGNMENT - 1));
    block->size = size;
    block->used = 0;
    block->next = &pool->head;
    block->prev = &pool->head;
    block->pool = pool;

    pool->head.used = 1;
    pool->head.size = 0;
    pool->head.next = block;
    pool->head.prev = block;
    pool->head.pool = pool;
    pool->next_unused = block;
    pool->sequence = 1;
    pool->refs = 1;
    mutex_unlock(&pool->lock);
  } else {
    log_error("Attempt to initialize already-initialized memory pool (%p) with new\n", (const void *)pool);
  }
}


void mem_destroy_pool(memory_pool_t *pool)
{
  if (pool->head.used) {
    mutex_lock(&pool->lock);

    if (pool->refs != 0) {
      log_warning("Destroying memory pool with non-zero reference count (%d)\n", pool->refs);
    }

    mem_check_pool(pool);
  
    free(pool->buffer);
    pool->buffer = NULL;
    pool->head.next = NULL;
    pool->head.prev = NULL;
    pool->next_unused = NULL;
    pool->head.used = 0;
    pool->sequence = 0;
    pool->refs = 0;
  
    mutex_unlock(&pool->lock);
    mutex_destroy(&pool->lock);

    log_note("Destroyed pool (%p)\n", (const void *)pool);
  } else {
    log_error("Attempt to destroy already-destroyed memory pool (%p)\n", (const void *)pool);
  }
}


memory_pool_t *mem_retain_pool(memory_pool_t *pool)
{
  if (!pool) return pool;

  mutex_lock(&pool->lock);
  ++pool->refs;
  mutex_unlock(&pool->lock);
  return pool;
}


void mem_release_pool(memory_pool_t *pool)
{
  if (!pool) return;
  mutex_lock(&pool->lock);
  int32_t refs = --pool->refs;
  mutex_unlock(&pool->lock);
  if (refs == 0) {
    mem_destroy_pool(pool);
  }
}

#if NDEBUG
void *mem_alloc(memory_pool_t *pool, buffersize_t size, int32_t tag)
#else
void *mem_alloc_debug(memory_pool_t *pool, buffersize_t size, int32_t tag, const char *file, const char *function, int32_t line)
#endif
{
  if (pool == NULL)
    pool = &g_main_pool;
  
  mutex_lock(&pool->lock);
  
  if (tag == 0) {
    log_error("Allocation failed - invalid tag %X\n", tag);
    goto alloc_unlock_and_exit;
  }
  
  if (!pool->head.used) {
    log_error("Allocation failed - pool is not initialized or corrupt\n");
    goto alloc_unlock_and_exit;
  }
  
  buffersize_t block_size = BLOCK_SIZE(size);
  if (block_size < MIN_BLOCK_SIZE) {
    block_size = MIN_BLOCK_SIZE;
    log_warning("Allocation of %zu is too small, allocating minimum size of %zu instead\n", size, MIN_ALLOC_SIZE);
  }
  
  if (block_size > pool->size) {
    log_error("Allocation failed - requested size %zu exceeds pool capacity (%zu)\n", size, pool->size);
    goto alloc_unlock_and_exit;
  }
  
  
  block_head_t *block = pool->next_unused;
  block_head_t *const terminator = pool->next_unused->prev;
  for (; block != terminator; block = block->next) {
    /* skip used blocks and blocks that're too small */
    if (block->used) continue;
    
    if (block->size < block_size) continue;
    
    /* if the free block is large enough to be split into two blocks, do that */
    if ((block->size - block_size) > MIN_BLOCK_SIZE) {
      block_head_t *unused = (block_head_t *)((char *)block + block_size);
      
      unused->size = block->size - block_size;
      block->size = block_size;
      
      unused->used = 0;
      unused->tag = 0;
      unused->pool = pool;
      /*unused->tag = DEFAULT_BLOCK_TAG_UNUSED;*/
      /* update list */
      unused->next = block->next;
      unused->prev = block;
      unused->next->prev = unused;
      block->next = unused;
      
      /*log_note("new unused block created:\n");*/
      /*dbg_print_block(unused);*/
    }
    
    if (pool->sequence == 0) pool->sequence = 1; /* in case of overflow */
    block->used = ++pool->sequence;
    block->tag = tag;
#if USE_MEMORY_GUARD
    ((uint32_t *)((char *)block + block_size))[-1] = MEMORY_GUARD;
#endif
    pool->next_unused = block->next;
    
#if !NDEBUG
    /* store source file, function, line, and requested size in debugging struct */

    size_t file_length = strlen(file) + 1;
    size_t fn_length = strlen(function) + 1; /* account for \0 in both cases */
    /* to avoid pointless fragmentation, we'll allocate this using malloc normally */
    char *file_copy = (char *)malloc((file_length + fn_length) * sizeof(char));
    strncpy(file_copy, file, file_length + 1);
    block->debug_info.source_file = file_copy;
    
    /* copy function as well */
    char *fn_copy = file_copy + file_length;
    strncpy(fn_copy, function, fn_length);
    block->debug_info.function = fn_copy;
    
    block->debug_info.line = line;
    block->debug_info.requested_size = size;
#endif /* !NDEBUG */
    
    mutex_unlock(&pool->lock);
    
    return block + 1;
  }
  
  /* out of memory */
  log_error("Failed to allocate %zu bytes - pool is out of memory\n", size);
  
alloc_unlock_and_exit:
  mutex_unlock(&pool->lock);
  
  return NULL;
}


void mem_free(void *buffer)
{
  if (!buffer) {
    log_error("Free on NULL\n");
    return;
  }
  
  block_head_t *block = (block_head_t *)buffer - 1;
  memory_pool_t *pool = block->pool;
  
  /*log_note("freeing block:\n");*/
  /*dbg_print_block(block);*/
  
  if (!pool) {
    log_error("Attempt to free block without an associated pool\n");
    goto free_unlock_and_exit;
  }
  
  mutex_lock(&pool->lock);
  
  if (block == &block->pool->head) {
    log_error("Free on header block of pool\n");
    goto free_unlock_and_exit;
  }
  
  if (block->size < MIN_BLOCK_SIZE) {
    log_error("Invalid block, too small (%zu) - may be corrupted\n", block->size);
    goto free_unlock_and_exit;
  }
  
#if USE_MEMORY_GUARD
  uint32_t guard = ((uint32_t *)((char *)block + block->size))[-1];
  if (guard != MEMORY_GUARD) {
    log_error("Block memory guard corrupted - reads %X\n", guard);
  }
#endif
  
  if (!block->used) {
    log_error("Double-free on block\n");
    goto free_unlock_and_exit;
  }
  
  /*block->tag = DEFAULT_BLOCK_TAG_UNUSED;*/
  block->used = 0;
  
  if (!block->next->used) {
    block->size += block->next->size;
    block->next = block->next->next;
    block->next->prev = block;
  }
  
  if (!block->prev->used) {
    block = block->prev;
    block->size += block->next->size;
    block->next = block->next->next;
    block->next->prev = block;
  }
  
#if !NDEBUG

  /* clear debug info */
  free(block->debug_info.source_file);
  block->debug_info.source_file = NULL;
  block->debug_info.function = NULL;
  block->debug_info.line = -1;
  block->debug_info.requested_size = 0;

#endif /* !NDEBUG */
  
  pool->next_unused = block;
  
free_unlock_and_exit:
  mutex_unlock(&pool->lock);
}


static inline void dbg_print_block(const block_head_t *block)
{
  /* the odd exception where log_* is not used... */
  fprintf(stderr, "BLOCK [header: %p | buffer: %p] {\n", (void *)block, (void *)(block+1));
  fprintf(stderr, "  guard: %X\n", ((const uint32_t *)((const char *)block+block->size))[-1]);
  fprintf(stderr, "  block size: %zu bytes\n", block->size);
  fprintf(stderr, "  prev: %p\n", block->prev);
  fprintf(stderr, "  next: %p\n", block->next);
  fprintf(stderr, "  used: %d\n", block->used);
  fprintf(stderr, "  tag: %X\n", block->tag);
#if !NDEBUG
  fprintf(stderr, "  source file: %s [%d]\n", block->debug_info.source_file, block->debug_info.line);
  fprintf(stderr, "  source function: %s\n", block->debug_info.function);
  fprintf(stderr, "  buffer size: %zu bytes\n", block->debug_info.requested_size);
#endif /* !NDEBUG */
  fprintf(stderr, "  pool: %p\n}\n", block->pool);
}


static void mem_check_pool(const memory_pool_t *pool)
{
  const block_head_t *block = pool->head.next;
  for (; block != &pool->head; block = block->next) {
    if (block->used) {
      log_note("Used memory block located\n");
    }
    mem_check_block(block, block->used);
  };
}


static void mem_check_block(const block_head_t *block, int debug_block)
{
  if (block->pool && block == &block->pool->head) {
    log_error("[%X] Cannot check pool header block\n", block->pool->tag);
    return;
  }
  
  int spew_block = debug_block;
  
  if (block->size < MIN_BLOCK_SIZE) {
    log_error("Block smaller than minimum required size (%zu) - may be corrupt\n", MIN_BLOCK_SIZE);
  }
  
  if (block->next == NULL || block->prev == NULL) {
    log_error("Block detached from block list\n");
  }
  
#if USE_MEMORY_GUARD
  if (block->used) {
    uint32_t guard = ((const uint32_t *)((const char *)block + block->size))[-1];
    if (guard != MEMORY_GUARD) {
      log_error("Memory guard corrupted\n");
    }
  }
#endif
  
  if (!block->pool) {
    log_error("Block detached from memory pool\n");
  }
  
  if (spew_block) {
    dbg_print_block(block);
  }
}


const block_head_t *mem_get_block(const void *buffer)
{
  const block_head_t *block = (const block_head_t *)buffer - 1;
  
#if USE_MEMORY_GUARD
  uint32_t guard = ((uint32_t *)((const char *)buffer + block->size))[-1];
  if (guard != MEMORY_GUARD) {
    log_error("Memory guard corrupted\n");
    dbg_print_block(block);
    return NULL;
  }
#endif
  
  return block;
}


#if defined(__cplusplus)
}
#endif
