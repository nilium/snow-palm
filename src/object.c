#include "object.h"
#include "map.h"
#include "mutex.h"

#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

/* The basic object constructor. */
static object_t *object_ctor(object_t *self);
/* The basic object destructor. */
static void object_dtor(object_t *self);

const class_t object_class = {
	NULL,
	sizeof(object_t),

	object_ctor,
	object_dtor,

	object_compare,
};

static map_t g_retain_map; /* kv-pair: (object address, (void *)-sized integer) */
static mutex_t g_retain_lock;

void sys_object_init(void)
{
	mutex_init(&g_retain_lock, 1);
	map_init(&g_retain_map, g_mapops_default, NULL);
}

void sys_object_shutdown(void)
{
	map_destroy(&g_retain_map);
	mutex_destroy(&g_retain_lock);
}

static object_t *object_ctor(object_t *self)
{
	/* retain count 1, NOP */
	return self;
}

static void object_dtor(object_t *self)
{
	/* TODO: weak reference table, signal object deletion */
}

int object_compare(object_t *self, object_t *other)
{
	ptrdiff_t diff = self - other;
	if (diff < 0)
		return -1;
	else if (diff > 0)
		return 1;
	else
		return 0;
}

object_t *object_retain(object_t *self)
{
	uint32_t *retainCount;

	mutex_lock(&g_retain_lock);

	retainCount = (uint32_t *)map_getAddr(&g_retain_map, self);
	if (retainCount) {
		*retainCount += 1;
	} else {
		map_insert(&g_retain_map, self, (void *)((uint32_t)2));
	}

	mutex_unlock(&g_retain_lock);
	return self;
}

object_t *object_autorelease(object_t *self)
{
	return self;
}

void object_release(object_t *self)
{
	uint32_t *retainCount;

	mutex_lock(&g_retain_lock);

	retainCount = map_getAddr(&g_retain_map, self);
	if (retainCount) {
		int count = (*retainCount) - 1;
		
		if (count == 1)
			map_remove(&g_retain_map, self);
		else
			*retainCount = count;
	}

	mutex_unlock(&g_retain_lock);

	/* if no entry in the map is found, the object's retain count
	   was 1, and is now 0, so delete it
	*/
	if (!retainCount) object_delete(self);
}

int32_t object_retainCount(object_t *self)
{
	uint32_t retainCount;

	mutex_lock(&g_retain_lock);

	retainCount = (uint32_t)map_get(&g_retain_map, self);
	if (retainCount == (uint32_t)NULL) retainCount = 1;

	mutex_unlock(&g_retain_lock);

	return retainCount;
}

object_t *object_new(const class_t *cls, memory_pool_t *pool)
{
	/* allocate the object from the given memory pool */
	object_t *self = (object_t *)mem_alloc(pool, cls->size, (int32_t)(cls));
	
	/* make sure we've actually allocated the memory - if we haven't, return NULL */
	if (!self)
		return NULL;
	
	/* zero the memory used by the object */
	memset(self, 0, cls->size);
	
	/* assign the given class (cls) to the object's `isa` pointer */
	self->isa = cls;
	/* construct/initialize the object (see note on constructors in object.h) */
	self = cls->ctor(self);
	
	/* return the constructed object */
	return self;
}

void object_delete(object_t *object)
{
	/* call the class destructor (which will, in turn, call superclass destructors
	   if written correctly
	*/
	object->isa->dtor(object);
	memset(object, 0, object->isa->size);
	mem_free(object);
}

#if defined(__cplusplus)
}
#endif /* __cplusplus */

