/*
  4D vector maths
  Written by Noel Cower

  See LICENSE.md for license information
*/

#ifndef VEC4_H

#define VEC4_H

#include "maths.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern const vec4_t g_vec4_zero;
extern const vec4_t g_vec4_one;

void vec4_copy(const vec4_t in, vec4_t out);
void vec4_set(s_float_t x, s_float_t y, s_float_t z, s_float_t w, vec4_t v);

/*!
 * Gets the squared length of a vector.  Useful for approximations and when
 * you don't need the actual magnitude.
 */
s_float_t vec4_length_squared(const vec4_t v);
/*!
 * Gets the length/magnitude of a vector.
 */
s_float_t vec4_length(const vec4_t v);
void vec4_normalize(const vec4_t in, vec4_t out);

void vec4_subtract(const vec4_t left, const vec4_t right, vec4_t out);
void vec4_add(const vec4_t left, const vec4_t right, vec4_t out);
void vec4_multiply(const vec4_t left, const vec4_t right, vec4_t out);
void vec4_invert(vec4_t v);

s_float_t vec4_dot_product(const vec4_t left, const vec4_t right);

void vec4_scale(s_float_t scalar, vec4_t v);
/*!
 * Divides the given vector by the divisor.
 * \returns Zero if the result is undefined, otherwise non-zero.
 */
int vec4_divide(s_float_t divisor, vec4_t v);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* end of include guard: VEC4_H */

