#include "buffer.h"
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

buffer_t *buffer_init(buffer_t *buffer, size_t capacity, allocator_t *alloc)
{
  if (alloc == NULL)
    alloc = g_default_allocator;

  buffer->alloc = alloc;
  buffer->size = 0;
  buffer->capacity = 0;
  buffer->fixed = false;
  buffer->start = buffer->offset = NULL;
  buffer_reserve(buffer, capacity);

  return buffer;
}

buffer_t *buffer_init_with_pointer(buffer_t *buffer, size_t size, void *p, allocator_t *alloc)
{
  if (buffer) {
    if (alloc == NULL)
      alloc = g_default_allocator;

    memset(buffer, 0, sizeof(*buffer));
    buffer->alloc = alloc;
    buffer->size = size;
    buffer->capacity = size;
    buffer->fixed = true;
    buffer->start = buffer->offset = (char *)p;
  }

  return buffer;
}

void buffer_destroy(buffer_t *buffer)
{
  if (buffer->start && ! buffer->fixed)
    com_free(buffer->alloc, buffer->start);
  memset(buffer, 0, sizeof(*buffer));
}

int buffer_eof(buffer_t *buf)
{
  if (! buf) {
    errno = EBADF;
    return -1;
  }

  return (buf->offset == (buf->start + buf->size));
}

size_t buffer_write(buffer_t *buf, const void *p, size_t len)
{
  size_t abs_size;

  if (! buf) {
    errno = EBADF;
    return 0;
  }

  if (! p) {
    errno = EINVAL;
    return 0;
  }

  if (len == 0)
    return 0;

  abs_size = (size_t)buffer_tell(buf) + len;
  if (abs_size > buf->size && buffer_resize(buf, abs_size) == -1)
    return -1;

  memcpy(buf->offset, p, len);
  buf->offset += len;

  return len;
}

size_t buffer_read(buffer_t *buf, void *p, size_t len)
{
  if (! buf) {
    errno = EBADF;
    return 0;
  }

  if (! p)
    return buffer_seek(buf, len, SEEK_CUR);

  if (buf->offset + len >= buf->start + buf->size)
    len = ((buf->start + buf->size) - buf->offset);

  if (len > 0) {
    memcpy(p, buf->offset, len);
    buf->offset += len;
  }

  return len;
}

size_t buffer_size(const buffer_t *buf)
{
  if (! buf) {
    errno = EBADF;
    return 0;
  }

  return buf->size;
}

size_t buffer_capacity(const buffer_t *buf)
{
  if (! buf) {
    errno = EBADF;
    return 0;
  }

  return buf->capacity;
}

int buffer_resize(buffer_t *buf, size_t size)
{
  if (! buf) {
    errno = EBADF;
    return -1;
  }

  if (size > buf->size && buffer_reserve(buf, size) == -1)
    return -1;

  buf->size = size;
  off_t offset = buffer_tell(buf);
  if (offset == -1)
    return -1;
  else if (offset > size)
    buf->offset = buf->start + size;

  return 0;
}

int buffer_reserve(buffer_t *buf, size_t capacity)
{
  size_t new_capacity;
  ptrdiff_t off;
  void *new_buf;

  if (!buf) {
    errno = EBADF;
    return -1;
  }

  if (capacity <= buf->capacity)
    return 0;

  if (buf->fixed) {
    errno = EINVAL;
    return -1;
  }

  new_capacity = buf->capacity * 2;
  if (new_capacity < capacity)
    new_capacity = capacity;

  new_buf = com_realloc(buf->alloc, buf->start, new_capacity);
  if (! new_buf) {
    errno = ENOMEM;
    return -1;
  }

  off = buf->offset - buf->start;

  buf->start = new_buf;
  buf->offset = new_buf + off;
  buf->capacity = new_capacity;

  return 0;
}

off_t buffer_tell(const buffer_t *buf)
{
  if (! buf) {
    errno = EBADF;
    return -1;
  }

  return (size_t)(buf->offset - buf->start);
}

int buffer_seek(buffer_t *buf, off_t pos, int whence)
{
  ptrdiff_t abs_pos;
  char *new_offset;

  if (! buf) {
    errno = EBADF;
    return -1;
  }

  switch (whence) {
    case SEEK_SET:
      new_offset = buf->start + pos;
      break;

    case SEEK_CUR:
      new_offset = buf->offset + pos;
      break;

    case SEEK_END:
      new_offset = (buf->start + buf->size) + pos;
      break;

    default:
      errno = EINVAL;
      return -1;
  }

  abs_pos = new_offset - buf->start;
  if (buf->start == NULL || buf->size <= abs_pos || abs_pos < 0) {
    errno = EOVERFLOW;
    return -1;
  }

  return 0;
}

int buffer_rewind(buffer_t *buf)
{
  if (! buf) {
    errno = EBADF;
    return -1;
  }
  buf->offset = buf->start;
}

void *buffer_pointer(buffer_t *buf)
{
  if (! buf) {
    errno = EBADF;
    return NULL;
  }

  return buf->start;
}

void *buffer_offset_pointer(buffer_t *buf)
{
  if (! buf) {
    errno = EBADF;
    return NULL;
  }

  return buf->offset;
}


#ifdef __cplusplus
}
#endif // __cplusplus
