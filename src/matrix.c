#include "matrix.h"
#include "vec3.h"
#include "vec4.h"

#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

/* reference:
	x	y	z	w
	0	1	2	3
	4	5	6	7
	8	9	10	11
	12	13	14	15
*/

const mat4_t g_mat4_identity = {
	1.0, 0.0, 0.0, 0.0,
	0.0, 1.0, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.0, 0.0, 0.0, 1.0
};

static inline float mat4_cofactor(const mat4_t m, int r0, int r1, int r2, int c0, int c1, int c2);

void mat4_identity(mat4_t out)
{
	out[0] = out[5] = out[10] = out[15] = 1.0;
	out[1] = out[2] = out[3] =
	out[4] = out[6] = out[7] =
	out[8] = out[9] = out[11] =
	out[12] = out[13] = out[14] = 0.0;
}

void mat4_copy(const mat4_t in, mat4_t out)
{
	if (in != out) {
		int index = 0;
		for (; index < 16; ++index) out[index] = in[index];
	}
}

void mat4_set(
	float m00, float m01, float m02, float m03,
	float m04, float m05, float m06, float m07,
	float m08, float m09, float m10, float m11,
	float m12, float m13, float m14, float m15,
	mat4_t out)
{
	out[0 ] = m00;
	out[1 ] = m01;
	out[2 ] = m02;
	out[3 ] = m03;
	out[4 ] = m04;
	out[5 ] = m05;
	out[6 ] = m06;
	out[7 ] = m07;
	out[8 ] = m08;
	out[9 ] = m09;
	out[10] = m10;
	out[11] = m11;
	out[12] = m12;
	out[13] = m13;
	out[14] = m14;
	out[15] = m15;
}

void mat4_setAxes3(const vec3_t x, const vec3_t y, const vec3_t z, const vec3_t w, mat4_t out)
{
	out[0] = x[0];
	out[4] = x[1];
	out[8] = x[2];

	out[1] = y[0];
	out[5] = y[1];
	out[9] = y[2];

	out[2] = z[0];
	out[6] = z[1];
	out[10] = z[2];

	out[3] = w[0];
	out[7] = w[1];
	out[11] = w[2];
	
	out[3] = out[7] = out[12] = 0.0;
	out[15] = 1.0;
}

void mat4_getAxes3(const mat4_t m, vec3_t x, vec3_t y, vec3_t z, vec3_t w)
{
	if (x) {
		x[0] = m[0];
		x[1] = m[4];
		x[2] = m[8];
	}

	if (y) {
		y[0] = m[1];
		y[1] = m[5];
		y[2] = m[9];
	}

	if (z) {
		z[0] = m[2];
		z[1] = m[6];
		z[2] = m[10];
	}

	if (w) {
		w[0] = m[3];
		w[1] = m[7];
		w[2] = m[11];
	}
}

void mat4_setAxes4(const vec4_t x, const vec4_t y, const vec4_t z, const vec4_t w, mat4_t out)
{
	out[0] = x[0];
	out[4] = x[1];
	out[8] = x[2];
	out[12] = x[3];

	out[1] = y[0];
	out[5] = y[1];
	out[9] = y[2];
	out[13] = y[3];

	out[2] = z[0];
	out[6] = z[1];
	out[10] = z[2];
	out[14] = z[3];

	out[3] = w[0];
	out[7] = w[1];
	out[11] = w[2];
	out[15] = w[3];
}

void mat4_getAxes4(const mat4_t m, vec4_t x, vec4_t y, vec4_t z, vec4_t w)
{
	if (x) {
		x[0] = m[0];
		x[1] = m[4];
		x[2] = m[8];
		x[3] = m[12];
	}

	if (y) {
		y[0] = m[1];
		y[1] = m[5];
		y[2] = m[9];
		y[3] = m[13];
	}

	if (z) {
		z[0] = m[2];
		z[1] = m[6];
		z[2] = m[10];
		z[3] = m[14];
	}

	if (w) {
		w[0] = m[3];
		w[1] = m[7];
		w[2] = m[11];
		w[3] = m[15];
	}
}


