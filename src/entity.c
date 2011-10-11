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

#define ENTITY_POOL_SIZE ((4096 * sizeof(listnode_t)) + (1024 * sizeof(entity_t)))
#define ENTITY_POOL_TAG (0x0C0DFEED)

static entity_t *entity_ctor(entity_t *self, memory_pool_t *pool);
static void entity_dtor(entity_t *self);
static void entity_invalidate_transform(entity_t *self, bool invalid_local, bool invalid_world);
static void entity_build_matrices(entity_t *self);

static inline void entity_unset_flag(entity_t *self, entity_flag_t flag)
{
	self->_iflags &= ~flag;
}

static inline void entity_set_flag(entity_t *self, entity_flag_t flag)
{
	self->_iflags |= flag;
}

static inline void entity_toggle_flag(entity_t *self, entity_flag_t flag)
{
	self->_iflags ^= flag;
}

static inline bool entity_is_flag_set(entity_t *self, entity_flag_t flag)
{
	return ((self->_iflags & flag) == flag);
}

const class_t _entity_class = {
	object_class,
	sizeof(entity_t),

	(constructor_t)entity_ctor,
	(destructor_t)entity_dtor,

	object_compare,
};

memory_pool_t _g_entity_pool;
#define g_entity_pool (&_g_entity_pool)

static list_t *g_entities;

void sys_entity_init(void)
{
	mem_init_pool(g_entity_pool, ENTITY_POOL_SIZE, ENTITY_POOL_TAG);
	g_entities = list_init(object_new(list_class, g_entity_pool), true);
}

void sys_entity_shutdown(void)
{
	object_release(g_entities);
	mem_release_pool(g_entity_pool);
}

static entity_t *entity_ctor(entity_t *self, memory_pool_t *pool)
{
	self = (entity_t *)self->isa->super->ctor((object_t *)self, pool);
	if (self) {
		vec3_copy(g_vec3_zero, self->position);
		vec3_copy(g_vec3_one, self->scale);
		quat_identity(self->rotation);
		mat4_identity(self->transform);

		self->children = list_init(object_new(list_class, g_entity_pool), false);
		self->components = list_init(object_new(list_class, g_entity_pool), false);

		self->parent = NULL;
		self->name = NULL;

		self->parentnode = list_append(&g_entities, self);
	}
	return self;
}

static void entity_dtor(entity_t *self)
{
	if (self->parent) {
		entity_remove_from_parent(self);
	}
	list_remove(self->parentnode);

	listnode_t *node = list_first_node(self->children);
	while (node) {
		entity_t *child = (entity_t *)node->obj;
		node = listnode_next(node);
		entity_remove_from_parent(child);
	}

	if (self->name)
		mem_free(self->name);

	object_release(self->children);
	object_release(self->components);

	self->isa->super->dtor((object_t *)self);
}

entity_t *entity_new(class_t *cls)
{
	if (!class_is_kind_of(cls, entity_class))
		return NULL;

	entity_t *self = object_new(entity_class, g_entity_pool);
}

entity_t *entity_init(entity_t *self, const char *name, entity_t *parent)
{
	if (name != NULL)
		entity_set_name(self, name);

	if (parent)
		entity_add_child(parent, self);

	return self;
}

void entity_add_child(entity_t *self, entity_t *child)
{
	if (child->parent != NULL) {
		log_error("Attempting to add an entity as a child it already has a parent.\n");
		return;
	}
	list_remove(child->parentnode);
	child->parentnode = list_append(self->children, child);
    child->parent = self;
}

void entity_remove_from_parent(entity_t *self)
{
	if (self->parent) {
		list_remove(self->parentnode);
		self->parentnode = list_append(&g_entities, self);
		self->parent = NULL;
	} else {
		log_error("Attempting to remove entity from a parent when it has no parent.\n");
	}
}

void entity_set_name(entity_t *self, const char *name)
{
	if (self->name)
		mem_free(self->name);

	if (name) {
		size_t len = strlen(name);
		self->name = mem_alloc(NULL, len + 1, ENTITY_POOL_TAG);
		memcpy(self->name, name, len + 1);
	} else {
		self->name = NULL;
	}
}

const char *entity_get_name(const entity_t *self)
{
	return self->name;
}

/* tform mutators */

void entity_position(entity_t *self, float x, float y, float z)
{
	self->position[0] = x;
	entity_invalidate_transform(self, true, true);
}

void entity_move(entity_t *self, float x, float y, float z)
{
	entity_build_matrices(self);
	vec3_t movement = {x, y, z};
	quat_multiply_vec3(self->rotation, movement, movement);
	vec3_add(movement, self->position, self->position);
	mat4_translate(x, y, z, self->transform);
//	entity_invalidate_transform(self, true, true);
}

void entity_translate(entity_t *self, float x, float y, float z)
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

void entity_scale(entity_t *self, float x, float y, float z)
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

void entity_get_scale(entity_t *self, float *x, float *y, float *z)
{
	if (x) *x = self->scale[0];
	if (y) *y = self->scale[1];
	if (z) *z = self->scale[2];
}

void entity_get_rotation(entity_t *self, quat_t out)
{
	quat_copy(self->rotation, out);
}

void entity_get_position(entity_t *self, float *x, float *y, float *z)
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
	
	listnode_t *node = list_first_node(self->children);
	while (node) {
		entity_invalidate_transform((entity_t *)node->obj, false, invalid_local);
		node = listnode_next(node);
	}
}

static void entity_build_matrices(entity_t *self)
{
	mat4_t build;
	/* rebuild local transform */
	if (entity_is_flag_set(self, DIRTY_TRANSFORM)) {
		mat4_from_quat(self->rotation, build);
		mat4_scale(build, vec3_expand(self->scale), build);
		mat4_translate(vec3_expand(self->position), build);
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

