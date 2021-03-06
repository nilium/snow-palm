/*
  3D vector maths
  Written by Noel Cower

  See LICENSE.md for license information
*/

#define __SNOW__VEC3_C__

#include "vec3.h"

#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

const vec3_t g_vec3_zero = {0.0, 0.0, 0.0};
const vec3_t g_vec3_one = {1.0, 1.0, 1.0};

void vec3_copy(const vec3_t in, vec3_t out)
{
  out[0] = in[0];
  out[1] = in[1];
  out[2] = in[2];
}

void vec3_set(s_float_t x, s_float_t y, s_float_t z, vec3_t v)
{
  v[0] = x;
  v[1] = y;
  v[2] = z;
}

/*!
 * Gets the squared length of a vector.  Useful for approximations and when
 * you don't need the actual magnitude.
 */
s_float_t vec3_length_squared(const vec3_t v)
{
  return (v[0] * v[0]) + (v[1] * v[1]) + (v[2] * v[2]);
}

/*!
 * Gets the length/magnitude of a vector.
 */
s_float_t vec3_length(const vec3_t v)
{
  return sqrtf((v[0] * v[0]) + (v[1] * v[1]) + (v[2] * v[2]));
}

void vec3_normalize(const vec3_t in, vec3_t out)
{
  s_float_t mag = vec3_length(in);
  if (mag) mag = 1.0 / mag;
  out[0] = in[0] * mag;
  out[1] = in[1] * mag;
  out[2] = in[2] * mag;
}

void vec3_subtract(const vec3_t left, const vec3_t right, vec3_t out)
{
  out[0] = left[0] - right[0];
  out[1] = left[1] - right[1];
  out[2] = left[2] - right[2];
}

void vec3_add(const vec3_t left, const vec3_t right, vec3_t out)
{
  out[0] = left[0] + right[0];
  out[1] = left[1] + right[1];
  out[2] = left[2] + right[2];
}

void vec3_multiply(const vec3_t left, const vec3_t right, vec3_t out)
{
  out[0] = left[0] * right[0];
  out[1] = left[1] * right[1];
  out[2] = left[2] * right[2];
}

void vec3_invert(vec3_t v)
{
  v[0] = -v[0];
  v[1] = -v[1];
  v[2] = -v[2];
}

void vec3_cross_product(const vec3_t left, const vec3_t right, vec3_t out)
{
  s_float_t x, y, z;
  x = (left[1] * right[2]) - (left[2] * right[1]);
  y = (left[0] * right[2]) - (left[2] * right[0]);
  z = (left[0] * right[1]) - (left[1] * right[0]);
  out[0] = x;
  out[1] = y;
  out[2] = z;
}

s_float_t vec3_dot_product(const vec3_t left, const vec3_t right)
{
  return ((left[0] * right[0]) + (left[1] * right[1]) + (left[2] * right[2]));
}

void vec3_scale(s_float_t scalar, vec3_t v)
{
  v[0] *= scalar;
  v[1] *= scalar;
  v[2] *= scalar;
}

/*!
 * Divides the given vector by the divisor.
 * \returns Zero if the result is undefined, otherwise non-zero.
 */
int vec3_divide(s_float_t divisor, vec3_t v)
{
  if (divisor) {
    divisor = 1.0 / divisor;
    v[0] *= divisor;
    v[1] *= divisor;
    v[2] *= divisor;
    return 1;
  }
  return 0;
}

#if defined(__cplusplus)
}
#endif /* __cplusplus */

