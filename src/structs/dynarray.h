/*
  Dynamic array object
  Written by Noel Cower

  See LICENSE.md for license information
*/

#ifndef __SNOW__DYNARRAY_H__

#define __SNOW__DYNARRAY_H__

#include <snow-config.h>
#include <memory/allocator.h>

#ifdef __SNOW__DYNARRAY_C__
#define S_INLINE
#else
#define S_INLINE inline
#endif

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
  allocator_t *allocator;
};

/* Initialize an array with the given object size, capacity, and allocator.
   Object size specifies the size of each element in the array. */
array_t *array_init(array_t *self, size_t object_size, size_t capacity, allocator_t *alloc);

/* Destroys an array. */
void array_destroy(array_t *self);
/* Copies an array from src to dst. The original array pointed to by dst, if
   any, will be lost. */
bool array_copy(const array_t *src, array_t *dst);

/* Resizes an array. New elements are zeroed. */
bool array_resize(array_t *self, size_t size);
/* Attempts to reserve space for, at minimum, `capacity` number of elements. */
bool array_reserve(array_t *self, size_t capacity);
/* Removes all elements from an array and sets its size to zero. */
bool array_clear(array_t *self);

/* Returns the size of an array. */
size_t array_size(const array_t *self);
/* Returns the capacity of an array. */
size_t array_capacity(const array_t *self);
/* Returns whether the array an empty. */
bool array_empty(const array_t *self);

/* Sorts the values in the array using the comparator function. */
bool array_sort(array_t *self, int (*comparator)(const void *left, const void *right));

/* Copies the value at the specified index in the array to the dst pointer.
   If the dst pointer is NULL, it's a fatal error. */
bool array_get(array_t *self, size_t index, void *dst);
/* Copies the specified value to the array element at the specified index. If
   the value is NULL, it zeroes the contents of that element. */
bool array_store(array_t *self, size_t index, const void *src);
/* Returns a pointer to the element at the index specified. If the index is
   outside the array's bounds, it's a fatal error. */
void *array_at_index(array_t *self, size_t index);

/* Pushes a value onto the end of the array (copies from value). If NULL, will
   increase the size of the array and fill the new element with zeroes. */
bool array_push(array_t *self, const void *value);
/* Pops the value from the end of the array, reducing its size by 1. If the
   array is empty, this is a fatal error. */
bool array_pop(array_t *self, void *result);

/* Returns a pointer to the array data. */
void *array_buffer(array_t *self, size_t *byte_length);

/* Iterates over each element of the array and calls the iterator function for
   each element. */
void array_each(array_t *self, iterator_fn_t iter, void *context);

/* Returns a pointer to the last element of the array. If the array is empty,
   returns NULL. */
void *array_last(array_t *self);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#include <inline.end>

#endif /* end of include guard: __SNOW__DYNARRAY_H__ */
