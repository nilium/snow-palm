/*
 * memory.h
 *
 * Created by Noel R. Cower on 16-09-2011.
 * Copyright (c) 2011 Noel R. Cower.  All rights reserved.
 */

#ifndef MEMORY_H_SA7RAUP3
#define MEMORY_H_SA7RAUP3

#include "config.h"
#include "mutex.h"

/*!
	\file

	The API for creating, destroying, and allocating from memory pools.  Memory
	pools are essentially linked lists that represent chunks of memory in a
	larger chunk of memory.  The purpose of this is to ensure that, when
	necessary, memory that ought to be close together is.  There is no solid way
	to reduce fragmentation, but ::mem_alloc will always attempt to use the next
	free or most recently freed block of memory for any new allocations.
	
	\par Thread Safety
	The only operations wrapped in a lock are ::mem_destroy_pool,
	::mem_retain_pool, ::mem_release_pool, ::mem_alloc, and ::mem_free.  Others
	do not get that treatment.  If you need to lock the pool for another reason,
	the memory_pool_t::lock member is there, but it's advised that you don't try
	to ever fiddle with pool internals.
*/

#if defined(__cplusplus)
extern "C"
{
#endif

typedef size_t buffersize_t;
typedef ptrdiff_t bufferdiff_t;

typedef struct s_block_head block_head_t;
typedef struct s_memory_pool memory_pool_t;

/*!
 * Memory block header.  This is mainly for internal and debugging use.
 */
struct s_block_head
{
	/*! Whether the block is in use.  Zero if not in use, one if a header
	    block, otherwise a regular block. */
	int32_t used;
	/*! Identifying tag for the block.  Zero if unused. */
	int32_t tag;
	/*! Size of the memory block. Includes header, memory guard, and
	    alignment adjustment. */
	buffersize_t size;
	/*! Next block in the memory pool. */
	block_head_t *prev;
	/*! Previous block in the memory pool. */
	block_head_t *next;
	/*! Pointer back to the memory pool the block belongs to. */
	memory_pool_t *pool;
	
#if !NDEBUG

	struct
	{
		char *source_file;
		char *function;
		int32_t line;
		buffersize_t requested_size;
	} debug_info;

#endif
} ATTRIB_ALIGNED;

/*!
 * Memory pool structure.  Do not touch its members unless you want to break stuff.
 */
struct s_memory_pool
{
	/*! Size of the memory pool.  Includes adjustment for alignment. */
	buffersize_t size;
	/*! Reference count. */
	int32_t refs;
	/*! Counter for block allocation - can overflow. */
	int32_t sequence;
	/*! The memory used by the memory pool. */
	char *buffer;
	/*! The ID tag of the memory pool. */
	int32_t tag;
	/*! The next free block of memory. */
	block_head_t *nextUnused;
	/*! Header block - size is always 0, used is always 1, etc. */
	block_head_t head;
	/*! Pool lock */
	mutex_t lock;
};

extern const int32_t MAIN_POOL_TAG;

/*!
 * Initializes the global memory pool.
 */
void mem_init(void);

/*!
 * Destroys the global memory pool.  This will not destroy any other pools.
 */
void mem_shutdown(void);

/*!
 * Initializes a new memory pool.  Newly-initialized pools have a retain-count of 1.
 * 
 * \param[inout]	pool The address of an uninitialized pool to be initialized.
 * \param[in]		size The size of the memory pool to initialize.
 * \param[in]		tag An integer tag to identify the memory pool by.  Cannot be
 *	zero or the same as ::MAIN_POOL_TAG.  Recommendation: set up an enum for all
 *	pools and only use those values for tags.
 */
void mem_init_pool(memory_pool_t *pool, buffersize_t size, int32_t tag);

/*!
 * Destroys a memory pool.
 * @param pool The address of a previously-initialized pool to be destroyed.
 */
void mem_destroy_pool(memory_pool_t *pool);

/*!
 * Increases the retain count of a pool.  All retains must be matched by a call
 * to ::mem_release_pool.
 * #112933
 * \param[in] pool The pool to retain.
 * \returns The memory pool being retained.
 */
memory_pool_t *mem_retain_pool(memory_pool_t *pool);

/*!
 * Decreases the retain count of a pool.  If the retain count of the pool reaches
 * zero, it is destroyed via ::mem_destroy_pool.
 *
 * \param[in] pool The pool to release.
 */
void mem_release_pool(memory_pool_t *pool);

/*! \fn void *mem_alloc(memory_pool_t *pool, buffersize_t size, int32_t tag)
 * Allocates a buffer of the requested size with the given tag.
 *
 * \param[in]	pool	The pool to allocate the memory from.  If NULL, memory
 *	will be allocated from the global memory pool.
 * \param[in]	size	The requested size of the buffer.
 * \param[in]	tag		An identifying tag for the buffer.  Cannot be zero.
 * \returns	A new buffer of the requested size, or NULL if one cannot be
 *	allocated.  In case of an error, a message will be written to stderr.
 */

#if !NDEBUG
#define mem_alloc(POOL, SIZE, TAG) mem_alloc_debug((POOL), (SIZE), (TAG), __FILE__, __FUNCTION__, __LINE__)
void *mem_alloc_debug(memory_pool_t *pool, buffersize_t size, int32_t tag, const char *file, const char *function, int32_t line);
#else // !NDEBUG
void *mem_alloc(memory_pool_t *pool, buffersize_t size, int32_t tag);
#endif

/*!
 * Frees a buffer previously allocated by ::mem_alloc.
 *
 * \param[in] buffer The buffer to be freed.
 */
void mem_free(void *buffer);

/*!
 * Gets the block header for a given buffer.  This must be a block allocated
 * through ::mem_alloc.  Attempting to use this with any other pointer is liable
 * to cause explosions.
 *
 * \remark This method will check for the block's memory guard.  If the memory
 *	guard is corrupted, this function will return NULL, as it is no longer
 *	safe to interact with the block.  Assume that something has gone
 *	horribly wrong if this happens.
 *
 * \param[in]	buffer	The buffer to get the block header for.
 * \returns	The address of the buffer's block header, or NULL if the block is
 *	suspected to be corrupt.
 */
const block_head_t *mem_get_block(const void *buffer);


#if defined(__cplusplus)
}
#endif

#endif /* end of include guard: MEMORY_H_SA7RAUP3 */
