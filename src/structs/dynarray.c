/*
  Dynamic array object
  Written by Noel Cower

  See LICENSE.md for license information
*/

#include "dynarray.h"

#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

void array_destroy(array_t *self)
{
  if (self == NULL) {
    s_fatal_error(1, "Cannot destroy NULL array.");
    return;
  }

  allocator_t *alloc = self->allocator;

  if (self->buf != NULL)
    com_free(alloc, self->buf);

  memset(self, 0, sizeof(*self));
}

array_t *array_init(array_t *self, size_t object_size, size_t capacity, allocator_t *alloc)
{
  if (alloc == NULL)
    alloc = g_default_allocator;

  if (object_size == 0) {
    s_fatal_error(1, "Invalid object size for array: 0.");
    return NULL;
  } else if (!(alloc->malloc && alloc->free)) {
    s_fatal_error(1, "malloc & free pointers are NULL.");
    return NULL;
  } else if (NULL == self) {
    s_fatal_error(1, "Failed to allocate array.");
    return NULL;
  }

  self->allocator = alloc;
  self->obj_size = object_size;
  self->size = 0;
  self->capacity = 0;
  self->buf = NULL;

  if (!array_reserve(self, capacity)) {
    array_destroy(self);
    s_fatal_error(1, "Failed to create array with capacity %zu", capacity);
    return NULL;
  }

  return self;
}

bool array_copy(const array_t *src, array_t *dst)
{
  if (src == NULL || dst == NULL) {
    s_fatal_error(1, "Cannot copy NULL array.");
    return false;
  }

  array_t copy = *dst;
  const size_t src_buffer_size = (src->size * src->obj_size);
  const size_t dst_buffer_size = (copy.capacity * copy.obj_size);

  if (copy.buf) {
    memset(copy.buf, 0, dst_buffer_size);

    if (dst_buffer_size < src_buffer_size) {
      // release the buffer since it can't hold the source buffer
      dst->buf = NULL;
      com_free(copy.allocator, copy.buf);
      copy.buf = NULL;
      copy.capacity = 0;
    } else {
      // reuse the buffer
      copy.capacity = dst_buffer_size / src->obj_size;
    }
  }

  copy.size = 0;
  copy.obj_size = src->obj_size;

  if (src->buf) {
    if (!array_resize(&copy, src->size)) {
      s_fatal_error(1, "Failed to copy array.");
      return false;
    }

    if (src->size > 0 && copy.size == src->size)
      memcpy(copy.buf, src->buf, src_buffer_size);
  }

  *dst = copy;

  return true;
}

bool array_resize(array_t *self, size_t size)
{
  if (self == NULL) {
    s_fatal_error(1, "Cannot resize NULL array.");
    return false;
  }

  if (array_reserve(self, size)) {
    if (self->buf) {
      if (size == 0 && self->size != 0) {
        // simply zero the used portion of the array
        memset(self->buf, 0, self->size * self->obj_size);
      } else if (size < self->size) {
        // zero the portion of the array cut off
        memset(self->buf + (size * self->obj_size), 0, (self->size - size) * self->obj_size);
      }
    }
    self->size = size;

    return true;
  }

  s_fatal_error(1, "Failed to resize array.");

  return false;
}

bool array_reserve(array_t *self, size_t capacity)
{
  size_t new_size, new_cap, orig_size;
  char *new_buf = NULL;
  bool tried_min = false;

  if (capacity <= self->capacity || capacity == 0) return true;

  new_cap = self->capacity * 2;
  if (new_cap < capacity) {
    new_cap = capacity; /* minimum requested is larger, use it */
    tried_min = true;
  }

  orig_size = self->size * self->obj_size;

reserve_capacity:
  new_size = new_cap * self->obj_size;

  new_buf = (char *)com_malloc(self->allocator, new_size);
  if (NULL == new_buf) {
    /* in the event that the new buffer can't be allocated, try one more route
       before giving up
    */
    if (!tried_min) {
      /* if the minimum capacity requested hasn't been tried yet, try it */
      tried_min = true;
      new_cap = capacity;
      goto reserve_capacity;
    }

    s_fatal_error(1, "Failed to reserve %zu elements for array.", new_cap);

    return false;
  }

  if (self->buf && orig_size)
    memcpy(new_buf, self->buf, orig_size);

  memset(new_buf + orig_size, 0, new_size - orig_size);
  if (self->buf) com_free(self->allocator, self->buf);
  self->buf = new_buf;
  self->capacity = new_cap;

  return true;
}

