/*
  Dynamic array object
  Written by Noel Cower

  See LICENSE.md for license information
*/

#ifndef DYNARRAY_H

#define DYNARRAY_H

#include <snow-config.h>
#include <memory/allocator.h>

#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

/*! Array object type */
typedef struct s_array array_t;
typedef void (*iterator_fn_t)(void *elem, size_t index, void *context, bool *stop);

struct s_array
{
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
  /*! The array's allocator */
  allocator_t allocator;
};

array_t *array_new(size_t object_size, size_t capacity, allocator_t alloc);

void array_destroy(array_t *self);
array_t *array_copy(const array_t *src);

bool array_resize(array_t *self, size_t size);
bool array_reserve(array_t *self, size_t capacity);

size_t array_size(const array_t *self);
size_t array_capacity(const array_t *self);

bool array_sort(array_t *self, int (*comparator)(const void *left, const void *right));

bool array_get(array_t *self, size_t index, void *dst);
bool array_store(array_t *self, size_t index, const void *src);
void *array_at_index(array_t *self, size_t index);

bool array_push(array_t *self, const void *value);
bool array_pop(array_t *self, void *result);

void *array_buffer(array_t *self, size_t *byte_length);

void array_each(array_t *self, iterator_fn_t iter, void *context);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* end of include guard: DYNARRAY_H */
