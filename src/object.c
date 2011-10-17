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

const class_t g_object_class = {
	NULL,
	sizeof(object_t),

	object_ctor,
	object_dtor,

	object_compare,
};

#ifndef NDEBUG
/** These globals are here mainly to track the maximum number of objects allocated
	during runtime.  This, in turn, allows me to tweak the size of the retain
	memory pool.  I should probably do the same for weak references, but the
	reality is that weak references are unlikely to be heavily used due to
	their relatively high performance cost (this could be optimized later, probably).
*/
/* max_objects tracks the peak number of objects at runtime */
static size_t max_objects = 0;
/* total_objects tracks the number of objects currently instantiated */
static size_t total_objects = 0;
#endif

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

/* mapops to retain/release the lists assigned to them as values */
static void *refval_copy(void *value);
static void refval_destroy(void *value);

static mapops_t g_refops = {
	NULL,
	NULL,
	NULL,
	refval_copy,
	refval_destroy
};

static void *refval_copy(void* value)
{
	if (value != NULL)
		object_retain((object_t *)value);
	return value;
}

static void refval_destroy(void *value)
{
	if (value != NULL)
		object_release((object_t *)value);
}

void sys_object_init(void)
{
	mem_init_pool(&g_retain_pool, RETAIN_POOL_SIZE, RETAIN_POOL_TAG);
	mem_init_pool(&g_refpool, WEAKREF_POOL_SIZE, WEAKREF_POOL_TAG);

	mutex_init(&g_retain_lock, 1);

	map_init(&g_retain_map, g_mapops_default, &g_retain_pool);
	map_init(&g_refmap, g_refops, &g_refpool);
	map_init(&g_refvaluemap, g_mapops_default, &g_refpool);
}

void sys_object_shutdown(void)
{
	map_destroy(&g_retain_map);
	map_destroy(&g_refmap);
	map_destroy(&g_refvaluemap);
	
	mutex_destroy(&g_retain_lock);

	mem_release_pool(&g_refpool);
	mem_release_pool(&g_retain_pool);
#ifndef NDEBUG
	log_note("Peak object allocation count was %zu objects\n", max_objects);
#endif
}

static object_t *object_ctor(object_t *self, memory_pool_t *pool)
{
	(void)pool;
	/* retain count 1, NOP */
	return self;
}

static void object_dtor(object_t *self)
{
	object_zero_weak(self);
#ifndef NDEBUG
	/* for the sake of tracking object allocations (to determine necessary memory use) */
	mutex_lock(&g_retain_lock);
	total_objects -= 1;
	mutex_unlock(&g_retain_lock);
#endif
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
	if (self == NULL) {
		log_warning("Attempting to retain NULL.\n");
		return NULL;
	}
	uint32_t *retain_count;

	mutex_lock(&g_retain_lock);

	retain_count = (uint32_t *)map_get_addr(&g_retain_map, (mapkey_t)self);
	if (retain_count) {
		*retain_count += 1;
	} else {
		map_insert(&g_retain_map, self, (void *)((uint32_t)2));
	}

	mutex_unlock(&g_retain_lock);
	return self;
}

object_t *object_autorelease(object_t *self)
{
	if (self == NULL) {
		log_warning("Attempting to autorelease NULL.\n");
		return NULL;
	}

	mutex_lock(&g_retain_lock);
	autoreleasepool_add_object(self);
	mutex_unlock(&g_retain_lock);

	return self;
}

void object_release(object_t *self)
{
	uint32_t *retain_count;

	if (self == NULL) {
		log_warning("Attempting to release NULL.\n");
		return;
	}

	mutex_lock(&g_retain_lock);

	retain_count = (uint32_t *)map_get_addr(&g_retain_map, (mapkey_t)self);
	if (retain_count) {
		int count = (*retain_count) - 1;
		
		if (count == 1)
			map_remove(&g_retain_map, self);
		else
			*retain_count = count;
	}

	mutex_unlock(&g_retain_lock);

	/* if no entry in the map is found, the object's retain count
	   was 1, and is now 0, so delete it
	*/
	if (!retain_count) object_delete(self);
}

int32_t object_retain_count(object_t *self)
{
	uint32_t retain_count;

	mutex_lock(&g_retain_lock);

	retain_count = (uint32_t)map_get(&g_retain_map, self);
	if (retain_count == (uint32_t)NULL) retain_count = 1;

	mutex_unlock(&g_retain_lock);

	return retain_count;
}

object_t *object_new(const class_t *cls, memory_pool_t *pool)
{
#ifndef NDEBUG
	/* for the sake of tracking object allocations (to determine necessary memory use) */
	mutex_lock(&g_retain_lock);
	total_objects += 1;
	if (total_objects > max_objects) max_objects = total_objects;
	mutex_unlock(&g_retain_lock);
#endif
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

void object_store_weak(object_t *obj, object_t **store)
{
	object_t **prev_value;
	
	mutex_lock(&g_retain_lock);

	prev_value = (object_t **)map_get_addr(&g_refvaluemap, (mapkey_t)store);
	if (prev_value) {
		/* already has a weak reference */
		if (*prev_value == obj) {
			/** in the event that the weak reference being stored is the same,
				we can just go ahead and skip doing anything.

				I swear, my use of goto is probably going to make a few people
				want to tear their eyeballs out.
			*/
			goto finish_store_weak;
		}

		list_t *list = map_get(&g_refmap, *prev_value);
		list_remove_value(list, (object_t *)store);

		if (obj) {
			*prev_value = obj;
		} else {
			map_remove(&g_refvaluemap, (mapkey_t)store);
		}
	} else if (obj) {
		map_insert(&g_refvaluemap, (mapkey_t)store, obj);
	}

	if (obj) {
		list_t *valrefs = map_get(&g_refmap, (mapkey_t)obj);
		if (!valrefs) {
			valrefs = list_init(object_new(list_class, &g_refpool), true);
			map_insert(&g_refmap, obj, valrefs);
		}
		/** This is not actually an object, but I'm relying on the fact that lists
			can store weak references, so it's all good.  Sort of.
		*/
		list_prepend(valrefs, (object_t *)store);
	}

	*store = obj;

finish_store_weak:
	mutex_unlock(&g_retain_lock);
}

object_t *object_get_weak(object_t **store)
{
	mutex_lock(&g_retain_lock);
	object_t *obj = *store;
	if (obj) object_autorelease(object_retain(obj));
	mutex_unlock(&g_retain_lock);
	return obj;
}

void object_zero_weak(object_t *obj)
{
	list_t *list;

	if (!obj) return;

	mutex_lock(&g_retain_lock);

	list = map_get(&g_refmap, obj);
	if (list) {
		listnode_t *node = list_first_node(list);
		while (node) {
			map_remove(&g_refvaluemap, (mapkey_t)node->obj);
			*((object_t **)node->obj) = NULL;
			node = listnode_next(node);
		}
		map_remove(&g_refmap, obj);
	}

	mutex_unlock(&g_retain_lock);
}

bool class_is_kind_of(const class_t *cls, const class_t *other)
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

