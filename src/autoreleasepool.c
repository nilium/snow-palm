/*
	Autorelease pool routines
	Written by Noel Cower

	See LICENSE.md for license information
*/

#include "autoreleasepool.h"
#include "dynarray.h"
#include "threadstorage.h"

#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

#define AUTORELEASE_POOL_ALLOC_TAG 0x0D007011

typedef struct s_autoreleasepool autoreleasepool_t;

struct s_autoreleasepool
{
	array_t *objectArray;
	array_t *baseArray;
};

static tlskey_t g_arp_base_key = (tlskey_t)"arp_base_key";

static void arpool_tls_dtor(tlskey_t key, void *value)
{
	autoreleasepool_t *base = (autoreleasepool_t *)value;
	if (base) {
		size_t len = array_size(base->objectArray);
		object_t **objs = (object_t **)array_buffer(base->objectArray, NULL);
		while (len) {
			object_release(objs[--len]);
		}

		object_release(base->objectArray);
		object_release(base->baseArray);
		mem_free(base);
	}
}

void autoreleasepool_push()
{
	autoreleasepool_t *base = (autoreleasepool_t *)tls_get(g_arp_base_key);
	if (!base) {
		base = (autoreleasepool_t *)mem_alloc(NULL, sizeof(autoreleasepool_t), AUTORELEASE_POOL_ALLOC_TAG);
		base->objectArray = array_new(sizeof(object_t *), 0, 32, NULL);
		base->baseArray = array_new(sizeof(size_t), 1, 8, NULL);
		tls_put(g_arp_base_key, base, arpool_tls_dtor);
		*(size_t *)array_buffer(base->baseArray, NULL) = 0;
	} else {
		size_t baseIndex = array_size(base->objectArray);
		array_push(base->baseArray, &baseIndex);
	}
}

void autoreleasepool_addObject(object_t *object)
{
	autoreleasepool_t *base = (autoreleasepool_t *)tls_get(g_arp_base_key);

	if (!base) {
		log_error("Attempting to add object to autorelease pool without an autorelease pool available - nothing done.\n");
		return;
	}

	array_push(base->objectArray, &object);
}

void autoreleasepool_pop()
{
	autoreleasepool_t *base = (autoreleasepool_t *)tls_get(g_arp_base_key);

	if (!base) {
		log_error("Attempting to pop autorelease pool without one on the stack - nothing done.\n");
		return;
	}

	size_t dest;
	object_t **objs = (object_t **)array_buffer(base->objectArray, NULL);
	size_t size = array_size(base->objectArray);
	array_pop(base->baseArray, &dest);
	while (size != dest) {
		object_release(objs[--size]);
	}
	array_resize(base->objectArray, dest);
}

#if defined(__cplusplus)
}
#endif /* __cplusplus */
