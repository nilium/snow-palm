#define __SNOW__BUFFER_C__

#include "buffer.h"
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

//////// BUFFER

buffer_t *buffer_init(buffer_t *buffer, size_t size, allocator_t *alloc)
{
  if (buffer) {
    if (alloc == NULL)
      alloc = g_default_allocator;

    buffer->alloc = alloc;
    buffer->ptr = NULL;
    buffer->size = 0;
    buffer->capacity = 0;
    buffer->outside = false;

    buffer_resize(buffer, size);
  } else if (! buffer) {
    s_log_error("Attempt to initialize a NULL buffer.");
  }

  return buffer;
}

buffer_t *buffer_init_with_pointer(buffer_t *buffer, size_t size, void *p, allocator_t *alloc)
{
  if (buffer && p) {
    if (alloc == NULL)
      alloc = g_default_allocator;

    buffer->alloc = alloc;
    buffer->ptr = (char *)p;
    buffer->size = size;
    buffer->capacity = size;
    buffer->outside = true;
  } else if (! p) {
    s_log_error("Cannot initialize a buffer with a NULL pointer.");
  } else if (! buffer) {
    s_log_error("Attempt to initialize a NULL buffer.");
  }

  return buffer;
}

int buffer_destroy(buffer_t *buffer)
{
  if (!buffer) {
    errno = EBADF;
    s_log_error("Attempt to destroy a NULL buffer.");
    return -1;
  }

  if (buffer->ptr && ! buffer->outside)
    com_free(buffer->alloc, buffer->ptr);

  memset(buffer, 0, sizeof(*buffer));

  return 0;
}

size_t buffer_size(const buffer_t *buf)
{
  if (! buf) {
    s_log_error("Attempt to get the size of a NULL buffer.");
    errno = EBADF;
    return 0;
  }

  return buf->size;
}

size_t buffer_capacity(const buffer_t *buf)
{
  if (! buf) {
    s_log_error("Attempt to get the capacity of a NULL buffer.");
    errno = EBADF;
    return 0;
  }

  return buf->capacity;
}

int buffer_resize(buffer_t *buf, size_t size)
{
  if (! buf) {
    s_log_error("Attempt to resize a NULL buffer.");
    errno = EBADF;
    return -1;
  }

  if (buf->outside) {
    s_log_error("Attempt to resize a fixed-size buffer.");
    errno = EINVAL;
    return -1;
  }

  if (size > buf->size && buffer_reserve(buf, size) == -1)
    return -1;

  buf->size = size;

  return 0;
}

int buffer_reserve(buffer_t *buf, size_t capacity)
{
  size_t new_capacity;
  void *new_buf;

  if (!buf) {
    s_log_error("Attempt to reserve memory for a NULL buffer.");
    errno = EBADF;
    return -1;
  } else if (capacity <= buf->capacity) {
    return 0;
  } else if (buf->outside) {
    s_log_error("Attempt to reserve memory for a fixed-size buffer.");
    errno = EINVAL;
    return -1;
  }

  new_capacity = buf->capacity * 2;
  if (new_capacity < capacity)
    new_capacity = capacity;

  new_buf = com_realloc(buf->alloc, buf->ptr, new_capacity);
  if (! new_buf) {
    s_log_error("Failed to reallocate memory for a buffer.");
    errno = ENOMEM;
    return -1;
  }

  buf->ptr = new_buf;
  buf->capacity = new_capacity;

  return 0;
}

const void *buffer_pointer_const(const buffer_t *buf) {
  if (! buf) {
    s_log_error("Attempt to get the raw pointer of a NULL buffer.");
    errno = EBADF;
    return NULL;
  }

  return buf->ptr;
}

void *buffer_pointer(buffer_t *buf)
{
  if (! buf) {
    s_log_error("Attempt to get the raw pointer of a NULL buffer.");
    errno = EBADF;
    return NULL;
  }

  return buf->ptr;
}

#ifdef __cplusplus
}
#endif // __cplusplus
