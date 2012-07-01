/*
  Memory pool system
  Written by Noel Cower

  See LICENSE.md for license information
*/

#ifndef __SNOW__MEMORY_POOL_H__
#define __SNOW__MEMORY_POOL_H__

#include <snow-config.h>
#include <threads/mutex.h>
#include "allocator.h"

/*!
  \file

  The API for creating, destroying, and allocating from memory pools. Memory
  pools are essentially linked lists that represent chunks of memory in a
  larger chunk of memory. The purpose of this is to ensure that, when
  necessary, memory that ought to be close together is. There is no solid way
  to reduce fragmentation, but ::pool_malloc will always attempt to use the next
  free or most recently freed block of memory for any new allocations.

  \par Thread Safety
  The only operations wrapped in a lock are ::mem_destroy_pool,
  ::mem_retain_pool, ::mem_release_pool, ::pool_malloc, and ::pool_free. Others
  do not get that treatment. If you need to lock the pool for another reason,
  the pool_t::lock member is there, but it's advised that you don't try
  to ever fiddle with pool internals.
*/

#ifdef __SNOW__MEMORY_POOL_C__
#define S_INLINE
#else
#define S_INLINE inline
#endif

#if defined(__cplusplus)
extern "C"
{
#endif

#define SBSFMT "%zu"
#define SBDFMT "%td"

typedef size_t buffersize_t;
typedef ptrdiff_t bufferdiff_t;

typedef struct s_block_head block_head_t;
typedef struct s_pool pool_t;

/*!
 * Memory block header. This is mainly for internal and debugging use.
 */
struct s_block_head
{
  /*! Whether the block is in use. Zero if not in use, one if a header
      block, otherwise a regular block. */
  int32_t used;
  /*! Identifying tag for the block. Zero if unused. */
  int32_t tag;
  /*! Size of the memory block. Includes header, memory guard, and
      alignment adjustment. */
  buffersize_t size;
  /*! Next block in the memory pool. */
  block_head_t *prev;
  /*! Previous block in the memory pool. */
  block_head_t *next;
  /*! Pointer back to the memory pool the block belongs to. */
  pool_t *pool;

#if !NDEBUG

  /* debugging info for tracking allocations */
  struct
  {
    /* the source file this block was allocated from */
    char *source_file;
    /* the function this block was allocated from */
    char *function;
    /* the line in the source file that this block was allocated from */
    int32_t line;
    /* the size requested (this always differs from the above size) */
    buffersize_t requested_size;
  } debug_info;

#endif
};

/*!
 * Memory pool structure. Do not touch its members unless you want to break stuff.
 */
struct s_pool
{
  allocator_t *alloc;
  /*! Size of the memory pool. Includes adjustment for alignment. */
  buffersize_t size;
  /*! Counter for block allocation - can overflow. */
  int32_t sequence;
  /*! The memory used by the memory pool. */
  char *buffer;
  /*! Whether the buffer should be freed on destruction. If managed, free the
      memory. If not, do nothing to it. */
  bool managed;
  /*! The next free block of memory. */
  block_head_t *next_unused;
  /*! Header block - size is always 0, used is always 1, etc. */
  block_head_t head;
  /*! Pool lock */
  mutex_t lock;
};

/*!
 * Initializes the global memory pool.
 */
void sys_pool_init(allocator_t *alloc);

/*!
 * Destroys the global memory pool. This will not destroy any other pools.
 */
void sys_pool_shutdown(void);

/*!
 * Initializes a new memory pool. Newly-initialized pools have a retain-count of 1.
 *
 * \param[inout]  pool The address of an uninitialized pool to be initialized.
 * \param[in]   size The size of the memory pool to initialize.
 */
int pool_init(pool_t *pool, buffersize_t size, allocator_t *alloc);

/*!
 * Initializes a new memory pool with an existing block of memory.
 */
int pool_init_with_pointer(pool_t *pool, void *p, buffersize_t size, allocator_t *alloc);

/*!
 * Destroys a memory pool.
 * @param pool The address of a previously-initialized pool to be destroyed.
 */
void pool_destroy(pool_t *pool);

/*! \fn void *pool_malloc(pool_t *pool, buffersize_t size, int32_t tag)
 * Allocates a buffer of the requested size with the given tag.
 *
 * \param[in] pool  The pool to allocate the memory from. If NULL, memory
 *  will be allocated from the global memory pool.
 * \param[in] size  The requested size of the buffer.
 * \param[in] tag   An identifying tag for the buffer. Cannot be zero.
 * \returns A new buffer of the requested size, or NULL if one cannot be
 *  allocated. In case of an error, a message will be written to stderr.
 */

#if !NDEBUG
#define pool_malloc(POOL, SIZE, TAG) pool_malloc_debug((POOL), (SIZE), (TAG), __FILE__, __FUNCTION__, __LINE__)
void *pool_malloc_debug(pool_t *pool, buffersize_t size, int32_t tag, const char *file, const char *function, int32_t line);
#else // !NDEBUG
void *pool_malloc(pool_t *pool, buffersize_t size, int32_t tag);
#endif

/*!
 * Reallocates memory for the given pointer with the new buffer size.
 * Returns NULL on failure. If mem_realloc fails, the original buffer
 * is not freed.
 *
 * Unlike normal realloc, this will NOT allocate new memory if you
 * pass NULL as the original pointer. com_realloc will do this given a
 * pool allocator, however.
 */
void *pool_realloc(void *p, buffersize_t size);

/*!
 * Frees a buffer previously allocated by ::pool_malloc.
 *
 * \param[in] buffer The buffer to be freed.
 */
void pool_free(void *buffer);

/*!
 * Gets the block header for a given buffer. This must be a block allocated
 * through ::pool_malloc. Attempting to use this with any other pointer is
 * liable to cause explosions.
 *
 * \remark This method will check for the block's memory guard. If the memory
 *  guard is corrupted, this function will return NULL, as it is no longer
 *  safe to interact with the block. Assume that something has gone
 *  horribly wrong if this happens.
 *
 * \param[in] buffer  The buffer to get the block header for.
 * \returns The address of the buffer's block header, or NULL if the block is
 *  suspected to be corrupt.
 */
const block_head_t *pool_block_for_pointer(const void *buffer);


//! Gets an allocator for the given memory pool.
allocator_t pool_allocator(pool_t *pool);


#if defined(__cplusplus)
}
#endif

#include <inline.end>

#endif /* end of include guard: __SNOW__MEMORY_POOL_H__ */
