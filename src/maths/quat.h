/*
	Quaternion maths
	Written by Noel Cower

	See LICENSE.md for license information
*/

#ifndef QUAT_H

#define QUAT_H

#include "maths.h"

#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

extern const quat_t g_quat_identity;

/* note: all methods assume that input quaternions are unit quaternions */

void quat_set(s_float_t x, s_float_t y, s_float_t z, s_float_t w, quat_t out);
void quat_copy(const quat_t in, quat_t out);
void quat_identity(quat_t q);

void quat_inverse(const quat_t in, quat_t out);
void quat_negate(const quat_t in, quat_t out);

void quat_multiply(const quat_t left, const quat_t right, quat_t out);
void quat_multiply_vec3(const quat_t left, const vec3_t right, vec3_t out);

void quat_from_angle_axis(s_float_t angle, s_float_t x, s_float_t y, s_float_t z, quat_t out);
void quat_from_mat4(const mat4_t mat, quat_t out);

void quat_slerp(const quat_t from, const quat_t to, s_float_t delta, quat_t out);

int quat_equals(const quat_t left, const quat_t right);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* end of include guard: QUAT_H */

