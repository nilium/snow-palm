#include "allocator.h"
#include <stdlib.h>

static void *sn_malloc(size_t min_size, void *ctx) {
  (void)ctx;
  return malloc(min_size);
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
