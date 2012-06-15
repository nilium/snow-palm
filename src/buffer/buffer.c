#include "buffer.h"
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

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
    buffer->context.mapped_file = 0;

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
    buffer->context.mapped_file = 0;
  } else if (! p) {
    s_log_error("Cannot initialize a buffer with a NULL pointer.");
  } else if (! buffer) {
    s_log_error("Attempt to initialize a NULL buffer.");
  }

  return buffer;
}

buffer_t *buffer_init_with_file(buffer_t *buffer, const char *file)
{
  #define FILE_MODE (O_RDWR|O_EXCL)

  int fildes = -1;
  void *mapped_ptr = NULL;
  off_t end = 0;

  if (buffer && file) {
    fildes = open(file, FILE_MODE);
    if (fildes < 0) {
      s_log_error("Error opening file %s: %d.", file, errno);
      return NULL;
    }

    // get size of file..
    end = lseek(fildes, 0, SEEK_END);
    if (end == -1 || lseek(fildes, 0, SEEK_SET) == -1) {
      s_log_error("Error getting size of file %s: %d.", file, errno);
      goto buffer_file_error;
    } else if (end == 0) {
      s_log_error("File %s is empty, closing buffer.", file);
      goto buffer_file_error;
    }

    mapped_ptr = mmap(NULL, (size_t)end, PROT_READ|PROT_WRITE, MAP_FILE, fildes, 0);
    if (mapped_ptr == MAP_FAILED) {
      s_log_error("Error mapping buffer to file descriptor for file %s: %d.",
        file, errno);
      mapped_ptr = NULL;
      goto buffer_file_error;
    }

    buffer->alloc = NULL;
    buffer->ptr = mapped_ptr;
    buffer->capacity = buffer->size = (size_t)end;
    buffer->outside = true;
    buffer->context.mapped_file = fildes;
  } else if (! file) {
    s_log_error("Cannot initialize a buffer with a NULL file path.");
  } else if (! buffer) {
    s_log_error("Attempt to initialize a NULL buffer.");
  }

  return buffer;

buffer_file_error:
  if (mapped_ptr && munmap(mapped_ptr, (size_t)end))
    s_fatal_error(1, "Error unmapping pointer for file %s: %d.", file, errno);
  else if (fildes > -1 && close(fildes))
    s_fatal_error(1, "Error closing buffer-mapped file %s: %d.", file, errno);

  return NULL;

  #undef FILE_MODE
}

int buffer_destroy(buffer_t *buffer)
{
  int fildes;

  if (!buffer) {
    errno = EBADF;
    s_log_error("Attempt to destroy a NULL buffer.");
    return -1;
  }

  fildes = buffer->context.mapped_file;

  if (buffer->ptr && ! buffer->outside) {
    com_free(buffer->alloc, buffer->ptr);
  } else if (buffer->ptr && fildes > -1) {
    if (munmap(buffer->ptr, buffer->size)) {
      s_log_error("Error unmapping file buffer (will not destroy buffer): %d.", errno);
      return -1;
    }

    if (close(fildes))
      s_log_error("Error closing file (continuing to destroy buffer): %d.", errno);
  }

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