void mat4_rotation(float angle, float x, float y, float z, mat4_t out)
{
	const float angle_rad = angle * S_DEG2RAD;
	const float c = cosf(angle_rad);
	const float s = sinf(angle_rad);
	const float ic = 1.0 - c;
	const float xy = x * y * ic;
	const float yz = y * z * ic;
	const float xz = x * z * ic;
	const float xs = s * x;
	const float ys = s * y;
	const float zs = s * z;

	out[0] = ((x * x) * ic) + c;
	out[1] = xy + zs;
	out[2] = xz - ys;
	out[4] = xy - zs;
	out[5] = ((y * y) * ic) + c;
	out[6] = yz + xs;
	out[8] = xz + ys;
	out[9] = yz - xs;
	out[10] = ((z * z) * ic) + c;
	out[3] = out[7] = out[11] =
	out[12] = out[13] = out[14] = 0.0;
	out[15] = 1.0;
}

void mat4_frustum(float left, float right, float bottom, float top, float near, float far, mat4_t out)
{
	const float xdelta = right - left;
	const float ydelta = top - bottom;
	const float zdelta = far - near;
	const float neardouble = 2.0 * near;

	out[0] = neardouble / xdelta;
	out[2] = (right + left) / xdelta;
	out[5] = neardouble / ydelta;
	out[6] = (top + bottom) / ydelta;
	out[10] = -((far + near) / zdelta);
	out[11] = -1.0;
	out[14] = -((neardouble * far) / zdelta);
	out[1] = out[3] = out[4] =
	out[7] = out[8] = out[9] =
	out[12] = out[13] = out[15] = 0.0;
}

void mat4_orthographic(float left, float right, float bottom, float top, float near, float far, mat4_t out)
{
	const float xdelta = right - left;
	const float ydelta = top - bottom;
	const float zdelta = far - near;

	out[0] = 2.0 / xdelta;
	out[5] = 2.0 / ydelta;
	out[10] = -2.0 / zdelta;
	out[12] = -((right + left) / xdelta);
	out[13] = -((top + bottom) / ydelta);
	out[14] = -((far + near) / zdelta);
	out[15] = 1.0;
	out[1] = out[2] = out[3] =
	out[4] = out[6] = out[7] =
	out[8] = out[9] = out[11] = 0.0;
}

void mat4_perspective(float fovY, float aspect, float near, float far, mat4_t out)
{
	const float r = tanf(fovY * 0.5 * S_DEG2RAD);
	const float left = -r * aspect;
	const float right = r * aspect;
	const float bottom = -r;
	const float top = r;
	const float twoNear = 2 * near;
	const float zdelta = 1.0 / (near - far);

	out[0] = twoNear * (right - left);
	out[5] = twoNear / (top - bottom);
	out[10] = (far + near) * zdelta;
	out[11] = -1.0;
	out[14] = (twoNear * far) * zdelta;
	out[1] = out[2] = out[3] =
	out[4] = out[6] = out[7] =
	out[8] = out[9] = out[12] =
	out[13] = out[15] = 0.0;
}

void mat4_lookAt(const vec3_t eye, const vec3_t center, const vec3_t up, mat4_t out)
{
	vec3_t facingNorm, upNorm, s;
	/* facingNorm */
	vec3_subtract(center, eye, facingNorm);
	vec3_normalize(facingNorm, facingNorm);
	/* upNorm */
	vec3_normalize(up, upNorm);
	/* s */
	vec3_crossProduct(facingNorm, upNorm, s);
	vec3_normalize(s, s);
	/* upNorm rejigged */
	vec3_crossProduct(s, facingNorm, upNorm);
	upNorm[1] = -upNorm[1];
	facingNorm[0] = -facingNorm[0];
	facingNorm[1] = -facingNorm[1];
	facingNorm[2] = -facingNorm[2];

	mat4_setAxes3(s, upNorm, facingNorm, g_vec3_zero, out);
	mat4_translate(-eye[0], -eye[1], -eye[2], out);
}