bool array_clear(array_t *self)
{
  return array_resize(self, 0);
}

size_t array_size(const array_t *self)
{
  if (self == NULL) {
    s_fatal_error(1, "Cannot access NULL array.");
    return 0;
  }
  return self->size;
}

size_t array_capacity(const array_t *self)
{
  if (self == NULL) {
    s_fatal_error(1, "Cannot access NULL array.");
    return 0;
  }
  return self->capacity;
}

bool array_empty(const array_t *self)
{
  return !array_size(self);
}

bool array_sort(array_t *self, int (*comparator)(const void *left, const void *right))
{
  if (self == NULL) {
    s_fatal_error(1, "Cannot sort NULL array.");
    return false;
  }

  if (self->size < 2)
    return true;

  qsort(self->buf, self->size*self->obj_size, self->obj_size, comparator);

  return true;
}

bool array_get(array_t *self, size_t index, void *dst) {
  const void *src = array_at_index(self, index);

  if (src == NULL) {
    s_fatal_error(1, "Index %zu out of range [0..%zu]", index, (self->size - 1));
    return false;
  } else if (dst == NULL) {
    s_fatal_error(1, "Attempt to write to NULL pointer.");
    return false;
  }

  memcpy(dst, src, self->obj_size);

  return true;
}

bool array_store(array_t *self, size_t index, const void *src) {
  void *dst = array_at_index(self, index);
  if (dst == NULL) {
    s_fatal_error(1, "Index %zu out of range [0..%zu]", index, (self->size - 1));
    return false;
  }

  if (src == NULL) {
    memset(dst, 0, self->obj_size);
  } else {
    memcpy(dst, src, self->obj_size);
  }

  return true;
}

void *array_at_index(array_t *self, size_t index)
{
  if (!self) {
    s_fatal_error(1, "Cannot access NULL array.");
    return NULL;
  }

  if (self->size <= index) {
    s_fatal_error(1, "Index %zu out of bounds [0..%zu]", index, (self->size - 1));
    return NULL;
  }

  return (self->buf + (index * self->obj_size));
}

bool array_push(array_t *self, const void *value)
{
  if (self == NULL) {
    s_fatal_error(1, "Cannot push to NULL array.");
    return false;
  } else if (value == NULL) {
    size_t new_size = self->size + 1;
    if (array_resize(self, new_size))
      return true;

    s_fatal_error(1, "Failed to pushed value into array.");
    return false;
  }

  if (array_reserve(self, self->size + 1)) {
    memcpy(self->buf + (self->size * self->obj_size), value, self->obj_size);
    self->size += 1;
    return true;
  } else {
    s_fatal_error(1, "Failed to reserve space for array_push.");
  }

  return false;
}

bool array_pop(array_t *self, void *result)
{
  char *addr;

  if (!self) {
    s_fatal_error(1, "Cannot access NULL array.");
    return NULL;
  }

  if (self->size == 0) {
    s_fatal_error(1, "Array underflow: attempt to pop from empty array.");
    return false;
  }

  addr = self->buf + ((self->size - 1) * self->obj_size);

  if (result)
    memcpy(result, addr, self->obj_size);

  memset(addr, 0, self->obj_size);
  self->size -= 1;

  return true;
}

void *array_buffer(array_t *self, size_t *byte_length)
{
  if (!self) {
    s_fatal_error(1, "Cannot access NULL array.");
    return NULL;
  }

  if (self->buf == NULL)
    return NULL;

  if (byte_length)
    *byte_length = self->size * self->obj_size;
  return self->buf;
}

void array_each(array_t *self, iterator_fn_t iter, void *context)
{
  if (!self) {
    s_fatal_error(1, "Cannot access NULL array.");
    return;
  } else if (!iter) {
    s_fatal_error(1, "Iterator function is NULL.");
    return;
  }

  char *buf;
  size_t index;
  const size_t end = self->size;
  const size_t obj_size = self->obj_size;
  bool stop = false;
  for (buf = self->buf, index = 0; !stop && index < end; ++index, buf += obj_size)
    iter(buf, index, context, &stop);
}

void *array_last(array_t *self)
{
  if (!self) {
    s_fatal_error(1, "Cannot access NULL array.");
    return NULL;
  }

  if (self->buf == NULL)
    return NULL;

  return (self->buf + (self->size - 1));
}

#if defined(__cplusplus)
}
#endif /* __cplusplus */
