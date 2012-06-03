#ifndef __SNOW_ALLOCATOR_H__
#define __SNOW_ALLOCATOR_H__ 1

typedef void (*free_fn_t)(void *p, void *context);
typedef void *(*malloc_fn_t)(size_t min_size, void *context);
typedef void *(*realloc_fn_t)(void *p, size_t min_size, void *context);

typedef struct s_allocator {
  malloc_fn_t malloc;
  realloc_fn_t realloc;
  free_fn_t free;
  void *context;
} allocator_t;

inline void *com_malloc(allocator_t *alloc, size_t min_size) {
  return alloc->malloc(min_size, alloc->context);
}

inline void *com_realloc(allocator_t *alloc, void *p, size_t min_size) {
  return alloc->realloc(p, min_size, alloc->context);
}

inline void com_free(allocator_t *alloc, void *p) {
  return alloc->free(p, alloc->context);
}

extern allocator_t *g_default_allocator;

#endif /* end __SNOW_ALLOCATOR_H__ include guard */
