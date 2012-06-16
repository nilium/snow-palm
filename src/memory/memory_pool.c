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

typedef uint32_t guard_t;

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
#define MEMORY_GUARD_SIZE (sizeof(guard_t))
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
/*!
 * Returns whether a particular block can be split into a new block of
 * pred_size and its difference.
 */
static int mem_can_split_block(block_head_t *block, buffersize_t pred_size);
/*!
 * Splits a block into two blocks, the original block sized pred_size and the
 * new block taking up the difference.
 */
static int mem_split_block(block_head_t *block, buffersize_t pred_size);
/*!
 * Combines two blocks into one. They must be seated next to each other, or
 * the function will return -1 (failure). Returns 0 on success.
 */
static int mem_merge_blocks(block_head_t *blka, block_head_t *blkb);



void sys_mem_init(allocator_t *alloc)
{
  if (g_main_pool.head.used) return;

  mem_init_pool(&g_main_pool, MAIN_POOL_SIZE, alloc);
  g_main_pool.refs = 0;
}


void sys_mem_shutdown(void)
{
  if ( ! g_main_pool.head.used) return;

  mem_destroy_pool(&g_main_pool);
}


void mem_init_pool(memory_pool_t *pool, buffersize_t size, allocator_t *alloc)
{
  if (alloc == NULL)
    alloc = g_default_allocator;

  if ( ! pool->buffer) {
    if (size < MIN_POOL_SIZE) {
      size = MIN_POOL_SIZE;
    }
    s_log_note("Initializing memory pool (%p) with size %zu", (const void *)pool, size);

    mutex_init(&pool->lock, true);
    mutex_lock(&pool->lock);

    buffersize_t buffer_size = (size + BLOCK_ALIGNMENT) & ~(BLOCK_ALIGNMENT - 1);
    if (size < MIN_POOL_SIZE)
      size = MIN_POOL_SIZE;

    pool->alloc = alloc;
    pool->size = buffer_size;
    pool->buffer = (char *)com_malloc(alloc, buffer_size);

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
    s_log_error("Attempt to initialize already-initialized memory pool (%p) with new", (const void *)pool);
  }
}


void mem_destroy_pool(memory_pool_t *pool)
{
  if (pool->head.used) {
    mutex_lock(&pool->lock);

    if (pool->refs != 0) {
      s_log_warning("Destroying memory pool with non-zero reference count (%d)", pool->refs);
    }

    mem_check_pool(pool);

    com_free(pool->alloc, pool->buffer);
    pool->buffer = NULL;
    pool->head.next = NULL;
    pool->head.prev = NULL;
    pool->next_unused = NULL;
    pool->head.used = 0;
    pool->sequence = 0;
    pool->refs = 0;

    mutex_unlock(&pool->lock);
    mutex_destroy(&pool->lock);

    s_log_note("Destroyed pool (%p)", (const void *)pool);
  } else {
    s_log_error("Attempt to destroy already-destroyed memory pool (%p)", (const void *)pool);
  }
}


memory_pool_t *mem_retain_pool(memory_pool_t *pool)
{
  if ( ! pool) return pool;

  mutex_lock(&pool->lock);
  ++pool->refs;
  mutex_unlock(&pool->lock);
  return pool;
}


void mem_release_pool(memory_pool_t *pool)
{
  if ( ! pool) return;
  mutex_lock(&pool->lock);
  int32_t refs = --pool->refs;
  mutex_unlock(&pool->lock);
  if (refs == 0) {
    mem_destroy_pool(pool);
  }
}


static int mem_can_split_block(block_head_t *block, buffersize_t pred_size)
{
  if (block->size < pred_size)
    return 0;

  return ((block->size - pred_size) > MIN_BLOCK_SIZE);
}


static int mem_split_block(block_head_t *block, buffersize_t pred_size)
{
  if (block->size < pred_size) {
    /* original block is too small to split */
    return -1;
  }

  /* check if the block can be split */
  if ((block->size - pred_size) > MIN_BLOCK_SIZE) {
    block_head_t *unused = (block_head_t *)((char *)block + pred_size);

    unused->size = block->size - pred_size;
    block->size = pred_size;

    unused->used = 0;
    unused->tag = 0;
    unused->pool = block->pool;
    /* update list */
    unused->next = block->next;
    unused->prev = block;
    unused->next->prev = unused;
    block->next = unused;

    return 0;
    }

    /* new block is too small to split */
    return -1;
}


