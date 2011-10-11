/*
	Object system / class
	Written by Noel Cower

	See LICENSE.md for license information
*/

#include "object.h"
#include "map.h"
#include "mutex.h"
#include "autoreleasepool.h"
#include "list.h"

#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

/* The basic object constructor. */
static object_t *object_ctor(object_t *self, memory_pool_t *pool);
/* The basic object destructor. */
static void object_dtor(object_t *self);

const class_t _object_class = {
	NULL,
	sizeof(object_t),

	object_ctor,
	object_dtor,

	object_compare,
};

#define RETAIN_POOL_SIZE (16384 * sizeof(void *))
#define WEAKREF_POOL_SIZE (16384* sizeof(listnode_t))
#define RETAIN_POOL_TAG 0x00F8800A
#define WEAKREF_POOL_TAG 0x00F8800B

static memory_pool_t g_retain_pool;
static map_t g_retain_map; /* kv-pair: (object address, (void *)-sized integer) */
static mutex_t g_retain_lock;

static memory_pool_t g_refpool;
static map_t g_refmap; /* kv-pair: (object address, (list_t of void **)) */
static map_t g_refvaluemap; /* kv-pair: (store address, object address) */
static mutex_t g_reflock;

/* mapops to retain/release the lists assigned to them as values */
static void refval_copy(void *value);
static void refval_destroy(void *value);

static mapops_t g_refops = {
	NULL,
	NULL,
	NULL,
	refval_copy,
	refval_destroy
};

static void refval_copy(void* value)
{
	if (value != NULL)
		object_retain(value);
}

static void refval_destroy(void *value)
{
	if (value != NULL)
		object_release(value);
}

void sys_object_init(void)
{
	mem_init_pool(&g_retain_pool, RETAIN_POOL_SIZE, RETAIN_POOL_TAG);
	mem_init_pool(&g_refpool, WEAKREF_POOL_SIZE, WEAKREF_POOL_TAG);

	mutex_init(&g_retain_lock, 1);
	mutex_init(&g_reflock, 0);

	map_init(&g_retain_map, g_mapops_default, &g_retain_pool);
	map_init(&g_refmap, g_refops, &g_refpool);
	map_init(&g_refvaluemap, g_mapops_default, &g_refpool);
}

void sys_object_shutdown(void)
{
	map_destroy(&g_retain_map);
	mutex_destroy(&g_retain_lock);
}

static object_t *object_ctor(object_t *self, memory_pool_t *pool)
{
	(void)pool;
	/* retain count 1, NOP */
	return self;
}

static void object_dtor(object_t *self)
{
	object_zeroWeak(self);
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
	autoreleasepool_addObject(self);
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
	self = cls->ctor(self, pool);
	
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

void object_storeWeak(void *value, void **address)
{
	void **prevValue;
	
	mutex_lock(&g_reflock);

	prevValue = map_getAddr(&g_refvaluemap, (mapkey_t)address);
	if (prevValue) {
		/* already has a weak reference */
		if (*prevValue == value) return;
		list_t *list = map_get(&g_refmap, *prevValue);
		list_removeValue(list, address);

		if (value) {
			*prevValue = value;
		} else {
			map_remove(&g_refvaluemap, (mapkey_t)address);
		}
	} else if (value) {
		map_insert(&g_refvaluemap, (mapkey_t)address, value);
	}

	if (value) {
		list_t *valrefs = map_get(&g_refmap, (mapkey_t)value);
		if (!valrefs) {
			valrefs = list_init(object_new(list_class, &g_refpool), true);
			map_insert(&g_refmap, value, valrefs);
		}
		/* something of a hack since a list assumes it's storing object references */
		list_prepend(valrefs, (object_t *)address);
	}

	*address = value;

	mutex_unlock(&g_reflock);
}

void object_zeroWeak(void *value)
{
	list_t *list;

	if (!value) return;

	mutex_lock(&g_reflock);

	list = map_get(&g_refmap, value);
	if (list) {
		listnode_t *node = list_firstNode(list);
		while (node) {
			map_remove(&g_refvaluemap, (mapkey_t)node->obj);
			*((void**)node->obj) = NULL;
			node = listnode_next(node);
		}
		map_remove(&g_refmap, value);
	}

	mutex_unlock(&g_reflock);
}

bool class_isKindOf(const class_t *cls, const class_t *other)
{
	while (cls) {
		if (cls == other)
			return true;
		cls = cls->super;
	}
	return false;
}

#if defined(__cplusplus)
}
#endif /* __cplusplus */