void mat4_fromQuat(const quat_t quat, mat4_t out)
{
	float tx, ty, tz, xx, xy, xz, yy, yz, zz, wx, wy, wz;

	tx = 2.0 * quat[0];
	ty = 2.0 * quat[1];
	tz = 2.0 * quat[2];

	xx = tx * quat[0];
	xy = tx * quat[1];
	xz = tx * quat[2];

	yy = ty * quat[1];
	yz = tz * quat[1];

	zz = tz * quat[3];

	wx = tx * quat[3];
	wy = ty * quat[3];
	wz = tz * quat[3];

	out[0 ] = 1.0 - (yy + zz);
	out[1 ] = xy - wz;
	out[2 ] = xz + wy;
	out[4 ] = xy + wz;
	out[5 ] = 1.0 - (xx + zz);
	out[6 ] = yz - wx;
	out[8 ] = xz - wy;
	out[9 ] = yz + wx;
	out[10] = 1.0 - (xx + yy);

	out[7 ] = 0.0;
	out[3 ] = 0.0;
	out[11] = 0.0;
	out[12] = 0.0;
	out[13] = 0.0;
	out[14] = 0.0;

	out[15] = 1.0;
}

void mat4_getRow4(const mat4_t in, int row, vec4_t out)
{
	if (0 <= row && row < 4) {
		const float *rowvec = in+(row * 4);
		out[0] = rowvec[0];
		out[1] = rowvec[1];
		out[2] = rowvec[2];
		out[3] = rowvec[3];
	}
}

void mat4_getRow3(const mat4_t in, int row, vec3_t out)
{
	if (0 <= row && row < 4) {
		const float *rowvec = in+(row * 4);
		out[0] = rowvec[0];
		out[1] = rowvec[1];
		out[2] = rowvec[2];
	}
}

void mat4_getColumn4(const mat4_t in, int column, vec4_t out)
{
	if (0 <= column && column < 4) {
		const float *colvec = in+column;
		out[0] = colvec[0];
		out[1] = colvec[4];
		out[2] = colvec[8];
		out[3] = colvec[12];
	}
}

void mat4_getColumn3(const mat4_t in, int column, vec3_t out)
{
	if (0 <= column && column < 4) {
		const float *colvec = in+column;
		out[0] = colvec[0];
		out[1] = colvec[4];
		out[2] = colvec[8];
	}
}

int mat4_equals(const mat4_t left, const mat4_t right)
{
	/*
	compare the XYZ components of all axes first, since they are the most likely
	to vary between checks
	*/
	return (
		fabsf(left[0] - right[0]) < S_FLOAT_EPSILON &&
		fabsf(left[1] - right[1]) < S_FLOAT_EPSILON &&
		fabsf(left[2] - right[2]) < S_FLOAT_EPSILON &&
                                
		fabsf(left[4] - right[4]) < S_FLOAT_EPSILON &&
		fabsf(left[5] - right[5]) < S_FLOAT_EPSILON &&
		fabsf(left[6] - right[6]) < S_FLOAT_EPSILON &&
                                
		fabsf(left[8] - right[8]) < S_FLOAT_EPSILON &&
		fabsf(left[9] - right[9]) < S_FLOAT_EPSILON &&
		fabsf(left[10] - right[10]) < S_FLOAT_EPSILON &&
                                  
		fabsf(left[12] - right[12]) < S_FLOAT_EPSILON &&
		fabsf(left[13] - right[13]) < S_FLOAT_EPSILON &&
		fabsf(left[14] - right[14]) < S_FLOAT_EPSILON &&

		fabsf(left[3] - right[3]) < S_FLOAT_EPSILON &&
		fabsf(left[7] - right[7]) < S_FLOAT_EPSILON &&
		fabsf(left[11] - right[11]) < S_FLOAT_EPSILON &&
		fabsf(left[15] - right[15]) < S_FLOAT_EPSILON
		);
}