static int mem_merge_blocks(block_head_t *blka, block_head_t *blkb)
{
  if ( ! blka || ! blkb) {
    s_log_error("Attempt to join one or more NULL blocks.");
    return -1;
  }

  /* swap blocks if they're out of order */
  if (blkb < blka) {
    block_head_t *swap = blka;
    blka = blkb;
    blkb = swap;
  }

  /* make sure blocks are adjacent */
  if (blka->next != blkb) {
    s_log_error("Attempt to join non-adjacent memory blocks.");
    return -2;
  }

  blka->next = blkb->next;
  blka->next->prev = blka;
  blka->size += blkb->size;

  return 0;
}


#if NDEBUG
void *mem_alloc(memory_pool_t *pool, buffersize_t size, int32_t tag)
#else
void *mem_alloc_debug(memory_pool_t *pool, buffersize_t size, int32_t tag, const char *file, const char *function, int32_t line)
#endif
{
  block_head_t *block = NULL;
  block_head_t *terminator = NULL;
  buffersize_t block_size;

  if (pool == NULL)
    pool = &g_main_pool;

  mutex_lock(&pool->lock);

  if (tag == 0) {
    s_log_error("Allocation failed - invalid tag %X", tag);
    goto alloc_unlock_and_exit;
  }

  if ( ! pool->head.used) {
    s_log_error("Allocation failed - pool is not initialized or corrupt");
    goto alloc_unlock_and_exit;
  }

  block_size = BLOCK_SIZE(size);

  if (block_size < MIN_BLOCK_SIZE) {
    block_size = MIN_BLOCK_SIZE;
    s_log_warning("Allocation of %zu is too small, allocating minimum size of %zu instead", size, MIN_ALLOC_SIZE);
  }

  if (block_size > pool->size) {
    s_log_error("Allocation failed - requested size %zu exceeds pool capacity (%zu)", size, pool->size);
    goto alloc_unlock_and_exit;
  }


  block = pool->next_unused;

  if ( ! block) {
    s_log_error("Allocation failed - pool corrupted.");
    return NULL;
  }

  terminator = pool->next_unused->prev;

  if ( ! terminator) {
    s_log_error("Allocation failed - pool corrupted.");
    return NULL;
  }

  for (; block != terminator; block = block->next) {
    /* skip used blocks and blocks that're too small */
    if (block->used)
      continue;
    else if (block->size < block_size)
      continue;

    /* if the free block is large enough to be split into two blocks, do that */
    if (mem_can_split_block(block, block_size))
      if (mem_split_block(block, block_size))
        s_log_error("Failed to split block, using unsplit block.");

    if (pool->sequence == 0)
      pool->sequence = 1; /* in case of overflow */

    block->used = ++pool->sequence;
    block->tag = tag;


#if USE_MEMORY_GUARD
    ((guard_t *)((char *)block + block_size))[-1] = MEMORY_GUARD;
#endif


#if !NDEBUG
    /* store source file, function, line, and requested size in debugging struct */

    size_t file_length = strlen(file) + 1;
    size_t fn_length = strlen(function) + 1; /* account for \0 in both cases */
    /* to avoid pointless fragmentation, we'll allocate this using malloc normally */
    char *file_copy = (char *)com_malloc(pool->alloc, (file_length + fn_length) * sizeof(char));
    strncpy(file_copy, file, file_length + 1);
    block->debug_info.source_file = file_copy;

    /* copy function as well */
    char *fn_copy = file_copy + file_length;
    strncpy(fn_copy, function, fn_length);
    block->debug_info.function = fn_copy;

    block->debug_info.line = line;
    block->debug_info.requested_size = size;
#endif /* !NDEBUG */

    pool->next_unused = block->next;

    mutex_unlock(&pool->lock);

    return block + 1;
  }

  /* out of memory */
  s_log_error("Failed to allocate %zu bytes - pool is out of memory", size);

alloc_unlock_and_exit:
  mutex_unlock(&pool->lock);

  return NULL;
}


