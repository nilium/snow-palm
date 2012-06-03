/*
	3D math types & macros
	Written by Noel Cower

	See LICENSE.md for license information
*/

#ifndef MATHS_H_G3RBNYON
#define MATHS_H_G3RBNYON

#include <math.h>

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

typedef float s_float_t;
typedef s_float_t mat4_t[16];
typedef s_float_t vec4_t[4];
typedef s_float_t vec3_t[3];
typedef s_float_t vec2_t[2];
typedef s_float_t quat_t[4];

/*
 * macros to expand vectors/matrices/quaternions for passing their components as arguments
 */

/* Expands a vector to the value of each component of the vector. */
#define vec2_splat(V) ((V)[0]), ((V)[1])
#define vec3_splat(V) ((V)[0]), ((V)[1]), ((V)[2])
#define vec4_splat(V) ((V)[0]), ((V)[1]), ((V)[2]), ((V)[3])

/* Expands a vector to an address for each component of the vector */
#define vec2_splat_addr(V) (V), (V+1)
#define vec3_splat_addr(V) (V), (V+1), ((V)+2)
#define vec4_splat_addr(V) (V), (V+1), ((V)+2), ((V)+3)

/* Matrix expansion - works the same as vec*_splat by providing a comma-separated list of the matrix's components. */
#define mat4_splat(M) \
	((M)[0 ]), ((M)[1 ]), ((M)[2 ]), ((M)[3 ]), \
	((M)[4 ]), ((M)[5 ]), ((M)[6 ]), ((M)[7 ]), \
	((M)[8 ]), ((M)[9 ]), ((M)[10]), ((M)[11]), \
	((M)[12]), ((M)[13]), ((M)[14]), ((M)[15])

/* Transposed matrix expansion - same as mat4_splat, but transposed. */
#define mat4_splat_transposed(M) \
	((M)[0 ]), ((M)[4 ]), ((M)[8 ]), ((M)[12]), \
	((M)[1 ]), ((M)[5 ]), ((M)[9 ]), ((M)[13]), \
	((M)[2 ]), ((M)[6 ]), ((M)[10]), ((M)[14]), \
	((M)[3 ]), ((M)[7 ]), ((M)[11]), ((M)[15])

/* Matrix column expansion */
#define mat4_splat_cX(M) \
	((M)[0 ]), ((M)[4 ]), ((M)[8 ]), ((M)[12])
#define mat4_splat_cY(M) \
	((M)[1 ]), ((M)[5 ]), ((M)[9 ]), ((M)[13])
#define mat4_splat_cZ(M) \
	((M)[2 ]), ((M)[6 ]), ((M)[10]), ((M)[14])
#define mat4_splat_cW(M) \
	((M)[3 ]), ((M)[7 ]), ((M)[11]), ((M)[15])

/* matrix row expansion */
#define mat4_splat_rX(M) \
	((M)[0 ]), ((M)[1 ]), ((M)[2 ]), ((M)[3 ])
#define mat4_splat_rY(M) \
	((M)[4 ]), ((M)[5 ]), ((M)[6 ]), ((M)[7 ])
#define mat4_splat_rZ(M) \
	((M)[8 ]), ((M)[9 ]), ((M)[10]), ((M)[11])
#define mat4_splat_rW(M) \
	((M)[12]), ((M)[13]), ((M)[14]), ((M)[15])

/* Quaternion expansion */
#define quat_splat(Q) ((Q)[0]), ((Q)[1]), ((Q)[2]), ((Q)[3])
#define quat_splat_addr(Q) (Q), (Q+1), ((Q)+2), ((Q)+3)


/*!
 * Floating point epsilon for double comparisons.  This is, more or less, the
 * limit to accuracy.  If the difference between two floating point values is
 * less than the epsilon, they are for all intents and purposes the same.
 *
 * It should be stressed that this is absolutely not an accurate epsilon.
 */
#define S_FLOAT_EPSILON (1.0e-6)

#define S_DEG2RAD (0.01745329)
#define S_RAD2DEG (57.2957795)

inline int float_is_zero(const s_float_t x) {
	return (fabs(x) < S_FLOAT_EPSILON);
}

inline int float_equals(const s_float_t x, const s_float_t y) {
	return (fabs(x - y) < S_FLOAT_EPSILON);
}

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* end of include guard: MATHS_H_G3RBNYON */

