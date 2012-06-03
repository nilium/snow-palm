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

allocator_t default_allocator() {
  static allocator_t g_default_allocator = {
    sn_malloc, sn_realloc, sn_free, NULL
  };
  return g_default_allocator;
}
