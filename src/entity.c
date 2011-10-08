#include "entity.h"

#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

#define ENTITY_POOL_SIZE ((1024 * sizeof(struct s_entity)))
#define ENTITY_POOL_TAG (0x0C0DFEED)

static entity_t *entity_ctor(entity_t *self);
static void entity_dtor(entity_t *self);
static void entity_invalidateTransform(entity_t *self, bool invalidLocal, bool invalidWorld);
static void entity_buildMatrices(entity_t *self);

static inline void entity_unsetFlag(entity_t *self, entity_flag_t flag)
{
	self->_iflags &= ~flag;
}

static inline void entity_setFlag(entity_t *self, entity_flag_t flag)
{
	self->_iflags |= flag;
}

static inline void entity_toggleFlag(entity_t *self, entity_flag_t flag)
{
	self->_iflags ^= flag;
}

static inline bool entity_isFlagSet(entity_t *self, entity_flag_t flag)
{
	return ((self->_iflags & flag) == flag);
}

const class_t entity_class = {
	&object_class,
	sizeof(entity_t),

	(constructor_t)entity_ctor,
	(destructor_t)entity_dtor,

	object_compare,
};

memory_pool_t g_entity_pool;
memory_pool_t *const entity_memory_pool = &g_entity_pool;

list_t g_entities;

void sys_entity_init(void)
{
	mem_init_pool(&g_entity_pool, ENTITY_POOL_SIZE, ENTITY_POOL_TAG);
	list_init(&g_entities, &g_entity_pool, true);
}

void sys_entity_shutdown(void)
{
	list_destroy(&g_entities);
	mem_release_pool(&g_entity_pool);
}

static entity_t *entity_ctor(entity_t *self)
{
	self = (entity_t *)self->isa->super->ctor((object_t *)self);
	if (self) {
		vec3_copy(g_vec3_zero, self->position);
		vec3_copy(g_vec3_one, self->scale);
		quat_identity(self->rotation);
		mat4_identity(self->transform);

		list_init(&self->children, &g_entity_pool, false);
		list_init(&self->components, &g_entity_pool, false);

		self->parent = NULL;
		self->name = NULL;

		self->parentnode = list_append(&g_entities, self);
	}
	return self;
}

static void entity_dtor(entity_t *self)
{
	if (self->parent) {
		entity_removeFromParent(self);
	}
	list_remove(self->parentnode);

	listnode_t *node = list_firstNode(&self->children);
	while (node) {
		entity_t *child = (entity_t *)node->obj;
		node = listnode_next(node);
		entity_removeFromParent(child);
	}

	if (self->name)
		mem_free(self->name);

	list_destroy(&self->children);
	list_destroy(&self->components);
}

entity_t *entity_new(const char *name, entity_t *parent)
{
	entity_t *self = (entity_t *)object_new(&entity_class, &g_entity_pool);

	if (name != NULL) {
		entity_setName(self, name);
	}

	if (parent)
		entity_addChild(parent, self);

	return self;
}

void entity_addChild(entity_t *self, entity_t *child)
{
	if (child->parent != NULL) {
		log_error("Attempting to add an entity as a child it already has a parent.\n");
		return;
	}
	list_remove(child->parentnode);
	child->parentnode = list_append(&self->children, child);
    child->parent = self;
}

void entity_removeFromParent(entity_t *self)
{
	if (self->parent) {
		list_remove(self->parentnode);
		self->parentnode = list_append(&g_entities, self);
		self->parent = NULL;
	} else {
		log_error("Attempting to remove entity from a parent when it has no parent.\n");
	}
}

void entity_setName(entity_t *self, const char *name)
{
	if (self->name)
		mem_free(self->name);
	
	if (name) {
		size_t len = strlen(name);
		self->name = mem_alloc(&g_entity_pool, len + 1, ENTITY_POOL_TAG);
		memcpy(self->name, name, len + 1);
	} else {
		self->name = NULL;
	}
}

const char *entity_getName(const entity_t *self)
{
	return self->name;
}

/* tform mutators */

void entity_position(entity_t *self, float x, float y, float z)
{
	self->position[0] = x;
	entity_invalidateTransform(self, true, true);
}

void entity_move(entity_t *self, float x, float y, float z)
{
	entity_buildMatrices(self);
	vec3_t movement = {x, y, z};
	quat_multiplyVec3(self->rotation, movement, movement);
	vec3_add(movement, self->position, self->position);
	mat4_translate(x, y, z, self->transform);
//	entity_invalidateTransform(self, true, true);
}

void entity_translate(entity_t *self, float x, float y, float z)
{
	self->position[0] += x;
	self->position[1] += y;
	self->position[2] += z;
	entity_invalidateTransform(self, true, true);
}

void entity_rotate(entity_t *self, quat_t rot)
{
	quat_copy(rot, self->rotation);
	entity_invalidateTransform(self, true, true);
}

void entity_turn(entity_t *self, quat_t rot)
{
	quat_multiply(rot, self->rotation, self->rotation);
	entity_invalidateTransform(self, true, true);
}

void entity_scale(entity_t *self, float x, float y, float z)
{
	self->scale[0] = x;
	self->scale[1] = y;
	self->scale[2] = z;
	entity_invalidateTransform(self, true, true);
}

/* tform getters */

void entity_getTransform(entity_t *self, mat4_t out)
{
	entity_buildMatrices(self);
	mat4_copy(self->transform, out);
}

void entity_getWorldTransform(entity_t *self, mat4_t out)
{
	entity_buildMatrices(self);
	mat4_copy(self->worldTransform, out);
}

void entity_getScale(entity_t *self, float *x, float *y, float *z)
{
	if (x) *x = self->scale[0];
	if (y) *y = self->scale[1];
	if (z) *z = self->scale[2];
}

void entity_getRotation(entity_t *self, quat_t out)
{
	quat_copy(self->rotation, out);
}

void entity_getPosition(entity_t *self, float *x, float *y, float *z)
{
	if (x) *x = self->position[0];
	if (y) *y = self->position[1];
	if (z) *z = self->position[2];
}


/** private API of sorts */

static void entity_invalidateTransform(entity_t *self, bool invalidLocal, bool invalidWorld)
{
	if (entity_isFlagSet(self, DIRTY_TRANSFORM | DIRTY_WORLD))
		return;
	
	entity_flag_t flag = 0;
	if (invalidLocal) flag |= DIRTY_TRANSFORM | DIRTY_WORLD;
	if (invalidWorld) flag |= DIRTY_WORLD;
	if (!flag) return;

	entity_setFlag(self, flag);
	
	listnode_t *node = list_firstNode(&self->children);
	while (node) {
		entity_invalidateTransform((entity_t *)node->obj, false, invalidLocal);
		node = listnode_next(node);
	}
}

static void entity_buildMatrices(entity_t *self)
{
	mat4_t build;
	if (entity_isFlagSet(self, DIRTY_TRANSFORM)) {
		mat4_fromQuat(self->rotation, build);
		mat4_scale(build, vec3_expand(self->scale), build);
		mat4_translate(vec3_expand(self->position), build);
		mat4_copy(build, self->transform);
	}
	if (entity_isFlagSet(self, DIRTY_WORLD)) {
		if (self->parent) {
			entity_getWorldTransform(self, build);
			mat4_multiply(build, self->transform, self->worldTransform);
		} else {
			mat4_copy(self->transform, self->worldTransform);
		}
	}
	entity_unsetFlag(self, DIRTY_FLAGS);
}

#if defined(__cplusplus)
}
#endif /* __cplusplus */