void mat4_transpose(const mat4_t in, mat4_t out)
{
	float temp;
	#define swapElem(X, Y) temp = in[(X)]; out[(X)] = in[(Y)]; out[(Y)] = temp
	swapElem(1, 4);
	swapElem(8, 2);
	swapElem(9, 6);
	swapElem(12, 3);
	swapElem(13, 7);
	swapElem(14, 11);
	out[0] = in[0];
	out[5] = in[5];
	out[10] = in[10];
	out[15] = in[15];
}

void mat4_inverseOrthogonal(const mat4_t in, mat4_t out)
{
	const float m12 = in[12];
	const float m13 = in[13];
	const float m14 = in[14];
	mat4_t temp = {
		in[0],
		in[4],
		in[8],
		0.0,
		in[1],
		in[5],
		in[9],
		0.0,
		in[2],
		in[6],
		in[10],
		0.0
	};

	temp[12] = -((m12 * temp[0]) + (m13 * temp[4]) + (m14 * temp[8]));
	temp[13] = -((m12 * temp[1]) + (m13 * temp[5]) + (m14 * temp[9]));
	temp[14] = -((m12 * temp[2]) + (m13 * temp[6]) + (m14 * temp[10]));
	temp[15] = 1.0;

	mat4_copy(temp, out);
}

int mat4_inverseAffine(const mat4_t in, mat4_t out)
{
	mat4_t temp;
	float det;
	float m12, m13, m14;

	temp[0 ] = (in[5 ] * in[10]) - (in[6 ] * in[9 ]);
	temp[1 ] = (in[2 ] * in[9 ]) - (in[1 ] * in[10]);
	temp[2 ] = (in[1 ] * in[6 ]) - (in[2 ] * in[5 ]);

	temp[4 ] = (in[6 ] * in[ 8]) - (in[4 ] * in[10]);
	temp[5 ] = (in[0 ] * in[10]) - (in[2 ] * in[ 8]);
	temp[6 ] = (in[2 ] * in[ 4]) - (in[0 ] * in[ 6]);

	temp[8 ] = (in[4 ] * in[ 9]) - (in[5 ] * in[ 8]);
	temp[9 ] = (in[1 ] * in[ 8]) - (in[0 ] * in[ 9]);
	temp[10] = (in[0 ] * in[ 5]) - (in[1 ] * in[ 4]);

	det = (in[0] * temp[0]) + (in[1] * temp[4]) + (in[2] * temp[8]);
	if (fabsf(det) < S_FLOAT_EPSILON) {
		mat4_identity(out);
		return 0;
	}

	det = 1.0 / det;

	out[0] = temp[0] * det;
	out[1] = temp[1] * det;
	out[2] = temp[2] * det;
	out[4] = temp[4] * det;
	out[5] = temp[5] * det;
	out[6] = temp[6] * det;
	out[8 ] = temp[8 ] * det;
	out[9 ] = temp[9 ] * det;
	out[10] = temp[10] * det;

	m12 = in[12];
	m13 = in[13];
	m14 = in[14];

	out[12] = -((m12 * temp[0]) + (m13 * temp[4]) + (m14 * temp[8 ]));
	out[13] = -((m12 * temp[1]) + (m13 * temp[5]) + (m14 * temp[9 ]));
	out[14] = -((m12 * temp[2]) + (m13 * temp[6]) + (m14 * temp[10]));

	out[3] = out[7] = out[11] = 0.0;
	out[15] = 1.0;

	return 1;
}

static inline float mat4_cofactor(const mat4_t m, int r0, int r1, int r2, int c0, int c1, int c2)
{
	#define cofactor_addr(l, r) (m[l*4+r])
	return (
		(cofactor_addr(r0,c0) * ((cofactor_addr(r1, c1) * cofactor_addr(r2, c2)) - (cofactor_addr(r2, c1) * cofactor_addr(r1, c2)))) -
		(cofactor_addr(r0,c1) * ((cofactor_addr(r1, c0) * cofactor_addr(r2, c2)) - (cofactor_addr(r2, c0) * cofactor_addr(r1, c2)))) +
		(cofactor_addr(r0,c2) * ((cofactor_addr(r1, c0) * cofactor_addr(r2, c1)) - (cofactor_addr(r2, c0) * cofactor_addr(r1, c1))))
		);
	#undef cofactor_addr
}

