#ifndef __SNOW__BUFFER_H__
#define __SNOW__BUFFER_H__ 1

#include <snow-config.h>
#include <memory/allocator.h>

#ifdef __SNOW__BUFFER_C__
#define S_INLINE
#else
#define S_INLINE inline
#endif

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct s_buffer buffer_t;

struct s_buffer {
  allocator_t *alloc;
  char *ptr;
  size_t size;
  size_t capacity;
  bool outside;
};

buffer_t *buffer_init(buffer_t *buffer, size_t capacity, allocator_t *alloc);
buffer_t *buffer_init_with_pointer(buffer_t *buffer, size_t size, void *p, allocator_t *alloc);
int buffer_destroy(buffer_t *buffer);

size_t buffer_size(const buffer_t *buf);
size_t buffer_capacity(const buffer_t *buf);

int buffer_resize(buffer_t *buf, size_t size);
int buffer_reserve(buffer_t *buf, size_t capacity);

const void *buffer_pointer_const(const buffer_t *buf);
void *buffer_pointer(buffer_t *buf);

#ifdef __cplusplus
}
#endif // __cplusplus

#include <inline.end>

#endif /* end __SNOW__BUFFER_H__ include guard */
