#define NO_INLINE_FLOAT_OPS

#include "maths.h"

int float_is_zero(const s_float_t x) {
  return (fabs(x) < S_FLOAT_EPSILON);
}

int float_equals(const s_float_t x, const s_float_t y) {
  return (fabs(x - y) < S_FLOAT_EPSILON);
}