void mat4_adjoint(const mat4_t in, mat4_t out)
{
	if (in == out) {
		mat4_t temp = {
			 mat4_cofactor(in, 1, 2, 3, 1, 2, 3),
			-mat4_cofactor(in, 0, 2, 3, 1, 2, 3),
			 mat4_cofactor(in, 0, 1, 3, 1, 2, 3),
			-mat4_cofactor(in, 0, 1, 2, 1, 2, 3),

			-mat4_cofactor(in, 1, 2, 3, 0, 2, 3),
			 mat4_cofactor(in, 0, 2, 3, 0, 2, 3),
			-mat4_cofactor(in, 0, 1, 3, 0, 2, 3),
			 mat4_cofactor(in, 0, 1, 2, 0, 2, 3),

			 mat4_cofactor(in, 1, 2, 3, 0, 1, 3),
			-mat4_cofactor(in, 0, 2, 3, 0, 1, 3),
			 mat4_cofactor(in, 0, 1, 3, 0, 1, 3),
			-mat4_cofactor(in, 0, 1, 2, 0, 1, 3),

			-mat4_cofactor(in, 1, 2, 3, 0, 1, 2),
			 mat4_cofactor(in, 0, 2, 3, 0, 1, 2),
			-mat4_cofactor(in, 0, 1, 3, 0, 1, 2),
			 mat4_cofactor(in, 0, 1, 2, 0, 1, 2)
		};
		mat4_copy(temp, out);
	} else {
		out[0 ] =  mat4_cofactor(in, 1, 2, 3, 1, 2, 3);
		out[1 ] = -mat4_cofactor(in, 0, 2, 3, 1, 2, 3);
		out[2 ] =  mat4_cofactor(in, 0, 1, 3, 1, 2, 3);
		out[3 ] = -mat4_cofactor(in, 0, 1, 2, 1, 2, 3);

		out[4 ] = -mat4_cofactor(in, 1, 2, 3, 0, 2, 3);
		out[5 ] =  mat4_cofactor(in, 0, 2, 3, 0, 2, 3);
		out[6 ] = -mat4_cofactor(in, 0, 1, 3, 0, 2, 3);
		out[7 ] =  mat4_cofactor(in, 0, 1, 2, 0, 2, 3);

		out[8 ] =  mat4_cofactor(in, 1, 2, 3, 0, 1, 3);
		out[9 ] = -mat4_cofactor(in, 0, 2, 3, 0, 1, 3);
		out[10] =  mat4_cofactor(in, 0, 1, 3, 0, 1, 3);
		out[11] = -mat4_cofactor(in, 0, 1, 2, 0, 1, 3);

		out[12] = -mat4_cofactor(in, 1, 2, 3, 0, 1, 2);
		out[13] =  mat4_cofactor(in, 0, 2, 3, 0, 1, 2);
		out[14] = -mat4_cofactor(in, 0, 1, 3, 0, 1, 2);
		out[15] =  mat4_cofactor(in, 0, 1, 2, 0, 1, 2);
	}
}

float mat4_determinant(const mat4_t m)
{
	return (
		(m[0] * mat4_cofactor(m, 1, 2, 3, 1, 2, 3)) -
		(m[1] * mat4_cofactor(m, 1, 2, 3, 0, 2, 3)) +
		(m[2] * mat4_cofactor(m, 1, 2, 3, 0, 1, 3)) -
		(m[3] * mat4_cofactor(m, 1, 2, 3, 0, 1, 2))
		);
}

