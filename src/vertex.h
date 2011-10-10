/*
	Vertex structures
	Written by Noel Cower

	See LICENSE.md for license information
*/

#ifndef VERTEX_H

#define VERTEX_H

#include <GLES2/gl2.h>

#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

/*! Static vertex structure, used for objects that are not animated, but can
	move if need be (normals provided for vertex lighting).
*/
typedef struct s_static_vertex static_vertex_t;
/*! Animated vertex data - supports two bones.  Only has one set of texcoords. */
typedef struct s_anim_vertex anim_vertex_t;

struct s_static_vertex
{
	float position[3];
	float normals[3];
	float texcoord0[2];
	float texcoord1[2]; // lightmap coordinates
	union {
		GLubyte rgba[4];		// RGBA color
		GLuint color;
	};
};

struct s_anim_vertex
{
	float position[3];
	float normals[3];
	float texcoord0[2];
	float bones[2];
	GLushort weights[2];
};

typedef union u_vertex
{
	anim_vertex_t animVert;
	static_vertex_t staticVert;
} vertex_t;

#define S_VERTEX_STRIDE sizeof(vertex_t)

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* end of include guard: VERTEX_H */

