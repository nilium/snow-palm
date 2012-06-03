#include "allocator.h"
#include <stdlib.h>

allocator_t default_allocator() {
  static allocator_t g_default_allocator = {
    free, malloc, realloc, NULL
  };
  return g_default_allocator;
}
