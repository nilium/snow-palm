#include "entity.h"

#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

#define ENTITY_POOL_SIZE ((1024 * sizeof(struct s_entity)))
#define ENTITY_POOL_TAG (0x0C0DFEED)

static entity_t *entity_ctor(entity_t *self);
static void entity_dtor(entity_t *self);

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
	list_init(&g_entities, &g_entity_pool);
}

void sys_entity_shutdown(void)
{
	list_destroy(&g_entities);
	mem_destroy_pool(&g_entity_pool);
}

static entity_t *entity_ctor(entity_t *self)
{
	self = (entity_t *)self->isa->super->ctor((object_t *)self);
	if (self) {
		vec3_copy(g_vec3_zero, self->position);
		vec3_copy(g_vec3_one, self->scale);
		quat_identity(self->rotation);
		mat4_identity(self->transform);

		list_init(&self->children, &g_entity_pool);
		list_init(&self->components, &g_entity_pool);

		self->name = NULL;
	}
	return self;
}

static void entity_dtor(entity_t *self)
{
	listnode_t *node = list_firstNode(&self->children);
	while (node) {
		entity_t *child = (entity_t *)node->obj;
		child->parent = NULL;
		node = listnode_next(node);
	}

	if (self->name)
		mem_free(self->name);

	list_destroy(&self->children);
	list_destroy(&self->components);
}

entity_t *entity_new(const char *name)
{
	entity_t *self = (entity_t *)object_new(&entity_class, &g_entity_pool);

	if (name != NULL) {
		entity_setName(self, name);
	}

	return self;
}

void entity_addChild(entity_t *parent, entity_t *child)
{
}

void entity_removeFromParent(entity_t *child)
{
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
	return NULL;
}

void entity_move(entity_t *self, float x, float y, float z)
{
}

void entity_translate(entity_t *self, float x, float y, float z)
{
}

void entity_rotate(entity_t *self, float x, float y, float z)
{
}

void entity_getRotation(entity_t *self, float *x, float *y, float *z)
{
}

void entity_turn(entity_t *self, float x, float y, float z)
{
}

void entity_scale(entity_t *self, float x, float y, float z)
{
}

void entity_getScale(entity_t *self, float *x, float *y, float *z)
{
}


#if defined(__cplusplus)
}
#endif /* __cplusplus */

