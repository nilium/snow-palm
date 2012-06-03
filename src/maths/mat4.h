/*
  Transformation matrix
  Written by Noel Cower

  See LICENSE.md for license information
*/

#ifndef MATRIX_H_3CZ1OZK7
#define MATRIX_H_3CZ1OZK7

#include "maths.h"

#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

extern const mat4_t g_mat4_identity;

void mat4_identity(mat4_t out);
void mat4_copy(const mat4_t in, mat4_t out);
void mat4_set(
  s_float_t m00, s_float_t m01, s_float_t m02, s_float_t m03,
  s_float_t m04, s_float_t m05, s_float_t m06, s_float_t m07,
  s_float_t m08, s_float_t m09, s_float_t m10, s_float_t m11,
  s_float_t m12, s_float_t m13, s_float_t m14, s_float_t m15,
  mat4_t out);

void mat4_set_axes3(const vec3_t x, const vec3_t y, const vec3_t z, const vec3_t w, mat4_t out);
void mat4_get_axes3(const mat4_t m, vec3_t x, vec3_t y, vec3_t z, vec3_t w);
void mat4_set_axes4(const vec4_t x, const vec4_t y, const vec4_t z, const vec4_t w, mat4_t out);
void mat4_get_axes4(const mat4_t m, vec4_t x, vec4_t y, vec4_t z, vec4_t w);

/*! Builds a rotation matrix with the given angle and axis. */
void mat4_rotation(s_float_t angle, s_float_t x, s_float_t y, s_float_t z, mat4_t out);
void mat4_frustum(s_float_t left, s_float_t right, s_float_t bottom, s_float_t top, s_float_t near, s_float_t far, mat4_t out);
void mat4_orthographic(s_float_t left, s_float_t right, s_float_t bottom, s_float_t top, s_float_t near, s_float_t far, mat4_t out);
void mat4_perspective(s_float_t fov_y, s_float_t aspect, s_float_t near, s_float_t far, mat4_t out);
void mat4_look_at(const vec3_t eye, const vec3_t center, const vec3_t up, mat4_t out);
void mat4_from_quat(const quat_t quat, mat4_t out);

void mat4_get_row4(const mat4_t in, int row, vec4_t out);
void mat4_get_row3(const mat4_t in, int row, vec3_t out);
void mat4_get_column4(const mat4_t in, int column, vec4_t out);
void mat4_get_column3(const mat4_t in, int column, vec3_t out);

int mat4_equals(const mat4_t left, const mat4_t right);

void mat4_transpose(const mat4_t in, mat4_t out);
void mat4_inverse_orthogonal(const mat4_t in, mat4_t out);
/*!
 * Writes the inverse affine of the input matrix to the output matrix.
 * \returns Non-zero if an inverse affine matrix can be created, otherwise
 * zero if not.  If zero, the output matrix is the identity matrix.
 */
int mat4_inverse_affine(const mat4_t in, mat4_t out);
void mat4_adjoint(const mat4_t in, mat4_t out);
s_float_t mat4_determinant(const mat4_t m);
int mat4_inverse_general(const mat4_t in, mat4_t out);

/*! Translates the given matrix by <X, Y, Z>. */
void mat4_translate(s_float_t x, s_float_t y, s_float_t z, const mat4_t in, mat4_t out);
void mat4_translation(s_float_t x, s_float_t y, s_float_t z, mat4_t out);
void mat4_multiply(const mat4_t left, const mat4_t right, mat4_t out);
void mat4_multiply_vec4(const mat4_t left, const vec4_t right, vec4_t out);
void mat4_transform_vec3(const mat4_t left, const vec3_t right, vec3_t out);
void mat4_rotate_vec3(const mat4_t left, const vec3_t right, vec3_t out);
void mat4_inv_rotate_vec3(const mat4_t left, const vec3_t right, vec3_t out);
void mat4_scale(const mat4_t in, s_float_t x, s_float_t y, s_float_t z, mat4_t out);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* end of include guard: MATRIX_H_3CZ1OZK7 */

