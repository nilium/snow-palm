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

static list_t *list_ctor(list_t *self, memory_pool_t *pool);
static void list_dtor(list_t *self);

const class_t g_list_class = {
	object_class,
	sizeof(list_t),

	(constructor_t)list_ctor,
	(destructor_t)list_dtor,

	object_compare
};

const int32_t LIST_ALLOC_TAG = 0x00115700;

static list_t *list_ctor(list_t *self, memory_pool_t *pool)
{
	self = self->isa->super->ctor(self, pool);

	if (self) {
		self->head.next = self->head.prev = &self->head;
		self->head.obj = NULL;
		self->head.list = self;
		self->pool = mem_retain_pool(pool);
		self->weak = false;
	}

	return self;
}

static void list_dtor(list_t *self)
{
	self->head.prev->next = NULL;
	listnode_t *next = self->head.next;
	while (next) {
		listnode_t *temp = next->next;
		if (next->obj && !self->weak) object_release(next->obj);
		mem_free(next);
		next = temp;
	}

	memset(&self->head, 0, sizeof(listnode_t));

	mem_release_pool(self->pool);
	self->pool = NULL;

	self->isa->super->dtor(self);
}

list_t *list_init(list_t *self, bool weak)
{
	self->weak = weak;
	return self;
}

listnode_t *list_insert_before(listnode_t *node, object_t *obj)
{
	listnode_t *newnode = mem_alloc(node->list->pool, sizeof(listnode_t), LIST_ALLOC_TAG);
	node->list->size += 1;

	if (obj && !node->list->weak) object_retain(obj);
	
	newnode->list = node->list;
	newnode->obj = obj;
	
	newnode->next = node;
	newnode->prev = node->prev;

	newnode->prev->next = newnode;
	newnode->next->prev = newnode;

	return newnode;
}

listnode_t *list_insert_after(listnode_t *node, object_t *obj)
{
	listnode_t *newnode = mem_alloc(node->list->pool, sizeof(listnode_t), LIST_ALLOC_TAG);
	node->list->size += 1;
	
	if (obj && !node->list->weak) object_retain(obj);

	newnode->list = node->list;
	newnode->obj = obj;
	
	newnode->prev = node;
	newnode->next = node->next;

	newnode->prev->next = newnode;
	newnode->next->prev = newnode;

	return newnode;
}

listnode_t *list_append(list_t *list, object_t *obj)
{
	return list_insert_after(list->head.prev, obj);
}

listnode_t *list_prepend(list_t *list, object_t *obj)
{
	return list_insert_before(list->head.next, obj);
}

void *list_at(const list_t *list, int index)
{
	if (list->size <= (index < 0 ? (list->size + index) : index)) {
		log_error("Attempt to access contents of node at index %d beyond list bounds\n", index);
		return NULL;
	}

	listnode_t *node = list_node_at(list, index);
	return node->obj;
}

listnode_t *list_node_at(const list_t *list, int index)
{
	int abs_index = index;
	if (index < 0) abs_index = list->size + index;
	if (list->size <= abs_index) {
		log_error("Attempt to access node at index %d beyond list bounds\n", index);
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

listnode_t *list_node_with_value(const list_t *list, object_t *obj)
{
	listnode_t *node = list->head.next;

	while (node != &list->head) {
		if (node->obj == obj)
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
	listnode_t *node = list->head.next;

	if (node == &list->head) return;

	list->head.next->prev = NULL;
	list->head.prev->next = NULL;
	list->head.next = list->head.prev = &list->head;
	list->size = 0;

	while (node) {
		listnode_t *next = node->next;
		if (node->obj && !list->weak) object_release(node->obj);
		mem_free(node);
		node = next;
	}
}

void list_remove(listnode_t *node)
{
	node->next->prev = node->prev;
	node->prev->next = node->next;
	if (node->obj && !node->list->weak) object_release(node->obj);
	mem_free(node);
}

bool list_remove_value(list_t *list, object_t *obj)
{
	listnode_t *node = list_node_with_value(list, obj);
	if (node) list_remove(node);
	return !!node;
}

int list_remove_all_of_value(list_t *list, object_t *obj)
{
	int n_removed = 0;
	listnode_t *node = list_node_with_value(list, obj);

	while (node) {
		listnode_t *next = node->next;
		list_remove(node);
		node = next;

		while (node->obj != obj && node != &list->head)
			node = node->next;

		if (node == &list->head)
			break;
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

