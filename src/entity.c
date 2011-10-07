#include "entity.h"

#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

#define ENTITY_POOL_SIZE ((1024 * sizeof(struct s_entity)))
#define ENTITY_POOL_TAG (0x0C0DFEED)

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

void entity_new(const char *name, entity_t *entity)
{
	entity_t self = (entity_t)mem_alloc(entity_memory_pool, sizeof(struct s_entity), ENTITY_POOL_TAG);

	vec3_copy(g_vec3_zero, self->position);
	vec3_copy(g_vec3_one, self->scale);
	quat_identity(self->rotation);
	mat4_identity(self->transform);
}

void entity_destroy(entity_t entity)
{
}

void entity_addChild(entity_t parent, entity_t child)
{
}

void entity_removeFromParent(entity_t child)
{
}

const char *entity_name(const entity_t entity)
{
	return NULL;
}

void entity_move(entity_t entity, float x, float y, float z)
{
}

void entity_translate(entity_t entity, float x, float y, float z)
{
}

void entity_rotate(entity_t entity, float x, float y, float z)
{
}

void entity_getRotation(entity_t entity, float *x, float *y, float *z)
{
}

void entity_turn(entity_t entity, float x, float y, float z)
{
	vec3_t out;
	entity_getScale(entity, vec3_expandAddr(out));
}

void entity_scale(entity_t entity, float x, float y, float z)
{
}

void entity_getScale(entity_t entity, float *x, float *y, float *z)
{
}


#if defined(__cplusplus)
}
#endif /* __cplusplus */

