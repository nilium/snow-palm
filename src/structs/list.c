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

static listnode_t *list_alloc_node(list_t *list, void *ptr) {
  listnode_t *node = (listnode_t *)com_malloc(list->allocator, sizeof(*node));
  node->list = list;
  node->pointer = ptr;

  return node;
}

void list_destroy(list_t *self)
{
  allocator_t *alloc = self->allocator;

  self->head.prev->next = NULL;
  listnode_t *next = self->head.next;
  while (next) {
    listnode_t *temp = next->next;
    com_free(alloc, next);
    next = temp;
  }

  memset(self, 0, sizeof(*self));
}

list_t *list_init(list_t *self, allocator_t *alloc)
{
  if (alloc == NULL)
    alloc = g_default_allocator;
  
  self->allocator = alloc;
  self->size = 0;
  self->head.next = self->head.prev = &self->head;
  self->head.list = self;
  return self;
}

bool list_insert_node_before(listnode_t *succ, listnode_t *pred)
{
  if (!(succ && pred))
    return false;

  pred->next = succ;
  pred->prev = succ->prev;

  succ->prev->next = pred;
  succ->prev = pred;

  pred->list = succ->list;
  succ->list->size += 1;

  return true;
}

bool list_insert_node_after(listnode_t *pred, listnode_t *succ)
{
  if (!(succ && pred))
    return false;

  succ->prev = pred;
  succ->next = pred->next;

  pred->next->prev = succ;
  pred->next = succ;

  succ->list = pred->list;
  pred->list->size += 1;

  return true;
}

listnode_t *list_insert_before(listnode_t *node, void *pointer)
{
  list_t *list = node->list;
  listnode_t *self = list_alloc_node(node->list, pointer);
  list->size += 1;
  
  self->next = node;
  self->prev = node->prev;

  self->prev->next = self;
  self->next->prev = self;

  return self;
}

listnode_t *list_insert_after(listnode_t *node, void *pointer)
{
  list_t *list = node->list;
  listnode_t *self = list_alloc_node(list, pointer);
  list->size += 1;
  
  self->prev = node;
  self->next = node->next;

  self->prev->next = self;
  self->next->prev = self;

  return self;
}

listnode_t *list_append(list_t *list, void *pointer)
{
  return list_insert_after(list->head.prev, pointer);
}

listnode_t *list_prepend(list_t *list, void *pointer)
{
  return list_insert_before(list->head.next, pointer);
}

void *list_at(const list_t *list, size_t index)
{
  if (list->size <= index) {
    s_log_error("Attempt to access contents of node at index %zu beyond list bounds", index);
    return NULL;
  }

  listnode_t *node = list_node_at(list, index);
  return node->pointer;
}

listnode_t *list_node_at(const list_t *list, size_t index)
{
  size_t abs_index = index;
  if (list->size <= abs_index) {
    s_log_error("Attempt to access node at index %zu beyond list bounds", index);
    return NULL;
  }

  listnode_t *node;
  if (index < (list->size / 2)) {
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

listnode_t *list_node_with_value(const list_t *list, const void *pointer, is_equal_fn_t equals)
{
  listnode_t *node;

  if (equals == NULL)
    return list_node_with_pointer(list, pointer);

  node = list->head.next;

  while (node != &list->head) {
    if (equals(pointer, node->pointer))
      return node;
    node = node->next;
  }

  return NULL;
}

listnode_t *list_node_with_pointer(const list_t *list, const void *pointer)
{
  listnode_t *node = list->head.next;

  while (node != &list->head) {
    if (pointer == node->pointer)
      return node;
    node = node->next;
  }

  return NULL;
}

size_t list_count(const list_t *list)
{
  return (list ? list->size : 0);
}

bool list_is_empty(const list_t *list)
{
  return (list && list->size == 0);
}

void list_clear(list_t *list)
{
  allocator_t *alloc = list->allocator;
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

bool list_remove_pointer(list_t *list, const void *pointer)
{
  listnode_t *node = list_node_with_pointer(list, pointer);
  if (node) list_remove(node);
  return !!node;
}

size_t list_remove_all_of_pointer(list_t *list, const void *pointer)
{
  size_t n_removed = 0;
  listnode_t *head = &list->head;
  listnode_t *node = head->next;

  while (node && node != head) {
    listnode_t *next = node->next;

    if (pointer == node->pointer)
      list_remove(node);

    node = next;
  }

  return n_removed;
}

bool list_remove_value(list_t *list, const void *pointer, is_equal_fn_t equals)
{
  listnode_t *node = list_node_with_value(list, pointer, equals);
  if (node) list_remove(node);
  return !!node;
}

size_t list_remove_all_of_value(list_t *list, const void *pointer, is_equal_fn_t equals)
{
  size_t n_removed = 0;
  listnode_t *head;
  listnode_t *node;

  if (equals == NULL)
    return list_remove_all_of_pointer(list, pointer);

  head = &list->head;
  node = head->next;

  while (node && node != head) {
    listnode_t *next = node->next;

    if (equals(pointer, node->pointer))
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

void list_each(list_t *list, list_iter_fn_t iter, void *context)
{
  if (list == NULL) {
    return;
  }
  if (iter == NULL) {
    return;
  }
  bool stop = false;
  listnode_t *node = list->head.next;
  listnode_t *term = &list->head;
  while (!stop && node != term) {
    iter(node->pointer, context, &stop);
    node = node->next;
  }
}

#if defined(__cplusplus)
}
#endif /* __cplusplus */

