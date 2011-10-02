/*
 * memory.c
 *
 * Created by Noel R. Cower on 16-09-2011.
 * Copyright (c) 2011 Noel R. Cower.  All rights reserved.
 */


#include "memory.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if defined(__cplusplus)
extern "C"
{
#endif

/*!
 * The minimum size of a memory pool.  This number is kind of arbitrary, but by
 * default the minimum is the size of four minimum-size blocks.
 */
#define MIN_POOL_SIZE (MIN_BLOCK_SIZE * 4)
/*! Default memory pool size for main pools. */
#define DEFAULT_POOL_SIZE (8/*mb*/ * 1024/*kb*/ * 1024/*b*/)
/*! Alignment for memory blocks.  Must be a power of two minus one. */
#define BLOCK_ALIGNMENT (15)
/*! Macro for quickly getting the actual size of a block given a requested size. */
#define BLOCK_SIZE(SZ) ((buffersize_t)((SZ) + sizeof(block_head_t) + (BLOCK_ALIGNMENT) & ~(BLOCK_ALIGNMENT) + sizeof(uint32_t)))
/*! Minimum block size of one byte. */
#define MIN_BLOCK_SIZE BLOCK_SIZE(32)
/*! Memory guard value - used to determine if something has written outside the
	bounds of a block. */
#define MEMORY_GUARD (0xBEEFC0DE)

#if !defined(MAIN_POOL_SIZE)
#define MAIN_POOL_SIZE DEFAULT_POOL_SIZE
#endif

/*! Main memory pool's tag. */
const int32_t MAIN_POOL_TAG = 0x8FFFFFFF; /*"net.spifftastic.snow.mainMemoryPool";*/

/*! Unused memory block tag. */
/*static const char *DEFAULT_BLOCK_TAG_UNUSED = "net.spifftastic.snow.unused";*/

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
 * Destroys a memory pool.
 * @param pool The address of a previously-initialized pool to be destroyed.
 */
static void mem_destroy_pool(memory_pool_t *pool);


void mem_init(void)
{
	if (g_main_pool.head.used) return;
	
	mem_init_pool(&g_main_pool, MAIN_POOL_SIZE, MAIN_POOL_TAG);
}


void mem_shutdown(void)
{
	if (!g_main_pool.head.used) return;
	
	mem_destroy_pool(&g_main_pool);
}


void mem_init_pool(memory_pool_t *pool, buffersize_t size, int32_t tag)
{
#if THREADED && defined(__cplusplus)
	pool->lock.lock();
#endif
	
	if (!pool->buffer) {
		if (size < MIN_POOL_SIZE) {
			size = MIN_POOL_SIZE;
		}
		log_note("[%X] Initializing memory pool (%p) with size %zu\n", tag, (const void *)pool, size);

		pool->buffer = (char *)malloc(MAIN_POOL_SIZE + (BLOCK_ALIGNMENT));
		pool->size = MAIN_POOL_SIZE + (BLOCK_ALIGNMENT);

		block_head_t *block = (block_head_t *)(((unsigned long)pool->buffer + (BLOCK_ALIGNMENT)) & ~(BLOCK_ALIGNMENT));
		block->size = MAIN_POOL_SIZE;
		block->used = 0;
		/*block->tag = DEFAULT_BLOCK_TAG_UNUSED;*/
		block->next = &pool->head;
		block->prev = &pool->head;
		block->pool = pool;

		pool->head.used = 1;
		pool->head.size = 0;
		pool->head.next = block;
		pool->head.prev = block;
		pool->tag = tag;
		pool->head.pool = pool;
		pool->nextUnused = block;
		pool->sequence = 1;
		pool->refs = 1;
	} else {
		log_error("[%X] Attempt to initialize already-initialized memory pool (%p) with new tag %X\n", pool->tag, (const void *)pool, tag);
	}
	
#if THREADED && defined(__cplusplus)
	pool->lock.unlock();
#endif
}


static void mem_destroy_pool(memory_pool_t *pool)
{
#if THREADED && defined(__cplusplus)
	pool->lock.lock();
#endif
	
	if (pool->head.used) {
		int32_t tag = pool->tag;
		mem_check_pool(pool);
	
		free(pool->buffer);
		pool->buffer = NULL;
		pool->head.next = NULL;
		pool->head.prev = NULL;
		pool->nextUnused = NULL;
		pool->head.used = 0;
		pool->sequence = 0;
		pool->refs = 0;
	
		log_note("[%X] Destroyed pool (%p)\n", tag, (const void *)pool);
	} else {
		log_error("Attempt to destroy already-destroyed memory pool (%p)\n", (const void *)pool);
	}

#if THREADED && defined(__cplusplus)
	pool->lock.unlock();
#endif
}


memory_pool_t *mem_retain_pool(memory_pool_t *pool)
{
#if THREADED && defined(__cplusplus)
	pool->lock.lock();
#endif
	++pool->refs;
#if THREADED && defined(__cplusplus)
	pool->lock.unlock();
#endif
	return pool;
}


void mem_release_pool(memory_pool_t *pool)
{
	if (!pool) return;
#if THREADED && defined(__cplusplus)
	pool->lock.lock();
#endif
	int32_t refs = --pool->refs;
#if THREADED && defined(__cplusplus)
	pool->lock.unlock();
#endif
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
	
	if (tag == 0) {
		log_error("Allocation failed - invalid tag %X\n", tag);
	}
	
	if (!pool->head.used) {
		log_error("Allocation failed - [%X] pool is not initialized or corrupt\n", pool->tag);
		return NULL;
	}
	
	buffersize_t blockSize = BLOCK_SIZE(size);
	if (blockSize < MIN_BLOCK_SIZE) {
		blockSize = MIN_BLOCK_SIZE;
	}
	
	if (blockSize > pool->size) {
		log_error("Allocation failed - [%X] requested size %zu exceeds pool capacity (%zu)\n", pool->tag, size, pool->size);
		return NULL;
	}
	
#if THREADED && defined(__cplusplus)
	pool->lock.lock();
#endif
	
	block_head_t *block = pool->nextUnused;
	block_head_t *const terminator = pool->nextUnused->prev;
	for (; block != terminator; block = block->next) {
		/* skip used blocks and blocks that're too small */
		if (block->used) continue;
		
		if (block->size < blockSize) continue;
		
		/* if the free block is large enough to be split into two blocks, do that */
		if ((block->size - blockSize) > MIN_BLOCK_SIZE) {
			block_head_t *unused = (block_head_t *)((char *)block + blockSize);
			
			unused->size = block->size - blockSize;
			block->size = blockSize;
			
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
		((uint32_t *)((char *)block + blockSize))[-1] = MEMORY_GUARD;
		pool->nextUnused = block->next;
		
#if !NDEBUG
		size_t fileLength = strlen(file) + 1;
		size_t fnLength = strlen(function) + 1; /* account for \0 in both cases */
		/* to avoid pointless fragmentation, we'll allocate this using malloc normally */
		char *fileCopy = (char *)malloc((fileLength + fnLength) * sizeof(char));
		strncpy(fileCopy, file, fileLength + 1);
		block->debug_info.source_file = fileCopy;
		
		/* copy function as well */
		char *fnCopy = fileCopy + fileLength;
		strncpy(fnCopy, function, fnLength);
		block->debug_info.function = fnCopy;
		
		block->debug_info.line = line;
		block->debug_info.requested_size = size;
#endif /* !NDEBUG */
		
		/*log_note("returning block for sz %zu:\n", size);*/
		/*dbg_print_block(block);*/
		
#if THREADED && defined(__cplusplus)
		pool->lock.unlock();
#endif
		
		return block + 1;
	}
	
#if THREADED && defined(__cplusplus)
	pool->lock.unlock();
#endif
	
	log_error("Failed to allocate %zu bytes - [%X] pool is out of memory\n", size, pool->tag);
	
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
		return;
	}
	
#if THREADED && defined(__cplusplus)
	pool->lock.lock();
#endif
	
	if (block == &block->pool->head) {
		log_error("[%X] Free on header block of pool\n", block->pool->tag);
		return;
	}
	
	if (block->size < MIN_BLOCK_SIZE) {
		log_error("[%X] Invalid block, too small (%zu) - may be corrupted\n", block->pool->tag, block->size);
		return;
	}
	
	uint32_t guard = ((uint32_t *)((char *)block + block->size))[-1];
	if (guard != MEMORY_GUARD) {
		log_error("[%X] Block memory guard corrupted - reads %X\n", block->pool->tag, guard);
	}
	
	if (!block->used) {
		log_error("[%X] Double-free on block\n", block->pool->tag);
		return;
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
	
	pool->nextUnused = block;
	
#if THREADED && defined(__cplusplus)
	pool->lock.unlock();
#endif
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
	fprintf(stderr, "  source file: %s [%zu]\n", block->debug_info.source_file, block->debug_info.line);
	fprintf(stderr, "  source function: %s\n", block->debug_info.function);
	fprintf(stderr, "  buffer size: %zu bytes\n", block->debug_info.requested_size);
#endif /* !NDEBUG */
	fprintf(stderr, "  pool tag: %X\n}\n", block->pool ? block->pool->tag : 0);
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
	
	if (block->used) {
		uint32_t guard = ((const uint32_t *)((const char *)block + block->size))[-1];
		if (guard != MEMORY_GUARD) {
			log_error("Memory guard corrupted\n");
		}
	}
	
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
	
	uint32_t guard = ((uint32_t *)((const char *)buffer + block->size))[-1];
	if (guard != MEMORY_GUARD) {
		log_error("Memory guard corrupted\n");
		dbg_print_block(block);
		return NULL;
	}
	
	return block;
}


#if defined(__cplusplus)
}
#endif
