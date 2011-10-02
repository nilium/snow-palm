/*
 * math3d.h
 *
 * Created by Noel R. Cower on 29-09-2011.
 * Copyright (c) 2011 Noel R. Cower.  All rights reserved.
 */

#ifndef MATH3D_H_G3RBNYON
#define MATH3D_H_G3RBNYON

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

/* vector expansion */
#define vec2_expand(V) ((V)[0]), ((V)[1])
#define vec3_expand(V) ((V)[0]), ((V)[1]), ((V)[2])
#define vec4_expand(V) ((V)[0]), ((V)[1]), ((V)[2]), ((V)[3])

/* matrix expansion */
#define mat4_expand(M) \
	((M)[0 ]), ((M)[1 ]), ((M)[2 ]), ((M)[3 ]), \
	((M)[4 ]), ((M)[5 ]), ((M)[6 ]), ((M)[7 ]), \
	((M)[8 ]), ((M)[9 ]), ((M)[10]), ((M)[11]), \
	((M)[12]), ((M)[13]), ((M)[14]), ((M)[15])

/* matrix expansion transposed */
#define mat4_expandTransposed(M) \
	((M)[0 ]), ((M)[4 ]), ((M)[8 ]), ((M)[12]), \
	((M)[1 ]), ((M)[5 ]), ((M)[9 ]), ((M)[13]), \
	((M)[2 ]), ((M)[6 ]), ((M)[10]), ((M)[14]), \
	((M)[3 ]), ((M)[7 ]), ((M)[11]), ((M)[15])

/* matrix column expansion */
#define mat4_expandCX(M) \
	((M)[0 ]), ((M)[4 ]), ((M)[8 ]), ((M)[12])
#define mat4_expandCY(M) \
	((M)[1 ]), ((M)[5 ]), ((M)[9 ]), ((M)[13])
#define mat4_expandCZ(M) \
	((M)[2 ]), ((M)[6 ]), ((M)[10]), ((M)[14])
#define mat4_expandCW(M) \
	((M)[3 ]), ((M)[7 ]), ((M)[11]), ((M)[15])

/* matrix row expansion */
#define mat4_expandRX(M) \
	((M)[0 ]), ((M)[1 ]), ((M)[2 ]), ((M)[3 ])
#define mat4_expandRY(M) \
	((M)[4 ]), ((M)[5 ]), ((M)[6 ]), ((M)[7 ])
#define mat4_expandRZ(M) \
	((M)[8 ]), ((M)[9 ]), ((M)[10]), ((M)[11])
#define mat4_expandRW(M) \
	((M)[12]), ((M)[13]), ((M)[14]), ((M)[15])

/* quaternion expansion */
#define quat_expand(Q) ((Q)[0]), ((Q)[1]), ((Q)[2]), ((Q)[3])


/*!
 * Floating point epsilon for float comparisons.  This is, more or less, the limit
 * to accuracy.  If the difference between two floating point values is less than
 * the epsilon, they are for all intents and purposes the same.
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
#include <math.h>
#include "vec3.h"
#include "vec4.h"
#include "matrix.h"
#include "quat.h"

#endif /* end of include guard: MATH3D_H_G3RBNYON */

