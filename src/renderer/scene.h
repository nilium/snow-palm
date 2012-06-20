#ifndef __SNOW_SCENE_H__
#define __SNOW_SCENE_H__ 1

#include <snow-config.h>
#include <memory/allocator.h>
#include <structs/list.h>
#include <threads/mutex.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct s_scene {
  allocator_t *alloc;
  // FIXME: Camera is not implemented, uncomment when it is.
  // the active camera object
  // struct s_camera *camera;
  // a list of all entities to be updated (searched recursively) -- includes
  // cameras
  list_t entities;

  mutex_t lock;
} scene_t;

scene_t *scene_new(allocator_t *alloc);
void scene_destroy(scene_t *scene);

S_INLINE void scene_lock(scene_t *scene)
{
  mutex_lock(&scene->lock);
}

S_INLINE void scene_unlock(scene_t *scene)
{
  mutex_lock(&scene->lock);
}

S_INLINE void scene_trylock(scene_t *scene)
{
  mutex_lock(&scene->lock);
}

// Clears a scene of its entities (destroys them)
void scene_clear(scene_t *scene);
// Tells all entities to do their update routines
void scene_update(scene_t *scene);
// Tells all entities to do their drawing routines
void scene_draw(scene_t *scene);

struct s_entity *scene_new_entity(scene_t *scene, const char *name, struct s_entity *parent);
/*
// FIXME: Camera not implemented, uncomment when it is.
struct s_camera *scene_new_camera(scene_t *scene, const char *name, struct s_entity *parent);
*/

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* end __SNOW_SCENE_H__ include guard */
