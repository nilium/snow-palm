/*
  4D vector maths
  Written by Noel Cower

  See LICENSE.md for license information
*/

#define __SNOW__VEC4_C__

#include "vec4.h"

#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

const vec4_t g_vec4_zero = {0.0, 0.0, 0.0, 0.0};
const vec4_t g_vec4_one = {1.0, 1.0, 1.0, 1.0};

void vec4_copy(const vec4_t in, vec4_t out)
{
  out[0] = in[0];
  out[1] = in[1];
  out[2] = in[2];
  out[3] = in[3];
}

void vec4_set(s_float_t x, s_float_t y, s_float_t z, s_float_t w, vec4_t v)
{
  v[0] = x;
  v[1] = y;
  v[2] = z;
  v[3] = w;
}

/*!
 * Gets the squared length of a vector.  Useful for approximations and when
 * you don't need the actual magnitude.
 */
s_float_t vec4_length_squared(const vec4_t v)
{
  return (v[0] * v[0]) + (v[1] * v[1]) + (v[2] * v[2]) + (v[3] * v[3]);
}

/*!
 * Gets the length/magnitude of a vector.
 */
s_float_t vec4_length(const vec4_t v)
{
  return sqrtf((v[0] * v[0]) + (v[1] * v[1]) + (v[2] * v[2]) + (v[3] * v[3]));
}

void vec4_normalize(const vec4_t in, vec4_t out)
{
  s_float_t mag = vec4_length(in);
  if (mag) mag = 1.0 / mag;
  out[0] = in[0] * mag;
  out[1] = in[1] * mag;
  out[2] = in[2] * mag;
  out[3] = in[3] * mag;
}

void vec4_subtract(const vec4_t left, const vec4_t right, vec4_t out)
{
  out[0] = left[0] - right[0];
  out[1] = left[1] - right[1];
  out[2] = left[2] - right[2];
  out[3] = left[3] - right[3];
}

void vec4_add(const vec4_t left, const vec4_t right, vec4_t out)
{
  out[0] = left[0] + right[0];
  out[1] = left[1] + right[1];
  out[2] = left[2] + right[2];
  out[3] = left[3] + right[3];
}

void vec4_multiply(const vec4_t left, const vec4_t right, vec4_t out)
{
  out[0] = left[0] * right[0];
  out[1] = left[1] * right[1];
  out[2] = left[2] * right[2];
  out[3] = left[3] * right[3];
}

void vec4_invert(vec4_t v)
{
  v[0] = -v[0];
  v[1] = -v[1];
  v[2] = -v[2];
  v[3] = -v[3];
}

s_float_t vec4_dot_product(const vec4_t left, const vec4_t right)
{
  return ((left[0] * right[0]) + (left[1] * right[1]) + (left[2] * right[2]) + (left[3] * right[3]));
}

void vec4_scale(s_float_t scalar, vec4_t v)
{
  v[0] *= scalar;
  v[1] *= scalar;
  v[2] *= scalar;
  v[3] *= scalar;
}

/*!
 * Divides the given vector by the divisor.
 * \returns Zero if the result is undefined, otherwise non-zero.
 */
int vec4_divide(s_float_t divisor, vec4_t v)
{
  if (divisor) {
    divisor = 1.0 / divisor;
    v[0] *= divisor;
    v[1] *= divisor;
    v[2] *= divisor;
    v[3] *= divisor;
    return 1;
  }
  return 0;
}

#if defined(__cplusplus)
}
#endif /* __cplusplus */

