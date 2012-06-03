/*
  Game entity class
  Written by Noel Cower

  See LICENSE.md for license information
*/

#ifndef ENTITY_H

#define ENTITY_H

#include <snow-config.h>
#include "math3d.h"
#include "list.h"
#include "object.h"

#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

typedef enum
{
  DIRTY_TRANSFORM   =0x1<<0,  /*! Transformation matrix is dirty. */
  DIRTY_WORLD     =0x1<<1,  /*! World transform is dirty. */
  /*! Combination of all DIRTY_ flags. */
  DIRTY_FLAGS     =(DIRTY_TRANSFORM|DIRTY_WORLD),

  ROOT_ENTITY     =0x1<<2,  /*! Entity is a root entity. */
  
  ENTITY_DISABLED   =0x1<<3,  /*! Entity is currently disabled. */
  ENTITY_HIDDEN   =0x1<<4,  /*! Entity is currently hidden. */
} entity_flag_t;

typedef struct s_entity entity_t;

struct s_entity
{
  class_t *isa;

  list_t *components;

  list_t *children;
  listnode_t *parentnode;
  entity_t *parent;

  char *name;
  
  /*! internal flags */
  entity_flag_t prv_iflags;

  mat4_t world_transform;
  mat4_t transform;
  quat_t rotation;
  vec3_t position;
  vec3_t scale;
};

extern const class_t g_entity_class;
#define entity_class (&g_entity_class)

/** The entity system depends on the object and memory systems. */
void sys_entity_init(void);
void sys_entity_shutdown(void);

/*! Allocates a new instance of the given entity class.
  \returns A new instance of the given entity class, or NULL if the class
  doesn't inherit from ::entity_class.
*/
entity_t *entity_new(class_t *cls);
/*! Initializes the given entity. */
entity_t *entity_init(entity_t *self, const char *name, entity_t *parent);

/*! Adds a child to the entity. */
void entity_add_child(entity_t *self, entity_t *child);
/*! Removes the entity from its parent entity. */
void entity_remove_from_parent(entity_t *self);

/*! Set the entity's name to a copy of the name string provided, if any. */
void entity_set_name(entity_t *self, const char *name);
/*! Get the entity's name.  This is not a copy of the name string, so you
    must not free it.
*/
const char *entity_get_name(const entity_t *self);

/*! \brief Sets the position of the entity relative to its parent. */
void entity_position(entity_t *self, float x, float y, float z);
/*! \brief Moves the entity relative to its orientation and parent. */
void entity_move(entity_t *self, float x, float y, float z);
/*! \brief Translates the entity relative to its parent (ignores orientation). */
void entity_translate(entity_t *self, float x, float y, float z);

/*! \brief Sets the rotation of the entity (relative to its parent). */
void entity_rotate(entity_t *self, quat_t rot);
/*! \brief Rotates the entity by pitch, yaw, and roll (relative to its parent) - this concatenates rotations. */
void entity_turn(entity_t *self, quat_t rot);

/*! \brief Sets the scale of the entity.  Has no direct effect on entity behavior,
    but may modify component behavior.
*/
void entity_scale(entity_t *self, float x, float y, float z);

/*! Gets the entity's transformation matrix. */
void entity_get_transform(entity_t *self, mat4_t out);
/*! Gets the entity's world transformation matrix. */
void entity_get_world_transform(entity_t *self, mat4_t out);
/*! Gets the entity's scale (relative to its parent). */
void entity_get_scale(entity_t *self, float *x, float *y, float *z);
/*! Gets the entity's rotation (relative to its parent). */
void entity_get_rotation(entity_t *self, quat_t out);
/*! Gets the entity's position (relative to its parent). */
void entity_get_position(entity_t *self, float *x, float *y, float *z);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* end of include guard: ENTITY_H */

