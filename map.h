#ifndef MAP_H

#define MAP_H

#include "config.h"
#include "memory.h"

#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

typedef struct s_mapnode mapnode_t;
typedef struct s_map map_t;
/* typedef int (*comparator_fn)(void *left, void *right); */
typedef int key_t;

typedef enum {
	BLACK	= 0,
	RED		= 1
} mapnode_color_t;

struct s_mapnode
{
	mapnode_t *left, *right, *parent;
	void *p;
	key_t key;
	mapnode_color_t color;
};

struct s_map
{
	mapnode_t *root;
	int size;
/*	comparator_fn compare; */
	memory_pool_t *pool;
};

void map_init(map_t *map, memory_pool_t *pool);
void map_destroy(map_t *map);

void map_insert(map_t *map, key_t key, void *p);
int map_remove(map_t *map, key_t key);

int map_get(map_t *map, key_t key, void **result);
void map_getValues(map_t *map, void **values, size_t capacity);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* end of include guard: MAP_H */