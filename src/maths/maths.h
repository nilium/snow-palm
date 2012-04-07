/*
	3D math types & macros
	Written by Noel Cower

	See LICENSE.md for license information
*/


#ifndef MATHS_H_G3RBNYON
#define MATHS_H_G3RBNYON

#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

typedef float mat4_t[16];
typedef float vec4_t[4];
typedef float vec3_t[3];
typedef float quat_t[4];

/*
 * macros to expand vectors/matrices/quaternions for passing their components as arguments
 */

/* Expands a vector to the value of each component of the vector. */
#define vec2_expand(V) ((V)[0]), ((V)[1])
#define vec3_expand(V) ((V)[0]), ((V)[1]), ((V)[2])
#define vec4_expand(V) ((V)[0]), ((V)[1]), ((V)[2]), ((V)[3])

/* Expands a vector to an address for each component of the vector */
#define vec2_expand_addr(V) (V), (V+1)
#define vec3_expand_addr(V) (V), (V+1), ((V)+2)
#define vec4_expand_addr(V) (V), (V+1), ((V)+2), ((V)+3)

/* Matrix expansion - works the same as vec*_expand by providing a comma-separated list of the matrix's components. */
#define mat4_expand(M) \
	((M)[0 ]), ((M)[1 ]), ((M)[2 ]), ((M)[3 ]), \
	((M)[4 ]), ((M)[5 ]), ((M)[6 ]), ((M)[7 ]), \
	((M)[8 ]), ((M)[9 ]), ((M)[10]), ((M)[11]), \
	((M)[12]), ((M)[13]), ((M)[14]), ((M)[15])

/* Transposed matrix expansion - same as mat4_expand, but transposed. */
#define mat4_expand_transposed(M) \
	((M)[0 ]), ((M)[4 ]), ((M)[8 ]), ((M)[12]), \
	((M)[1 ]), ((M)[5 ]), ((M)[9 ]), ((M)[13]), \
	((M)[2 ]), ((M)[6 ]), ((M)[10]), ((M)[14]), \
	((M)[3 ]), ((M)[7 ]), ((M)[11]), ((M)[15])

/* Matrix column expansion */
#define mat4_expand_cX(M) \
	((M)[0 ]), ((M)[4 ]), ((M)[8 ]), ((M)[12])
#define mat4_expand_cY(M) \
	((M)[1 ]), ((M)[5 ]), ((M)[9 ]), ((M)[13])
#define mat4_expand_cZ(M) \
	((M)[2 ]), ((M)[6 ]), ((M)[10]), ((M)[14])
#define mat4_expand_cW(M) \
	((M)[3 ]), ((M)[7 ]), ((M)[11]), ((M)[15])

/* matrix row expansion */
#define mat4_expand_rX(M) \
	((M)[0 ]), ((M)[1 ]), ((M)[2 ]), ((M)[3 ])
#define mat4_expand_rY(M) \
	((M)[4 ]), ((M)[5 ]), ((M)[6 ]), ((M)[7 ])
#define mat4_expand_rZ(M) \
	((M)[8 ]), ((M)[9 ]), ((M)[10]), ((M)[11])
#define mat4_expand_rW(M) \
	((M)[12]), ((M)[13]), ((M)[14]), ((M)[15])

/* Quaternion expansion */
#define quat_expand(Q) ((Q)[0]), ((Q)[1]), ((Q)[2]), ((Q)[3])
#define quat_expand_addr(Q) (Q), (Q+1), ((Q)+2), ((Q)+3)


/*!
 * Floating point epsilon for float comparisons.  This is, more or less, the
 * limit to accuracy.  If the difference between two floating point values is
 * less than the epsilon, they are for all intents and purposes the same.
 *
 * It should be stressed that this is absolutely not an accurate epsilon.
 */
#define S_FLOAT_EPSILON (1.0e-6)

#define S_DEG2RAD (0.01745329)
#define S_RAD2DEG (57.2957795)

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#include "config.h"
#include "vec3.h"
#include "vec4.h"
#include "matrix.h"
#include "quat.h"

#endif /* end of include guard: MATHS_H_G3RBNYON */

