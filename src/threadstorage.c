/*
	Thread-local storage system
	Written by Noel Cower

	See LICENSE.md for license information
*/

#include "threadstorage.h"

#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

#if USE_PTHREADS

#define DTOR_KV_CAPACITY 32
#define TLS_ALLOC_TAG 0x006000F8

typedef struct s_tls_base tls_base_t;
typedef struct s_tls_entry tls_entry_t;

struct s_tls_base
{
	map_t kvmap;
	//memory_pool_t pool;
};

struct s_tls_entry
{
	tls_destructor_t dtor;
	void *value;
};

pthread_key_t g_tls_key;

static void tls_specific_dtor(void *value)
{
	int kvindex;
	mapkey_t keys[32];
	void *values[32];
	tls_base_t *base;

	if (!value) return;

	base = (tls_base_t *)value;

	while ((kvindex = map_get_values(&base->kvmap, keys, values, DTOR_KV_CAPACITY)))
	{
		while (kvindex--) {
			tls_entry_t *entry = (tls_entry_t *)(values[kvindex]);

			map_remove(&base->kvmap, keys[kvindex]);

			if (entry->dtor != NULL)
				entry->dtor(keys[kvindex], entry->value);

			mem_free(entry);
		}
	}

	map_destroy(&base->kvmap);

	mem_free(base);
}

void sys_tls_init(void)
{
	pthread_key_create(&g_tls_key, tls_specific_dtor);
}

void sys_tls_shutdown(void)
{
	tls_specific_dtor(pthread_getspecific(g_tls_key));
	pthread_key_delete(&g_tls_key);
}

void tls_put(tlskey_t key, void *value, tls_destructor_t dtor)
{
	void *specific = pthread_getspecific(g_tls_key);
	tls_base_t *base;
	tls_entry_t *entry;

	if (specific == NULL) {
		base = (tls_base_t *)mem_alloc(NULL, sizeof(tls_base_t), TLS_ALLOC_TAG);
		map_init(&base->kvmap, g_mapops_default, NULL);
		pthread_setspecific(g_tls_key, (void *)base);
	} else {
		base = (tls_base_t *)specific;
	}
	
	entry = map_get(&base->kvmap, key);
	if (!entry) {
		entry = (tls_entry_t *)mem_alloc(NULL, sizeof(tls_entry_t), TLS_ALLOC_TAG);
	}

	entry->dtor = dtor;
	entry->value = value;

	map_insert(&base->kvmap, key, entry);
}

void *tls_get(tlskey_t key)
{
	void *specific = pthread_getspecific(g_tls_key);

	if (specific) {
		tls_base_t *base = (tls_base_t *)specific;
		tls_entry_t *entry = map_get(&base->kvmap, key);
		if (entry) return entry->value;
	}

	return NULL;
}

#else

#error "No thread-local storage implementation available on this platform"

#endif

#if defined(__cplusplus)
}
#endif /* __cplusplus */

