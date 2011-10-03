#include "object.h"
#include "map.h"

#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

const class_t object_class = {
	NULL,
	sizeof(struct s_object),

	object_ctor,
	object_dtor,

	object_compare,

	object_retain,
	object_release,
	object_autorelease,
	object_retainCount
};

object_t object_ctor(object_t self)
{
	/* retain count 1, NOP */
	return self;
}

void object_dtor(object_t self)
{
}

int object_compare(object_t self, object_t other)
{
	return -1;
}

object_t object_retain(object_t self)
{
	return self;
}

object_t object_autorelease(object_t self)
{
	return self;
}

void object_release(object_t self)
{
}

int32_t object_retainCount(object_t self)
{
	return 1;
}

object_t object_new(class_t *cls, memory_pool_t *pool)
{
	/* allocate the object from the given memory pool */
	object_t self = (object_t)mem_alloc(pool, cls->size, (int32_t)(cls));
	
	/* make sure we've actually allocated the memory - if we haven't, return NULL */
	if (!self)
		return NULL;
	
	/* zero the memory used by the object */
	memset(self, 0, cls->size);
	
	/* assign the given class (cls) to the object's `isa` pointer */
	self->isa = cls;
	/* construct/initialize the object (see note on constructors in object.h) */
	self = cls->construct(self);
	
	/* return the constructed object */
	return self;
}

void object_delete(object_t object)
{
	/* call the class destructor (which will, in turn, call superclass destructors
	   if written correctly
	*/
	object->isa->destruct(object);
	memset(object, 0, object->isa->size);
	mem_free(object);
}

#if defined(__cplusplus)
}
#endif /* __cplusplus */

