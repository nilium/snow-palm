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
	array_t *object_array;
	array_t *base_array;
};

/* Note: this is a hack.  The key is a pointer to itself - this does not exactly
 * guarantee uniqueness, but the assumption is that the global will be given a
 * unique place in memory and the key will in turn be unique in that sense.  The
 * problem is that this goes along with the assumption that all other TLS keys
 * are unique and do not clash with this.  While I can guarantee this in my code,
 * I cannot reasonably guarantee it the moment someone else touches it.  Hence
 * this note - consider yourself warned, you are likely to be eaten by a grue.
 */
static tlskey_t g_arp_base_key = (tlskey_t)&g_arp_base_key;

static void arpool_tls_dtor(tlskey_t key, void *value)
{
	autoreleasepool_t *base = (autoreleasepool_t *)value;
	if (base) {
		size_t len = array_size(base->object_array);
		object_t **objs = (object_t **)array_buffer(base->object_array, NULL);
		while (len) {
			object_release(objs[--len]);
		}

		object_release(base->object_array);
		object_release(base->base_array);
		mem_free(base);
	}
}

void autoreleasepool_push()
{
	autoreleasepool_t *base = (autoreleasepool_t *)tls_get(g_arp_base_key);
	if (!base) {
		base = (autoreleasepool_t *)mem_alloc(NULL, sizeof(autoreleasepool_t), AUTORELEASE_POOL_ALLOC_TAG);
		base->object_array = array_init(object_new(array_class, NULL), sizeof(object_t *), 0, 32);
		base->base_array = array_init(object_new(array_class, NULL), sizeof(size_t), 1, 8);
		tls_put(g_arp_base_key, base, arpool_tls_dtor);
		*(size_t *)array_buffer(base->base_array, NULL) = 0;
	} else {
		size_t base_index = array_size(base->object_array);
		array_push(base->base_array, &base_index);
	}
}

void autoreleasepool_add_object(object_t *object)
{
	autoreleasepool_t *base = (autoreleasepool_t *)tls_get(g_arp_base_key);

	if (!base) {
		log_error("Attempting to add object to autorelease pool without an autorelease pool available - nothing done.\n");
		return;
	}

	array_push(base->object_array, &object);
}

void autoreleasepool_pop()
{
	autoreleasepool_t *base = (autoreleasepool_t *)tls_get(g_arp_base_key);

	if (!base) {
		log_error("Attempting to pop autorelease pool without one on the stack - nothing done.\n");
		return;
	}

	size_t dest;
	object_t **objs = (object_t **)array_buffer(base->object_array, NULL);
	size_t size = array_size(base->object_array);
	array_pop(base->base_array, &dest);
	while (size != dest) {
		object_release(objs[--size]);
	}
	array_resize(base->object_array, dest);
}

#if defined(__cplusplus)
}
#endif /* __cplusplus */
