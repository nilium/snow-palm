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

/*! Array object type */
typedef struct s_array array_t;

struct s_array
{
	const class_t *isa;

	/*! The array's buffer */
	char *buf;
	/*! Size of each element in the array */
	size_t obj_size;
	/*! The capacity of the array (how many elements it can hold before the
		it has to reallocate its buffer). */
	size_t capacity;
	/*! The size (or length) of the array - this is how many elements the array
		currently holds. */
	size_t size;
	/*! The memory pool associated with the array. */
	memory_pool_t *pool;
};

extern const class_t _array_class;
#define array_class (&_array_class)

array_t *array_init(array_t *self, size_t objectSize, size_t size, size_t capacity);
array_t *array_copy(array_t *other);

bool array_resize(array_t *self, size_t size);
bool array_reserve(array_t *self, size_t capacity);

size_t array_size(const array_t *self);
size_t array_capacity(const array_t *self);

void array_sort(array_t *self, int (*comparator)(const void *left, const void *right));

void *array_atIndex(array_t *self, size_t index);

bool array_push(array_t *self, const void *value);
void array_pop(array_t *self, void *result);

void *array_buffer(array_t *self, size_t *byteLength);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* end of include guard: DYNARRAY_H */
