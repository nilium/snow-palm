/*
	4D vector maths
	Written by Noel Cower

	See LICENSE.md for license information
*/

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

void vec4_set(float x, float y, float z, float w, vec4_t v)
{
	v[0] = x;
	v[1] = y;
	v[2] = z;
	v[3] = w;
}

float vec4_length_squared(const vec4_t v)
{
	return (v[0] * v[0]) + (v[1] * v[1]) + (v[2] * v[2]) + (v[3] * v[3]);
}

float vec4_length(const vec4_t v)
{
	return sqrtf((v[0] * v[0]) + (v[1] * v[1]) + (v[2] * v[2]) + (v[3] * v[3]));
}

void vec4_normalize(const vec4_t in, vec4_t out)
{
	float mag = vec4_length(in);
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

float vec4_dot_product(const vec4_t left, const vec4_t right)
{
	return ((left[0] * right[0]) + (left[1] * right[1]) + (left[2] * right[2]) + (left[3] * right[3]));
}

void vec4_scale(float scalar, vec4_t v)
{
	v[0] *= scalar;
	v[1] *= scalar;
	v[2] *= scalar;
	v[3] *= scalar;
}

int vec4_divide(float divisor, vec4_t v)
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

