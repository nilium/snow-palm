/*
  Vertex structures
  Written by Noel Cower

  See LICENSE.md for license information
*/

#ifndef VERTEX_H

#define VERTEX_H

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
  union {
    GLubyte rgba[4];    // RGBA color
    GLuint color;
  };
} PACKED_STRUCT;

struct s_anim_vertex
{
  GLfloat position[3];
  GLfloat normals[3];
  GLfloat texcoord0[2];
  GLfloat bones[2];
  GLushort weights[2];
} PACKED_STRUCT;

typedef union u_vertex
{
  anim_vertex_t anim_vert;
  static_vertex_t static_vert;
} vertex_t;

#define S_VERTEX_STRIDE sizeof(vertex_t)

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* end of include guard: VERTEX_H */

