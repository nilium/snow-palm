/*
  Linked list object container
  Written by Noel Cower

  See LICENSE.md for license information
*/

#include "list.h"

#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

static bool list_equals_default(size_t size, const void *left, const void *right) {
  return (memcmp(left, right, size) == 0);
}

static listnode_t *list_alloc_node(list_t *list, const void *value) {
  const size_t sz = list->obj_size;
  
  listnode_t *node = (listnode_t *)com_malloc(list->allocator, sizeof(*node) + sz);
  node->list = list;
  
  if (value)
    memcpy(node->value, value, sz);
  else
    memset(node->value, 0, sz);

  return node;
}

void list_destroy(list_t *self)
{
  allocator_t alloc = self->allocator;

  self->head.prev->next = NULL;
  listnode_t *next = self->head.next;
  while (next) {
    listnode_t *temp = next->next;
    com_free(alloc, next);
    next = temp;
  }

  memset(self, 0, sizeof(*self));
}

list_t *list_init(list_t *self, size_t object_size, allocator_t alloc)
{
  self->allocator = alloc;
  self->obj_size = object_size;
  self->size = 0;
  self->head.next = self->head.prev = &self->head;
  self->head.list = self;
  return self;
}

listnode_t *list_insert_before(listnode_t *node, void *value)
{
  list_t *list = node->list;
  listnode_t *self = list_alloc_node(node->list, value);
  list->size += 1;
  
  self->next = node;
  self->prev = node->prev;

  self->prev->next = self;
  self->next->prev = self;

  return self;
}

listnode_t *list_insert_after(listnode_t *node, void *value)
{
  list_t *list = node->list;
  listnode_t *self = list_alloc_node(list, value);
  list->size += 1;
  
  self->prev = node;
  self->next = node->next;

  self->prev->next = self;
  self->next->prev = self;

  return self;
}

listnode_t *list_append(list_t *list, void *value)
{
  return list_insert_after(list->head.prev, value);
}

listnode_t *list_prepend(list_t *list, void *value)
{
  return list_insert_before(list->head.next, value);
}

void *list_at(const list_t *list, int index)
{
  if (list->size <= (index < 0 ? (list->size + index) : index)) {
    s_log_error("Attempt to access contents of node at index %d beyond list bounds", index);
    return NULL;
  }

  listnode_t *node = list_node_at(list, index);
  return node->value;
}

listnode_t *list_node_at(const list_t *list, int index)
{
  int abs_index = index;
  if (index < 0) abs_index = list->size + index;
  if (list->size <= abs_index) {
    s_log_error("Attempt to access node at index %d beyond list bounds", index);
    return NULL;
  }

  listnode_t *node;
  if (index < list->size / 2) {
    node = list->head.next;
    while (abs_index-- && (node = node->next));
  } else {
    /* if the index is greater than half the size of the list, just start
       at the back of the list and go backwards */
    node = list->head.prev;
    while (++index && (node = node->prev));
  }

  return node;
}

listnode_t *list_node_with_value(const list_t *list, void *value, is_equal_fn_t equals)
{
  listnode_t *node = list->head.next;
  const size_t sz = list->obj_size;

  if (equals == NULL)
    equals = list_equals_default;

  while (node != &list->head) {
    if (equals(sz, value, node->value))
      return node;
    node = node->next;
  }

  return NULL;
}

int list_count(const list_t *list)
{
  return (list ? list->size : -1);
}

bool list_is_empty(const list_t *list)
{
  return (list ? (list->size == 0) : -1);
}

void list_clear(list_t *list)
{
  allocator_t alloc = list->allocator;
  listnode_t *node = list->head.next;

  if (node == &list->head) return;

  list->head.next->prev = NULL;
  list->head.prev->next = NULL;
  list->head.next = list->head.prev = &list->head;
  list->size = 0;

  while (node) {
    listnode_t *next = node->next;
    com_free(alloc, node);
    node = next;
  }
}

void list_remove(listnode_t *node)
{
  node->next->prev = node->prev;
  node->prev->next = node->next;
  com_free(node->list->allocator, node);
}

bool list_remove_value(list_t *list, void *value, is_equal_fn_t equals)
{
  listnode_t *node = list_node_with_value(list, value, equals);
  if (node) list_remove(node);
  return !!node;
}

size_t list_remove_all_of_value(list_t *list, void *value, is_equal_fn_t equals)
{
  size_t n_removed = 0;
  const size_t sz = list->obj_size;
  listnode_t *head = &list->head;
  listnode_t *node = head->next;

  if (equals == NULL)
    equals = list_equals_default;

  while (node && node != head) {
    listnode_t *next = node->next;

    if (equals(sz, value, node->value))
      list_remove(node);

    node = next;
  }

  return n_removed;
}

listnode_t *list_first_node(list_t *list)
{
  listnode_t *node = list->head.next;
  return node != &list->head ? node : NULL;
}

listnode_t *list_last_node(list_t *list)
{
  listnode_t *node = list->head.prev;
  return node != &list->head ? node : NULL;
}

listnode_t *listnode_next(listnode_t *node)
{
  return node->next != &node->list->head ? node->next : NULL;
}

listnode_t *listnode_previous(listnode_t *node)
{
  return node->prev != &node->list->head ? node->prev : NULL;
}

#if defined(__cplusplus)
}
#endif /* __cplusplus */