/*
  mem_realloc tries to make reallocation as cheap as possible by expanding its
  block to fit the new size. This is done by splitting previous/next blocks in
  the pool and merging those blocks.

  The best case scenario is that the new block is smaller. In that case, the
  block may not be resized at all.  It will be split if it can, but it's not
  an error if the block isn't resized at all.

  Second best case is that the next block in the pool can be split and merged
  with the original block. If it can't be split, it's not used, period.

  Third best case is that the previous block can be split and merged, with the
  original block memmove'd into place.

  Worst case is a new block is allocated, the contents memcpy'd to the new
  block, and then the old block is released.

  In the event of an error, NULL is returned and a log message is written
  describing what went wrong.
*/
void *mem_realloc(void *p, buffersize_t size)
{
  block_head_t *block;
  memory_pool_t *pool;
  size_t new_size;
  off_t size_diff;

  if ( ! p) {
    s_log_error("Realloc on NULL");
    return NULL;
  }

  block = (block_head_t *)p - 1;
  pool = block->pool;

  if ( ! pool) {
    s_log_error("Attempt to reallocate block without an associated pool");
    return NULL;
  }

  mutex_lock(&pool->lock);

  if (block == &block->pool->head) {
    s_log_error("Realloc on header block of pool");
    p = NULL;
    goto mem_realloc_exit;
  }

  new_size = BLOCK_SIZE(size);

  if (new_size < MIN_BLOCK_SIZE)
    new_size = MIN_BLOCK_SIZE; /* new size cannot go below the minimum
                                    block size */

  size_diff = (off_t)new_size - (off_t)block->size;

  if (size_diff == 0)
    return p; /* not resized at all */
  if (size_diff < 0 && abs(size_diff) < MIN_BLOCK_SIZE)
    return p; /* the size difference is small enough that resizing is
                 pointless, so we won't bother doing it */

  if (size_diff < 0) {
    /* new block is smaller, see if we can split it */
    if (mem_can_split_block(block, new_size)) {
      /* if we can split it, do so */
      if (mem_split_block(block, new_size) == 0) {
        p = block + 1;

        if ( ! block->next->next->used) {
          mem_merge_blocks(block->next, block->next->next);
        }

        pool->next_unused = block->next;
      } else {
        s_log_warning("Failed to split block, using unsplit block.");
      }
    } else {
      s_log_warning("New block size is too small to resize, leaving as-is.");
    }

    /* if the block can't be split, leave it as is -- the size difference is
       small enough that resizing is probably pointless. */

  } else if ( ! block->next->used && (block->next->size - size_diff) >= MIN_BLOCK_SIZE) {
    /* if the next block is unused, try to join it with that.
       not using mem_split_block because the new block is the only one that
       has to be valid -- in a sense, it's more like moving a block. */
    block_head_t copy = *block->next;
    block_head_t *split = (block_head_t *)((char *)block->next + size_diff);

    /* copy old to new */
    *split = copy;

    /* reset pointers */
    split->next->prev = split;
    split->prev->next = split;

#if USE_MEMORY_GUARD
    /* put memory guard in place (if in use) */
    ((guard_t *)((char *)block + new_size))[-1] = MEMORY_GUARD;
#endif /* USE_MEMORY_GUARD */

    /* reset next unused pointer */
    pool->next_unused = split;

    /* done */
    block->size = new_size;
  } else if ( ! block->prev->used) {
    /* if the last block is unused, try to join it with that */
    block_head_t *split = (block_head_t *)((char *)block->prev + (block->size - size_diff));
    block->prev->size -= size_diff;

    /* copy old to new */
    memmove(split, block, block->size);

    /* reset pointers */
    split->next->prev = split;
    split->prev->next = split;

#if USE_MEMORY_GUARD
    ((guard_t *)((char *)block + new_size))[-1] = MEMORY_GUARD;
#endif /* USE_MEMORY_GUARD */

    /* again, reset next unused pointer */
    pool->next_unused = block->prev;

    block->size = new_size;
    p = block+1;
  } else {
    /* last resort: allocate a new block, copy, free the old block */
    void *new_p = mem_alloc(block->pool, size, block->tag);

    if (new_p) {
      memcpy(new_p, p, block->size - sizeof(block_head_t));
      mem_free(p);
    } else {
      s_log_error("Failed to allocate new memory block for realloc");
    }

    p = new_p;
  }

  mem_realloc_exit:
  mutex_unlock(&pool->lock);
  return p;
}


