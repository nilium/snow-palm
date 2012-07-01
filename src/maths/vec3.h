/*
  3D vector maths
  Written by Noel Cower

  See LICENSE.md for license information
*/

#ifndef __SNOW__VEC3_H__

#define __SNOW__VEC3_H__

#include "maths.h"

#ifdef __SNOW__VEC3_C__
#define S_INLINE
#else
#define S_INLINE inline
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern const vec3_t g_vec3_zero;
extern const vec3_t g_vec3_one;

void vec3_copy(const vec3_t in, vec3_t out);
void vec3_set(s_float_t x, s_float_t y, s_float_t z, vec3_t v);

/*!
 * Gets the squared length of a vector.  Useful for approximations and when
 * you don't need the actual magnitude.
 */
s_float_t vec3_length_squared(const vec3_t v);
/*!
 * Gets the length/magnitude of a vector.
 */
s_float_t vec3_length(const vec3_t v);
void vec3_normalize(const vec3_t in, vec3_t out);

void vec3_subtract(const vec3_t left, const vec3_t right, vec3_t out);
void vec3_add(const vec3_t left, const vec3_t right, vec3_t out);
void vec3_multiply(const vec3_t left, const vec3_t right, vec3_t out);
void vec3_invert(vec3_t v);

void vec3_cross_product(const vec3_t left, const vec3_t right, vec3_t out);
s_float_t vec3_dot_product(const vec3_t left, const vec3_t right);

void vec3_scale(s_float_t scalar, vec3_t v);
/*!
 * Divides the given vector by the divisor.
 * \returns Zero if the result is undefined, otherwise non-zero.
 */
int vec3_divide(s_float_t divisor, vec3_t v);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#include <inline.end>

#endif /* end of include guard: __SNOW__VEC3_H__ */

