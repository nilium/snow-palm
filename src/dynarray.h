/*
	Dynamic array class
	Written by Noel Cower

	See LICENSE.md for license information
*/

#ifndef DYNARRAY_H

#define DYNARRAY_H

#include "config.h"
#include "memory.h"
#include "object.h"

#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

typedef struct s_array array_t;

struct s_array
{
	const class_t *isa;

	char *buf;
	size_t obj_size;
	size_t capacity;
	size_t size;
	memory_pool_t *pool;
};

array_t *array_new(size_t objectSize, size_t size, size_t capacity, memory_pool_t *pool);
array_t *array_copy(array_t *other);

bool array_resize(array_t *array, size_t size);
bool array_reserve(array_t *array, size_t capacity);

size_t array_size(const array_t *array);
size_t array_capacity(const array_t *array);

bool array_push(array_t *array, const void *value);
void array_pop(array_t *array, void *result);

void *array_buffer(array_t *array, size_t *byteLength);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* end of include guard: DYNARRAY_H */
