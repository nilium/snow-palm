/*
	3D vector maths
	Written by Noel Cower

	See LICENSE.md for license information
*/

#ifndef VEC3_H

#define VEC3_H

#include "math3d.h"

extern const vec3_t g_vec3_zero;
extern const vec3_t g_vec3_one;

void vec3_copy(const vec3_t in, vec3_t out);
void vec3_set(float x, float y, float z, vec3_t v);

/*!
 * Gets the squared length of a vector.  Useful for approximations and when
 * you don't need the actual magnitude.
 */
float vec3_lengthSquared(const vec3_t v);
/*!
 * Gets the length/magnitude of a vector.
 */
float vec3_length(const vec3_t v);
void vec3_normalize(const vec3_t in, vec3_t out);

void vec3_subtract(const vec3_t left, const vec3_t right, vec3_t out);
void vec3_add(const vec3_t left, const vec3_t right, vec3_t out);
void vec3_multiply(const vec3_t left, const vec3_t right, vec3_t out);
void vec3_invert(vec3_t v);

void vec3_crossProduct(const vec3_t left, const vec3_t right, vec3_t out);
float vec3_dotProduct(const vec3_t left, const vec3_t right);

void vec3_scale(float scalar, vec3_t v);
/*!
 * Divides the given vector by the divisor.
 * \returns Zero if the result is undefined, otherwise non-zero.
 */
int vec3_divide(float divisor, vec3_t v);

#endif /* end of include guard: VEC3_H */

