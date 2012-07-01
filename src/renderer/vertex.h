/*
  Vertex structures
  Written by Noel Cower

  See LICENSE.md for license information
*/

#ifndef __SNOW__VERTEX_H__
#define __SNOW__VERTEX_H__

#include <system/sgl.h>

#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

#define PACKED_STRUCT __attribute__((packed))

/*! Static vertex structure, used for objects that are not animated, but can
  move if need be (normals provided for vertex lighting).
*/
typedef struct s_fixed_vertex fixed_vertex_t;
/*! Animated vertex data - supports two bones.  Only has one set of texcoords. */
typedef struct s_anim_vertex anim_vertex_t;

struct s_fixed_vertex
{
  GLfloat position[3];
  GLfloat normals[3];
  GLfloat texcoord0[2];
  GLfloat texcoord1[2]; // lightmap coordinates
  GLubyte rgba[4];    // RGBA color
} PACKED_STRUCT;

struct s_anim_vertex
{
  GLfloat position[3];
  GLfloat normals[3];
  GLfloat texcoord0[2];
  GLfloat bones[2];
  GLushort weights[2];
} PACKED_STRUCT;

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* end of include guard: __SNOW__VERTEX_H__ */

