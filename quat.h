#ifndef QUAT_H

#define QUAT_H

#include "math3d.h"

#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

extern const quat_t g_quat_identity;

/* note: all methods assume that input quaternions are unit quaternions */

void quat_set(float x, float y, float z, float w, quat_t out);
void quat_copy(const quat_t in, quat_t out);
void quat_identity(quat_t q);

void quat_inverse(const quat_t in, quat_t out);
void quat_negate(const quat_t in, quat_t out);

void quat_multiply(const quat_t left, const quat_t right, quat_t out);

void quat_fromAngleAxis(float angle, float x, float y, float z, quat_t out);
void quat_fromMat4(const mat4_t mat, quat_t out);

void quat_slerp(const quat_t from, const quat_t to, float delta, quat_t out);

int quat_equals(const quat_t left, const quat_t right);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* end of include guard: QUAT_H */

