#ifndef __SNOW_BUFFER_H__
#define __SNOW_BUFFER_H__ 1

#include <snow-config.h>
#include <memory/allocator.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct s_buffer buffer_t;

struct s_buffer {
  allocator_t *alloc;
  bool fixed;
  size_t size;
  size_t capacity;
  char *start;
  char *offset;

  void *opaque;
};

buffer_t *buffer_init(buffer_t *buffer, size_t capacity, allocator_t *alloc);
buffer_t *buffer_init_with_pointer(buffer_t *buffer, size_t size, void *p, allocator_t *alloc);
void buffer_destroy(buffer_t *buffer);

int buffer_eof(buffer_t *buf);

size_t buffer_write(buffer_t *buf, const void *p, size_t len);
size_t buffer_read(buffer_t *buf, void *p, size_t len);

size_t buffer_size(const buffer_t *buf);
size_t buffer_capacity(const buffer_t *buf);

int buffer_resize(buffer_t *buf, size_t size);
int buffer_reserve(buffer_t *buf, size_t capacity);

off_t buffer_tell(const buffer_t *buf);

// whence = SEEK_CUR, SEEK_END, or SEEK_SET
// Returns 0 on success, otherwise it sets errno and returns -1.
// errno = EINVAL if whence is not one of the above constants.
//         EOVERFLOW if the seek destination would result in a position past
//           the bounds of the buffer.
//         EBADF if the buffer is NULL.
int buffer_seek(buffer_t *buf, off_t pos, int whence);
int buffer_rewind(buffer_t *buf);

void *buffer_pointer(buffer_t *buf);
void *buffer_offset_pointer(buffer_t *buf);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* end __SNOW_BUFFER_H__ include guard */
