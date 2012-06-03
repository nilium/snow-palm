#ifndef __SNOW_ALLOCATOR_H__
#define __SNOW_ALLOCATOR_H__ 1

typedef void (*free_fn_t)(void *);
typedef void *(*malloc_fn_t)(size_t);
typedef void *(*realloc_fn_t)(void *, size_t);

typedef struct s_allocator {
  free_fn_t free;
  malloc_fn_t malloc;
  realloc_fn_t realloc;
  void *context;
} allocator_t;

allocator_t default_allocator(void);

#endif /* end __SNOW_ALLOCATOR_H__ include guard */
