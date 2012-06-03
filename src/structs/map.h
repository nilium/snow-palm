/*
	Map collection
	Written by Noel Cower

	See LICENSE.md for license information
*/

#ifndef MAP_H

#define MAP_H

#include <snow-config.h>
#include "memory.h"

#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

typedef struct s_mapnode mapnode_t;
typedef struct s_map map_t;
typedef struct s_mapops mapops_t;
/* typedef int (*comparator_fn)(void *left, void *right); */
typedef void *mapkey_t;

typedef enum
{
	BLACK	= 0,
	RED		= 1
} mapnode_color_t;

struct s_mapnode
{
	mapnode_t *left, *right, *parent;
	void *p;
	mapkey_t key;
	mapnode_color_t color;
};

struct s_mapops
{
	mapkey_t (*copy_key)(mapkey_t key);
	void (*destroy_key)(mapkey_t key);
	int (*compare_key)(mapkey_t left, mapkey_t right);
	void *(*copy_value)(void *value);
	void (*destroy_value)(void *value);
};

struct s_map
{
	mapnode_t *root;
	int size;
/*	comparator_fn compare; */
	memory_pool_t *pool;
	mapops_t ops;
};

extern const mapops_t g_mapops_default;

void map_init(map_t *map, mapops_t ops, memory_pool_t *pool);
void map_destroy(map_t *map);

void map_insert(map_t *map, mapkey_t key, void *p);
bool map_remove(map_t *map, mapkey_t key);

int map_size(const map_t *map);

void *map_get(const map_t *map, mapkey_t key);
/* ho-ho, now we're cooking with evil */
void **map_get_addr(map_t *map, mapkey_t key);
int map_get_values(const map_t *map, mapkey_t *keys, void **values, size_t capacity);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* end of include guard: MAP_H */
