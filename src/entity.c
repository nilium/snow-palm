/*
  Game entity class
  Written by Noel Cower

  See LICENSE.md for license information
*/

#include "entity.h"

#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

static void entity_invalidate_transform(entity_t *self, bool invalid_local, bool invalid_world);
static void entity_build_matrices(entity_t *self);

static inline void entity_unset_flag(entity_t *self, entity_flag_t flag)
{
  self->prv_iflags &= ~flag;
}

static inline void entity_set_flag(entity_t *self, entity_flag_t flag)
{
  self->prv_iflags |= flag;
}

static inline void entity_toggle_flag(entity_t *self, entity_flag_t flag)
{
  self->prv_iflags ^= flag;
}

static inline bool entity_is_flag_set(entity_t *self, entity_flag_t flag)
{
  return ((self->prv_iflags & flag) == flag);
}

static list_t g_entities;
static allocator_t *g_entity_alloc = NULL;

void sys_entity_init(allocator_t *alloc)
{
  if (!alloc)
    alloc = g_default_allocator;

  list_init(&g_entities, alloc);
}

void sys_entity_shutdown(void)
{
  listnode_t *node;

  while ((node = list_first_node(&g_entities)))
    entity_destroy((entity_t *) node->pointer);

  list_destroy(&g_entities);
}

void entity_destroy(entity_t *self)
{
  if (self->parent) {
    entity_remove_from_parent(self);
  }
  list_remove(self->parentnode);

  listnode_t *node = list_first_node(&self->children);
  while (node) {
    entity_t *child = (entity_t *)node->pointer;
    node = listnode_next(node);
    entity_remove_from_parent(child);
  }

  list_destroy(&self->children);
}

entity_t *entity_new(const char *name, entity_t *parent)
{
  entity_t *self = com_malloc(g_entity_alloc, sizeof(*self));

  if (self) {
    vec3_copy(g_vec3_zero, self->position);
    vec3_copy(g_vec3_one, self->scale);
    quat_identity(self->rotation);
    mat4_identity(self->transform);

    list_init(&self->children, g_entity_alloc);

    self->parent = NULL;
    self->parentnode = list_append(&g_entities, self);

    if (name != NULL)
      entity_set_name(self, name);

    if (parent)
      entity_add_child(parent, self);
  }

  return self;
}

void entity_add_child(entity_t *self, entity_t *child)
{
  if (child->parent != NULL) {
    s_log_error("Attempting to add an entity as a child it already has a parent.\n");
    return;
  }
  list_remove(child->parentnode);
  child->parentnode = list_append(&self->children, child);
  child->parent = self;
}

void entity_remove_from_parent(entity_t *self)
{
  if (self->parent) {
    list_remove(self->parentnode);
    self->parentnode = list_append(&g_entities, self);
    self->parent = NULL;
  } else {
    s_log_error("Attempting to remove entity from a parent when it has no parent.\n");
  }
}

void entity_set_name(entity_t *self, const char *name)
{
  if (name) {
    strncpy(self->name, name, ENTITY_NAME_MAX_LEN);
  } else {
    memset(self->name, 0, ENTITY_NAME_MAX_LEN);
  }
}

const char *entity_get_name(const entity_t *self)
{
  return self->name;
}

/* tform mutators */

void entity_position(entity_t *self, s_float_t x, s_float_t y, s_float_t z)
{
  self->position[0] = x;
  self->position[1] = y;
  self->position[2] = z;
  entity_invalidate_transform(self, true, true);
}

void entity_move(entity_t *self, s_float_t x, s_float_t y, s_float_t z)
{
  entity_build_matrices(self);
  vec3_t movement = {x, y, z};
  quat_multiply_vec3(self->rotation, movement, movement);
  vec3_add(movement, self->position, self->position);
  mat4_translate(x, y, z, self->transform, self->transform);
  entity_invalidate_transform(self, true, true);
}

void entity_translate(entity_t *self, s_float_t x, s_float_t y, s_float_t z)
{
  self->position[0] += x;
  self->position[1] += y;
  self->position[2] += z;
  entity_invalidate_transform(self, true, true);
}

void entity_rotate(entity_t *self, quat_t rot)
{
  quat_copy(rot, self->rotation);
  entity_invalidate_transform(self, true, true);
}

void entity_turn(entity_t *self, quat_t rot)
{
  quat_multiply(rot, self->rotation, self->rotation);
  entity_invalidate_transform(self, true, true);
}

void entity_scale(entity_t *self, s_float_t x, s_float_t y, s_float_t z)
{
  self->scale[0] = x;
  self->scale[1] = y;
  self->scale[2] = z;
  entity_invalidate_transform(self, true, true);
}

/* tform getters */

void entity_get_transform(entity_t *self, mat4_t out)
{
  entity_build_matrices(self);
  mat4_copy(self->transform, out);
}

void entity_get_world_transform(entity_t *self, mat4_t out)
{
  entity_build_matrices(self);
  mat4_copy(self->world_transform, out);
}

void entity_get_scale(const entity_t *self, s_float_t *x, s_float_t *y, s_float_t *z)
{
  if (x) *x = self->scale[0];
  if (y) *y = self->scale[1];
  if (z) *z = self->scale[2];
}

void entity_get_rotation(const entity_t *self, quat_t out)
{
  quat_copy(self->rotation, out);
}

void entity_get_position(const entity_t *self, s_float_t *x, s_float_t *y, s_float_t *z)
{
  if (x) *x = self->position[0];
  if (y) *y = self->position[1];
  if (z) *z = self->position[2];
}


/** private API of sorts */

static void entity_invalidate_transform(entity_t *self, bool invalid_local, bool invalid_world)
{
  if (entity_is_flag_set(self, DIRTY_TRANSFORM | DIRTY_WORLD))
    return;

  entity_flag_t flag = 0;
  if (invalid_local) flag |= DIRTY_TRANSFORM | DIRTY_WORLD;
  if (invalid_world) flag |= DIRTY_WORLD;
  if (!flag) return;

  entity_set_flag(self, flag);
  
  listnode_t *node = list_first_node(&self->children);
  while (node) {
    entity_invalidate_transform((entity_t *)node->pointer, false, invalid_local);
    node = listnode_next(node);
  }
}

static void entity_build_matrices(entity_t *self)
{
  mat4_t build;
  /* rebuild local transform */
  if (entity_is_flag_set(self, DIRTY_TRANSFORM)) {
    mat4_from_quat(self->rotation, build);
    mat4_scale(build, vec3_splat(self->scale), build);
    mat4_translate(vec3_splat(self->position), build, build);
    mat4_copy(build, self->transform);
    goto rebuild_world_transform;
  }
  /* rebuild world transform */
  if (entity_is_flag_set(self, DIRTY_WORLD)) {
rebuild_world_transform:
    if (self->parent) {
      entity_get_world_transform(self, build);
      mat4_multiply(build, self->transform, self->world_transform);
    } else {
      mat4_copy(self->transform, self->world_transform);
    }
  }
  entity_unset_flag(self, DIRTY_FLAGS);
}

#if defined(__cplusplus)
}
#endif /* __cplusplus */

