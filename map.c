#include "map.h"

#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

const int MAP_ALLOC_TAG = 0x00773300;

#define IS_BLACK(NODE) ((NODE)->color == BLACK)
#define IS_RED(NODE) ((NODE)->color == RED)
#define IS_LEFT(NODE) ((NODE) == (NODE)->parent->left)
#define IS_RIGHT(NODE) ((NODE) == (NODE)->parent->right)
#define DIR_NODE(NODE, DIR) ((DIR) ? (NODE)->right : (NODE)->left)
#define OPP_NODE(NODE, DIR) ((DIR) ? (NODE)->left: (NODE)->right)

static mapnode_t NIL_NODE = {
	&NIL_NODE, &NIL_NODE, &NIL_NODE, NULL, (key_t)0, BLACK
};
static mapnode_t *NIL = &NIL_NODE;

static int map_test(mapnode_t *node);

static inline void node_rotate_left(map_t *map, mapnode_t *node)
{
		mapnode_t *right = node->right;
		node->right = right->left;
		if (node->right != NIL)
			node->right->parent = node;

		right->parent = node->parent;

		if (node->parent == NIL) {
			map->root = right;
		} else {
			if (IS_LEFT(node)) {
				node->parent->left = right;
			} else {
				node->parent->right = right;
			}
		}

		right->left = node;
		node->parent = right;
}

static inline void node_rotate_right(map_t *map, mapnode_t *node)
{
		mapnode_t *left = node->left;
		node->left = left->right;
		if (node->left != NIL)
			node->left->parent = node;

		left->parent = node->parent;

		if (node->parent == NIL) {
			map->root = left;
		} else {
			if (IS_LEFT(node)) {
				node->parent->left = left;
			} else {
				node->parent->right = left;
			}
		}

		left->right = node;
		node->parent = left;
}

static inline mapnode_t *node_sibling(mapnode_t *node)
{
	if (node->parent == NIL) return NIL;

	return IS_LEFT(node) ? node->parent->right : node->parent->left;
}

static mapnode_t *node_uncle(mapnode_t *node)
{
	if (node != NIL && node->parent != NIL) {
		return (node->parent->left == node) ? node->parent->right : node->parent->left;
	}
	return NIL;
}

static inline mapnode_t *node_grandparent(mapnode_t *node)
{
	if (node != NIL) {
		node = node->parent;
		if (node != NIL) node = node->parent;
	}

	return node;
}

typedef void (*rotate_fn)(map_t *, mapnode_t *);
static const rotate_fn g_rotations[2] = {
	node_rotate_left,		/* 0 left */
	node_rotate_right		/* 1 right */
};

static void mapnode_remove(map_t *map, mapnode_t *node)
{
	mapnode_t *destroyed, *y, *z;
	
	if (node->left == NIL) {
		destroyed = node;
		y = node->right;
	} else if (node->right == NIL) {
		destroyed = node;
		y = node->left;
	} else {
		destroyed = node->left;
		while (destroyed->right != NIL)
			destroyed = destroyed->right;

		y = destroyed->left;
		node->key = destroyed->key;
		node->p = destroyed->p;
	}

	z = destroyed->parent;
	if (y != NIL)
		y->parent = z;

	if (z == NIL) {
		map->root = y;
		goto finish_removal;
	}
	
	if (destroyed == z->left) {
		z->left = y;
	} else {
		z->right = y;
	}

	if (IS_BLACK(destroyed)) {
		while (y != map->root && IS_BLACK(y)) {
			mapnode_t *sibling;
			int dir = !(y == z->left);
			sibling = OPP_NODE(z, dir);

			if (IS_RED(sibling)) {
				sibling->color = BLACK;
				z->color = RED;
				g_rotations[dir](map, z);
				sibling = OPP_NODE(z, dir);
			}

			if (IS_BLACK(sibling->left) && IS_BLACK(sibling->right)) {
				sibling->color = RED;
				y = z;
				z = z->parent;
			} else {
				if (IS_BLACK(OPP_NODE(sibling, dir))) {
					DIR_NODE(sibling, dir)->color = BLACK;
					sibling->color = RED;
					g_rotations[!dir](map, sibling);
					sibling = OPP_NODE(z, dir);
				}

				sibling->color = z->color;
				z->color = BLACK;
				OPP_NODE(sibling, dir)->color = BLACK;
				g_rotations[dir](map, z);
				y = map->root;
			}
		}

		y->color = BLACK;

	}
	
finish_removal:
	mem_free(destroyed);
	map->size -= 1;

#if !defined(NDEBUG)
	map_test(map->root);
#endif
}

