/*
	Transformation matrix
	Written by Noel Cower

	See LICENSE.md for license information
*/

#ifndef MATRIX_H_3CZ1OZK7
#define MATRIX_H_3CZ1OZK7

#include "math3d.h"

#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

extern const mat4_t g_mat4_identity;

void mat4_identity(mat4_t out);
void mat4_copy(const mat4_t in, mat4_t out);
void mat4_set(
	float m00, float m01, float m02, float m03,
	float m04, float m05, float m06, float m07,
	float m08, float m09, float m10, float m11,
	float m12, float m13, float m14, float m15,
	mat4_t out);

void mat4_setAxes3(const vec3_t x, const vec3_t y, const vec3_t z, const vec3_t w, mat4_t out);
void mat4_getAxes3(const mat4_t m, vec3_t x, vec3_t y, vec3_t z, vec3_t w);
void mat4_setAxes4(const vec4_t x, const vec4_t y, const vec4_t z, const vec4_t w, mat4_t out);
void mat4_getAxes4(const mat4_t m, vec4_t x, vec4_t y, vec4_t z, vec4_t w);

/*! Builds a rotation matrix with the given angle and axis. */
void mat4_rotation(float angle, float x, float y, float z, mat4_t out);
void mat4_frustum(float left, float right, float bottom, float top, float near, float far, mat4_t out);
void mat4_orthographic(float left, float right, float bottom, float top, float near, float far, mat4_t out);
void mat4_perspective(float fovY, float aspect, float near, float far, mat4_t out);
void mat4_lookAt(const vec3_t eye, const vec3_t center, const vec3_t up, mat4_t out);
void mat4_fromQuat(const quat_t quat, mat4_t out);

void mat4_getRow4(const mat4_t in, int row, vec4_t out);
void mat4_getRow3(const mat4_t in, int row, vec3_t out);
void mat4_getColumn4(const mat4_t in, int column, vec4_t out);
void mat4_getColumn3(const mat4_t in, int column, vec3_t out);

int mat4_equals(const mat4_t left, const mat4_t right);

void mat4_transpose(const mat4_t in, mat4_t out);
void mat4_inverseOrthogonal(const mat4_t in, mat4_t out);
/*!
 * Writes the inverse affine of the input matrix to the output matrix.
 * \returns Non-zero if an inverse affine matrix can be created, otherwise
 * zero if not.  If zero, the output matrix is the identity matrix.
 */
int mat4_inverseAffine(const mat4_t in, mat4_t out);
void mat4_adjoint(const mat4_t in, mat4_t out);
float mat4_determinant(const mat4_t m);
int mat4_inverseGeneral(const mat4_t in, mat4_t out);

/*! Translates the given matrix by <X, Y, Z>. */
void mat4_translate(float x, float y, float z, mat4_t m);
void mat4_multiply(const mat4_t left, const mat4_t right, mat4_t out);
void mat4_multiplyVec4(const mat4_t left, const vec4_t right, vec4_t out);
void mat4_transformVec3(const mat4_t left, const vec3_t right, vec3_t out);
void mat4_rotateVec3(const mat4_t left, const vec3_t right, vec3_t out);
void mat4_invRotateVec3(const mat4_t left, const vec3_t right, vec3_t out);
void mat4_scale(const mat4_t in, float x, float y, float z, mat4_t out);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* end of include guard: MATRIX_H_3CZ1OZK7 */

