#ifndef __SNOW_MESH_H__
#define __SNOW_MESH_H__ 1

#include <snow-config.h>
#include <serialize/serialize.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct s_rmesh {
  GLuint num_bones;
  GLuint num_vertices;
  GLuint num_indices;

  struct s_rmodel *owner;
  struct s_rbones bones[];

  struct {
    GLuint vertex_buffer;
    GLuint index_buffer;
  } gl;

  union {
    static_vertex_t statics[];
    anim_vertex_t dynamics[];
  } vertices;
} rmesh_t;

rmesh_t *rmesh_new(allocator_t *alloc);
void rmesh_destroy(rmesh_t *mesh);

// Prepare the mesh to be drawn.
void rmesh_prepare_to_draw(rmesh_t *mesh);

// serialization routines
void sz_write_mesh(sz_context_t *ctx, rmesh_t *mesh);
rmesh_t *sz_read_mesh(sz_context_t *ctx, allocator_t *alloc);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* end __SNOW_MESH_H__ include guard */