int mat4_inverseGeneral(const mat4_t in, mat4_t out)
{
	int index;
	float det = mat4_determinant(in);

	if (fabsf(det) < S_FLOAT_EPSILON) {
		mat4_identity(out);
		return 0;
	}

	mat4_adjoint(in, out);
	det = 1.0 / det;
	for (index = 0; index < 16; ++index) out[index] *= det;

	return 1;
}

void mat4_translate(float x, float y, float z, mat4_t m)
{
	m[12] += (x * m[0]) + (y * m[4]) + (z * m[8]);
	m[13] += (x * m[1]) + (y * m[5]) + (z * m[9]);
	m[14] += (x * m[2]) + (y * m[6]) + (z * m[10]);
	m[15] += (x * m[3]) + (y * m[7]) + (z * m[11]);
}

void mat4_multiply(const mat4_t left, const mat4_t right, mat4_t out)
{
	mat4_t temp;
	int index;

	for (index = 0; index < 4; ++index) {
		float cx, cy, cz, cw;
		cx = left[index];
		cy = left[index + 4];
		cz = left[index + 8];
		cw = left[index + 12];

		temp[index     ] = (cx * right[0 ]) + (cy * right[1 ]) + (cz * right[2 ]) + (cz * right[3 ]);
		temp[index + 4 ] = (cx * right[4 ]) + (cy * right[5 ]) + (cz * right[6 ]) + (cz * right[7 ]);
		temp[index + 8 ] = (cx * right[8 ]) + (cy * right[9 ]) + (cz * right[10]) + (cz * right[11]);
		temp[index + 12] = (cx * right[12]) + (cy * right[13]) + (cz * right[14]) + (cz * right[15]);
	}

	mat4_copy(temp, out);
}

void mat4_multiplyVec4(const mat4_t left, const vec4_t right, vec4_t out)
{
	const float x = right[0], y = right[1], z = right[2], w = right[3];
	out[0] = (x * left[0]) + (y * left[4]) + (z * left[8 ]) + (w * left[12]);
	out[1] = (x * left[1]) + (y * left[5]) + (z * left[9 ]) + (w * left[13]);
	out[2] = (x * left[2]) + (y * left[6]) + (z * left[10]) + (w * left[14]);
	out[3] = (x * left[3]) + (y * left[7]) + (z * left[11]) + (w * left[15]);
}

void mat4_transformVec3(const mat4_t left, const vec3_t right, vec3_t out)
{
	const float x = right[0], y = right[1], z = right[2];
	out[0] = (x * left[0]) + (y * left[4]) + (z * left[8 ]) + left[12];
	out[1] = (x * left[1]) + (y * left[5]) + (z * left[9 ]) + left[13];
	out[2] = (x * left[2]) + (y * left[6]) + (z * left[10]) + left[14];
}

void mat4_rotateVec3(const mat4_t left, const vec3_t right, vec3_t out)
{
	const float x = right[0], y = right[1], z = right[2];
	out[0] = (x * left[0]) + (y * left[4]) + (z * left[8 ]);
	out[1] = (x * left[1]) + (y * left[5]) + (z * left[9 ]);
	out[2] = (x * left[2]) + (y * left[6]) + (z * left[10]);
}

void mat4_invRotateVec3(const mat4_t left, const vec3_t right, vec3_t out)
{
	const float x = right[0], y = right[1], z = right[2];
	out[0] = (x * left[0]) + (y * left[1]) + (z * left[2 ]);
	out[1] = (x * left[4]) + (y * left[5]) + (z * left[6 ]);
	out[2] = (x * left[8]) + (y * left[9]) + (z * left[10]);
}

void mat4_scale(const mat4_t in, float x, float y, float z, mat4_t out)
{
	mat4_copy(in, out);
	out[0] *= x;
	out[4] *= x;
	out[8] *= x;

	out[1] *= y;
	out[5] *= y;
	out[9] *= y;

	out[2 ] *= z;
	out[6 ] *= z;
	out[10] *= z;
}

#if defined(__cplusplus)
}
#endif /* __cplusplus */

