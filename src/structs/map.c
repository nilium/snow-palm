/*
  Map collection
  Written by Noel Cower

  See LICENSE.md for license information
*/

#define __SNOW__MAP_C__

#include "map.h"

#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

static int map_test(mapnode_t *node);
static mapkey_t mapkey_copy(mapkey_t key, allocator_t *alloc);
static void mapkey_destroy(mapkey_t key, allocator_t *alloc);
static int mapkey_compare(mapkey_t left, mapkey_t right);
static void *mapvalue_copy(void *value, allocator_t *alloc);
static void mapvalue_destroy(void *value, allocator_t *alloc);

const int MAP_ALLOC_TAG = 0x00773300;

#define IS_BLACK(NODE) ((NODE)->color == BLACK)
#define IS_RED(NODE) ((NODE)->color == RED)
#define IS_LEFT(NODE) ((NODE) == (NODE)->parent->left)
#define IS_RIGHT(NODE) ((NODE) == (NODE)->parent->right)
#define DIR_NODE(NODE, DIR) ((DIR) ? (NODE)->right : (NODE)->left)
#define OPP_NODE(NODE, DIR) ((DIR) ? (NODE)->left: (NODE)->right)

static mapnode_t NIL_NODE = {
  &NIL_NODE, &NIL_NODE, &NIL_NODE, NULL, (mapkey_t)0, BLACK
};

static mapnode_t *NIL = &NIL_NODE;

const mapops_t g_mapops_default = {
  mapkey_copy,
  mapkey_destroy,
  mapkey_compare,
  mapvalue_copy,
  mapvalue_destroy
};

/* implementation */

static mapkey_t mapkey_copy(mapkey_t key, allocator_t *alloc)
{
  (void)alloc;
  return key;
}

static void mapkey_destroy(mapkey_t key, allocator_t *alloc)
{
  (void)alloc;
  (void)key;
  return;
}

static int mapkey_compare(mapkey_t left, mapkey_t right)
{
  return ((left > right) - (left < right));
}

static void *mapvalue_copy(void *value, allocator_t *alloc)
{
  (void)alloc;
  return value;
}

static void mapvalue_destroy(void *value, allocator_t *alloc)
{
  (void)alloc;
  (void)value;
  return;
}

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
  node_rotate_left,   /* 0 left */
  node_rotate_right   /* 1 right */
};

static void mapnode_remove(map_t *map, mapnode_t *node)
{
  allocator_t *alloc = map->allocator;
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
  com_free(alloc, destroyed);
  map->size -= 1;

#if !defined(NDEBUG)
  map_test(map->root);
#endif
}

static mapnode_t *mapnode_find(const map_t *map, mapnode_t *node, mapkey_t key)
{
  while (node != NIL) {
    int comparison = map->ops.compare_key(key, node->key);

    if (comparison == 0)
      break;

    if (comparison < 0)
      node = node->left;
    else
      node = node->right;
  }

  return node;
}

void map_init(map_t *map, mapops_t ops, allocator_t *alloc)
{
  if (alloc == NULL)
    alloc = g_default_allocator;

  map->root = NIL;
  map->size = 0;
  map->allocator = alloc;

  if (ops.copy_key == NULL) ops.copy_key = mapkey_copy;
  if (ops.destroy_key == NULL) ops.destroy_key = mapkey_destroy;
  if (ops.compare_key == NULL) ops.compare_key = mapkey_compare;
  if (ops.copy_value == NULL) ops.copy_value = mapvalue_copy;
  if (ops.destroy_value == NULL) ops.destroy_value = mapvalue_destroy;

  map->ops = ops;
}

static void mapnode_destroy_r(map_t *map, mapnode_t *node)
{
  allocator_t *alloc = map->allocator;
  mapnode_t *l, *r;
  if (node == NIL) return;
  l = node->left;
  r = node->right;

  map->ops.destroy_key(node->key, alloc);
  map->ops.destroy_value(node->p, alloc);

  com_free(map->allocator, node);

  mapnode_destroy_r(map, l);
  mapnode_destroy_r(map, r);
}

void map_destroy(map_t *map)
{
  mapnode_destroy_r(map, map->root);
  memset(map, 0, sizeof(*map));
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
    s_log_note("Red violation on node with key %p\n", node->key);
    return 0;
  }

  int lh = map_test(left);
  int rh = map_test(right);

  if (left != NIL && left->key > node->key) {
    s_log_note("Left node (key: %p) of parent node (key: %p) is incorrectly ordered\n", left->key, node->key);
    return 0;
  }

  if (right != NIL && right->key < node->key) {
    s_log_note("Right node (key: %p) of parent node (key: %p) is incorrectly ordered\n", right->key, node->key);
    return 0;
  }

  if (lh && rh)
    return IS_RED(node) ? lh : lh + 1;
  else
    return 0;
}
#endif

void map_insert(map_t *map, mapkey_t key, void *p)
{
  allocator_t *alloc = map->allocator;
  mapnode_t *parent = map->root;
  mapnode_t **slot = &map->root;
  mapnode_t *insert;

  while (parent != NIL) {
    int comparison = map->ops.compare_key(key, parent->key);
    if (comparison == 0) {
      map->ops.destroy_value(parent->p, alloc);
      parent->p = map->ops.copy_value(p, alloc);
      return;
    } else if (comparison < 0) {
      if (parent->left != NIL) {
        parent = parent->left;
        continue;
      } else {
        slot = &parent->left;
        break;
      }
    } else {
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
  insert = (mapnode_t *)com_malloc(map->allocator, sizeof(mapnode_t));
  insert->left = insert->right = NIL;
  insert->key = map->ops.copy_key(key, alloc);
  insert->p = map->ops.copy_value(p, alloc);
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

bool map_remove(map_t *map, mapkey_t key)
{
  mapnode_t *node = mapnode_find(map, map->root, key);

  if (node != NIL) {
    allocator_t *alloc = map->allocator;
    void *p = node->p;
    key = node->key;

    mapnode_remove(map, node);

    map->ops.destroy_key(key, alloc);
    map->ops.destroy_value(p, alloc);

    return true;
  }

  return false;
}

int map_size(const map_t *map)
{
  return map->size;
}

void *map_get(const map_t *map, mapkey_t key)
{
  const mapnode_t *node = mapnode_find(map, map->root, key);

  if (node != NIL)
    return node->p;

  return NULL;
}

void **map_get_addr(map_t *map, mapkey_t key)
{
  mapnode_t *node = mapnode_find(map, map->root, key);

  if (node != NIL)
    return &node->p;

  return NULL;
}

static void map_get_values_r(const mapnode_t *node, mapkey_t *keys, void **values, int *count, size_t capacity)
{
  if (capacity <= *count)
    return;

  if (node == NIL)
    return;

  if (node->left != NIL) map_get_values_r(node->left, keys, values, count, capacity);

  if (keys) keys[*count] = node->key;
  if (values) values[*count] = node->p;

  *count += 1;

  if (node->right != NIL) map_get_values_r(node->right, keys, values, count, capacity);
}

int map_get_values(const map_t *map, mapkey_t *keys, void **values, size_t capacity)
{
  int count = 0;
  map_get_values_r(map->root, keys, values, &count, capacity);
  return count;
}


#if defined(__cplusplus)
}
#endif /* __cplusplus */

