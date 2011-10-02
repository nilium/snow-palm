#include "quat.h"
#include "vec4.h"

#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

const quat_t g_quat_identity = {0.0, 0.0, 0.0, 1.0};

void quat_set(float x, float y, float z, float w, quat_t out)
{
	out[0] = x;
	out[1] = y;
	out[2] = z;
	out[3] = w;
}

void quat_copy(const quat_t in, quat_t out)
{
	if (in == out) return;

	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
	out[3] = in[3];
}

void quat_identity(quat_t q)
{
	q[0] = q[1] = q[2] = 0.0;
	q[3] = 1.0;
}

void quat_inverse(const quat_t in, quat_t out)
{
	out[0] = -in[0];
	out[1] = -in[1];
	out[2] = -in[2];
	out[3] = in[3];
}

void quat_negate(const quat_t in, quat_t out)
{
	out[0] = -in[0];
	out[1] = -in[1];
	out[2] = -in[2];
	out[3] = -in[3];
}

void quat_multiply(const quat_t left, const quat_t right, quat_t out)
{
	float w;
	float w1, w2;
	vec3_t t1, t2;
	w1 = left[3];
	w2 = right[3];

	w = (w1 * w2) - vec3_dotProduct(left, right);
	vec3_copy(right, t1);
	vec3_scale(w1, t1);
	vec3_copy(left, t2);
	vec3_scale(w2, t2);
	vec3_add(t1, t2, t1);
	vec3_crossProduct(right, left, t2);
	vec3_add(t1, t2, t1);
	vec3_copy(t1, out);
	out[3] = w;
}

void quat_fromAngleAxis(float angle, float x, float y, float z, quat_t out)
{
	vec3_t v = {x, y, z};
	vec3_normalize(v, v);

	angle *= (S_DEG2RAD * 0.5);
	const float s = sinf(angle);

	out[0] = v[0] * s;
	out[1] = v[1] * s;
	out[2] = v[2] * s;
	out[3] = cosf(angle);
}

void quat_fromMat4(const mat4_t mat, quat_t out)
{
	float trace, r;
	int index;

	trace = mat[0] + mat[5] + mat[10];
	if (trace > 0) {
		r = sqrtf(trace + 1.0);
		out[3] = r * 0.5;
		r = 0.5 / r;
		out[0] = (mat[9] - mat[6]) * r;
		out[1] = (mat[2] - mat[8]) * r;
		out[2] = (mat[4] - mat[1]) * r;
	} else {
		index = 0;
		if (mat[5] > mat[0]) index = 1;
		if (mat[10] > mat[index * 5]) index = 2;

		r = sqrtf(mat[index * 5] - (mat[((index + 1)) % 3 * 5] + mat[((index + 2) % 3) * 5]) + 1.0);
		out[index] = r * 0.5;
 
		if (r) r = 0.5 / r;

		switch (index)
		{
		case 0:
			out[1] = (mat[1] + mat[4]) * r;
			out[2] = (mat[2] + mat[8]) * r;
			out[3] = (mat[9] - mat[6]) * r;
		break;

		case 1:
			out[3] = (mat[2] + mat[8]) * r;
			out[2] = (mat[9] + mat[6]) * r;
			out[0] = (mat[1] - mat[4]) * r;
		break;

		case 2:
			out[3] = (mat[4] + mat[1]) * r;
			out[0] = (mat[2] + mat[8]) * r;
			out[1] = (mat[6] - mat[9]) * r;
		break;
		}
	}
}

void quat_slerp(const quat_t from, const quat_t to, float delta, quat_t out)
{
	float dot, scale0, scale1, angle, inverseSin;
	float dx, dy, dz, dw;
	
	dot = vec4_dotProduct((const float *)from, (const float *)to);
	
	if (dot < 0.0) {
		dot = -dot;
		dx = -to[0];
		dy = -to[1];
		dz = -to[2];
		dw = -to[3];
	} else {
		dx = to[0];
		dy = to[1];
		dz = to[2];
		dw = to[3];
	}

	delta = fminf(fmaxf(delta, 0.0), 1.0);

	angle = acosf(dot);
	inverseSin = 1.0 / sinf(dot);

	scale0 = sin((1.0 - delta) * angle) * inverseSin;
	scale1 = sin(delta * angle) * inverseSin;

	out[0] = (from[0] * scale0) + (dx * scale1);
	out[1] = (from[1] * scale0) + (dy * scale1);
	out[2] = (from[2] * scale0) + (dz * scale1);
	out[3] = (from[3] * scale0) + (dw * scale1);
}

int quat_equals(const quat_t left, const quat_t right)
{
	return (
		fabsf(left[0] - right[0]) < S_FLOAT_EPSILON &&
		fabsf(left[1] - right[1]) < S_FLOAT_EPSILON &&
		fabsf(left[2] - right[2]) < S_FLOAT_EPSILON &&
		fabsf(left[3] - right[3]) < S_FLOAT_EPSILON
		);
}

#if defined(__cplusplus)
}
#endif /* __cplusplus */

