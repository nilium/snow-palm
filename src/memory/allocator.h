#ifndef __SNOW__ALLOCATOR_H__
#define __SNOW__ALLOCATOR_H__ 1

#include <snow-config.h>

#ifdef __SNOW__ALLOCATOR_C__
#define S_INLINE
#else
#define S_INLINE inline
#endif

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef void (*free_fn_t)(void *p, void *context);
typedef void *(*malloc_fn_t)(size_t min_size, void *context);
typedef void *(*realloc_fn_t)(void *p, size_t min_size, void *context);

typedef struct s_allocator {
  malloc_fn_t malloc;
  realloc_fn_t realloc;
  free_fn_t free;
  void *context;
} allocator_t;

extern allocator_t *g_default_allocator;

void *com_malloc(allocator_t *alloc, size_t min_size);
void *com_realloc(allocator_t *alloc, void *p, size_t min_size);
void com_free(allocator_t *alloc, void *p);

#ifdef __cplusplus
}
#endif // __cplusplus

#include <inline.end>

#endif /* end __SNOW__ALLOCATOR_H__ include guard */