void mem_free(void *buffer)
{
  if ( ! buffer) {
    s_log_error("Free on NULL");
    return;
  }

  block_head_t *block = (block_head_t *)buffer - 1;
  memory_pool_t *pool = block->pool;

  /*s_log_note("freeing block:");*/
  /*dbg_print_block(block);*/

  if ( ! pool) {
    s_log_error("Attempt to free block without an associated pool");
    return;
  }

  mutex_lock(&pool->lock);

  if (block == &block->pool->head) {
    s_log_error("Free on header block of pool");
    goto free_unlock_and_exit;
  }

  if (block->size < MIN_BLOCK_SIZE) {
    s_log_error("Invalid block, too small (%zu) - may be corrupted", block->size);
    goto free_unlock_and_exit;
  }

#if USE_MEMORY_GUARD
  guard_t guard = ((guard_t *)((char *)block + block->size))[-1];
  if (guard != MEMORY_GUARD) {
    s_log_error("Block memory guard corrupted - reads %X", guard);
  }
#endif

  if ( ! block->used) {
    s_log_error("Double-free on block");
    goto free_unlock_and_exit;
  }

  /*block->tag = DEFAULT_BLOCK_TAG_UNUSED;*/
  block->used = 0;

  if ( ! block->next->used) {
    block->size += block->next->size;
    block->next = block->next->next;
    block->next->prev = block;
  }

  if ( ! block->prev->used) {
    block = block->prev;
    block->size += block->next->size;
    block->next = block->next->next;
    block->next->prev = block;
  }

#if !NDEBUG

  /* clear debug info */
  com_free(pool->alloc, block->debug_info.source_file);
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
  /* the odd exception where s_log_* is not used... */
  fprintf(stderr, "BLOCK [header: %p | buffer: %p] {\n", (void *)block, (void *)(block+1));
  fprintf(stderr, "  guard: %X\n", ((const guard_t *)((const char *)block+block->size))[-1]);
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
  if (pool == NULL) {
    s_fatal_error(1, "Attempt to check NULL pool.");
    return;
  }

  if (pool->alloc == NULL) {
    s_fatal_error(1, "Pool allocator is NULL.");
    return;
  }

  const block_head_t *block = pool->head.next;
  for (; block != &pool->head; block = block->next) {
    if (block) {
      mem_check_block(block, block->used);
    } else {
      s_fatal_error(1, "Memory pool links are corrupted.");
      return;
    }
  };
}


static void mem_check_block(const block_head_t *block, int debug_block)
{
  if (block->pool && block == &block->pool->head) {
    s_log_error("Cannot check pool header block");
    return;
  }

  int spew_block = debug_block;

  if (block->size < MIN_BLOCK_SIZE) {
    s_log_error("Block smaller than minimum required size (%zu) - may be corrupt", MIN_BLOCK_SIZE);
  }

  if (block->next == NULL || block->prev == NULL) {
    s_log_error("Block detached from block list");
  }

#if USE_MEMORY_GUARD
  if (block->used) {
    guard_t guard = ((const guard_t *)((const char *)block + block->size))[-1];
    if (guard != MEMORY_GUARD) {
      s_log_error("Memory guard corrupted");
    }
  }
#endif

  if ( ! block->pool) {
    s_log_error("Block detached from memory pool");
  }

  if (spew_block) {
    dbg_print_block(block);
  }
}


const block_head_t *mem_get_block(const void *buffer)
{
  const block_head_t *block = (const block_head_t *)buffer - 1;

#if USE_MEMORY_GUARD
  guard_t guard = ((guard_t *)((const char *)buffer + block->size))[-1];
  if (guard != MEMORY_GUARD) {
    s_log_error("Memory guard corrupted");
    dbg_print_block(block);
    return NULL;
  }
#endif

  return block;
}

///////////////////////////////////////////////////////////////////////////////
////                             Pool Allocator                            ////
///////////////////////////////////////////////////////////////////////////////

#define POOL_ALLOCATOR_TAG (-1)

static void *al_pool_malloc(size_t min_size, void *ctx);
static void *al_pool_realloc(void *p, size_t min_size, void *ctx);
static void al_pool_free(void *p, void *ctx);


allocator_t pool_allocator(memory_pool_t *pool)
{
  allocator_t alloc = {
    .malloc = al_pool_malloc,
    .realloc = al_pool_realloc,
    .free = al_pool_free,
    .context = pool
  };
  return alloc;
}


static void *al_pool_malloc(size_t min_size, void *ctx)
{
  return mem_alloc((memory_pool_t *)ctx, min_size, POOL_ALLOCATOR_TAG);
}


static void *al_pool_realloc(void *p, size_t min_size, void *ctx)
{
  if (p)
    return mem_realloc(p, min_size);
  else
    return mem_alloc((memory_pool_t *)ctx, min_size, POOL_ALLOCATOR_TAG);
}


static void al_pool_free(void *p, void *ctx)
{
  (void)ctx;
  mem_free(p);
}


#if defined(__cplusplus)
}
#endif