static void mapnode_replace(map_t *map, mapnode_t *node, mapnode_t *with)
{
	if (node == NIL)
		return;

	if (node->parent == NIL) {
		map->root = with;
	} else {
		if (IS_LEFT(node))
			node->parent->left = with;
		else
			node->parent->right = with;
	}
	with->parent = node->parent;
}

static mapnode_t *mapnode_find(mapnode_t *node, key_t key)
{
	while (node != NIL) {
		if (node->key == key)
			break;

		if (key < node->key)
			node = node->left;
		else
			node = node->right;
	}

	return node;
}

void map_init(map_t *map, memory_pool_t *pool)
{
	map->root = NIL;
	map->size = 0;
	map->pool = mem_retain_pool(pool);
}

static void mapnode_destroy_r(mapnode_t *node)
{
	mapnode_t *l, *r;
	if (node == NIL) return;
	l = node->left;
	r = node->right;
	mem_free(node);

	mapnode_destroy_r(l);
	mapnode_destroy_r(r);
}

void map_destroy(map_t *map)
{
	mapnode_destroy_r(map->root);
}

#if !defined(NDEBUG)
static int map_test(mapnode_t *node)
{
	/* got this routine by Julienne Walker from
	 * http://eternallyconfuzzled.com/tuts/datastructures/jsw_tut_rbtree.aspx
	 */
	mapnode_t *left, *right;

	if (node == NIL) return 1;

	left = node->left;
	right = node->right;

	if (IS_RED(node) && (IS_RED(right) || IS_RED(right))) {
		log_note("Red violation on node with key %X\n", node->key);
		return 0;
	}

	int lh = map_test(left);
	int rh = map_test(right);

	if (left != NIL && left->key > node->key) {
		log_note("Left node (key: %X) of parent node (key: %X) is incorrectly ordered\n", left->key, node->key);
		return 0;
	}

	if (right != NIL && right->key < node->key) {
		log_note("Right node (key: %X) of parent node (key: %X) is incorrectly ordered\n", right->key, node->key);
		return 0;
	}

	if (lh && rh)
		return IS_RED(node) ? lh : lh + 1;
	else
		return 0;
}
#endif

void map_insert(map_t *map, key_t key, void *p)
{
	mapnode_t *parent = map->root;
	mapnode_t **slot = &map->root;
	mapnode_t *insert;

	while (parent != NIL) {
		if (key == parent->key) {
			parent->p = p;
			return;
		} else if (key < parent->key) {
			if (parent->left != NIL) {
				parent = parent->left;
				continue;
			} else {
				slot = &parent->left;
				break;
			}
		} else if (key > parent->key) {
			if (parent->right != NIL) {
				parent = parent->right;
				continue;
			} else {
				slot = &parent->right;
				break;
			}
		}
	}

	map->size += 1;
	insert = (mapnode_t *)mem_alloc(map->pool, sizeof(mapnode_t), MAP_ALLOC_TAG);
	insert->left = insert->right = NIL;
	insert->key = key;
	insert->p = p;
	insert->color = RED;
	insert->parent = parent;

	*slot = insert;
	
	while (IS_RED(insert->parent) && node_grandparent(insert) != NIL) {
		mapnode_t *uncle = node_sibling(insert->parent);
		if (IS_RED(uncle)) {
			insert->parent->color = BLACK;
			uncle->color = BLACK;
			uncle->parent->color = RED;
			insert = uncle->parent;
		} else {
			int insleft = IS_LEFT(insert);
			int parleft = IS_LEFT(insert->parent);

			if (!insleft && parleft) {
				insert = insert->parent;
				node_rotate_left(map, insert);
			} else if (insleft && !parleft) {
				insert = insert->parent;
				node_rotate_right(map, insert);
			}

			insert->parent->parent->color = RED;
			insert->parent->color = BLACK;

			if (parleft)
				node_rotate_right(map, insert->parent->parent);
			else
				node_rotate_left(map, insert->parent->parent);
		}
	}

	map->root->color = BLACK;

#if !defined(NDEBUG)
	map_test(map->root);
#endif
}

int map_remove(map_t *map, key_t key)
{
	mapnode_t *node = mapnode_find(map->root, key);
	
	if (node != NIL) {
		mapnode_remove(map, node);
		return 1;
	}

	return 0;
}

int map_get(map_t *map, key_t key, void **result)
{
	mapnode_t *node = mapnode_find(map->root, key);

	if (node != NIL && result)
		*result = node->p;

	return (node != NIL);
}

void map_getValues(map_t *map, void **values, size_t capacity)
{
}


#if defined(__cplusplus)
}
#endif /* __cplusplus */

