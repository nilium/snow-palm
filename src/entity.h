#ifndef ENTITY_H

#define ENTITY_H

#include "config.h"
#include "math3d.h"
#include "list.h"
#include "object.h"

#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

typedef enum
{
	DIRTY_TRANSFORM		=0x1<<0,	/*! Transformation matrix is dirty. */
	DIRTY_POSITION		=0x1<<1,	/*! Position is dirty. */
	DIRTY_ROTATION		=0x1<<2, 	/*! Rotation is dirty. */
	DIRTY_SCALE			=0x1<<3, 	/*! Scale is dirty. */
	DIRTY_WORLD			=0x1<<4, 	/*! World transform is dirty. */
	/*! Combination of all DIRTY_ flags. */
	DIRTY_FLAGS			=(DIRTY_TRANSFORM|DIRTY_POSITION|DIRTY_ROTATION|DIRTY_SCALE|DIRTY_WORLD),

	ROOT_ENTITY			=0x1<<5,	/*! Entity is a root entity. */
	
	ENTITY_DISABLED		=0x1<<6,	/*! Entity is currently disabled. */
	ENTITY_HIDDEN		=0x1<<7,	/*! Entity is currently hidden. */
} entity_flags_t;

typedef struct s_entity *entity_t;

struct s_entity
{
	struct s_object object_head;

	list_t children;
	list_t components;
	entity_t parent;
	char *name;
	/*! internal flags */
	entity_flags_t _iflags;
	mat4_t transform;
	mat4_t _world_transform;
	quat_t rotation;
	vec3_t position, scale;
};



void sys_entity_init(void);
void sys_entity_shutdown(void);

void entity_new(const char *name, entity_t *entity);
void entity_destroy(entity_t entity);

void entity_addChild(entity_t parent, entity_t child);
void entity_removeFromParent(entity_t child);

const char *entity_name(const entity_t entity);

void entity_move(entity_t entity, float x, float y, float z);
void entity_translate(entity_t entity, float x, float y, float z);
void entity_rotate(entity_t entity, float x, float y, float z);
void entity_turn(entity_t entity, float x, float y, float z);
void entity_scale(entity_t entity, float x, float y, float z);
void entity_getScale(entity_t entity, float *x, float *y, float *z);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* end of include guard: ENTITY_H */

