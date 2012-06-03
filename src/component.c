/*
	Base entity component
	Written by Noel Cower

	See LICENSE.md for license information
*/

#include "component.h"

#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

memory_pool_t gv_component_pool;
static component_t *component_ctor(component_t *self, memory_pool_t *pool);
static void component_dtor(component_t *self);
static void component_nop(component_t *self) {}

const component_class_t g_component_class = {
	{
		object_class,
		sizeof(component_t),

		(constructor_t)component_ctor,
		(destructor_t)component_dtor,
		object_compare
	},

	component_nop,
	component_nop,
	component_nop
};

static component_t *component_ctor(component_t *self, memory_pool_t *pool)
{
	return self->isa->head.super->ctor(self, pool);
}

static void component_dtor(component_t *self)
{
	self->isa->head.super->dtor(self);
}

#if defined(__cplusplus)
}
#endif /* __cplusplus */

