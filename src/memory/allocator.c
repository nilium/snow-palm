#define __SNOW__ALLOCATOR_C__

#include "allocator.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

static void *sn_malloc(size_t min_size, void *ctx) {
  (void)ctx;
  void *p = malloc(min_size);
  if (p) memset(p, 0, min_size);
  return p;
}

static void sn_free(void *p, void *ctx) {
  (void)ctx;
  free(p);
}

static void *sn_realloc(void *p, size_t min_size, void *ctx) {
  (void)ctx;
  return realloc(p, min_size);
}

static allocator_t default_allocator = {
  .malloc = sn_malloc,
  .realloc = sn_realloc,
  .free = sn_free,
  .context = NULL
};

allocator_t *g_default_allocator = &default_allocator;

void *com_malloc(allocator_t *alloc, size_t min_size) {
  if (alloc == NULL) {
    s_log_warning("NULL allocator provided, using default allocator.");
    alloc = g_default_allocator;
  }
  return alloc->malloc(min_size, alloc->context);
}

void *com_realloc(allocator_t *alloc, void *p, size_t min_size) {
  if (alloc == NULL) {
    s_log_warning("NULL allocator provided, using default allocator.");
    alloc = g_default_allocator;
  }
  return alloc->realloc(p, min_size, alloc->context);
}

void com_free(allocator_t *alloc, void *p) {
  if (alloc == NULL) {
    s_log_warning("NULL allocator provided, using default allocator.");
    alloc = g_default_allocator;
  }
  return alloc->free(p, alloc->context);
}

#ifdef __cplusplus
}
#endif // __cplusplus
