#include "list.h"

#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

const int32_t LIST_ALLOC_TAG = 0x00115700;

void list_init(list_t *list, memory_pool_t *pool, bool weak)
{
	list->head.next = list->head.prev = &list->head;
	list->head.obj = NULL;
	list->head.list = list;
	list->pool = mem_retain_pool(pool);
	list->weak = weak;
}

void list_destroy(list_t *list)
{
	list->head.prev->next = NULL;
	listnode_t *next = list->head.next;
	while (next) {
		listnode_t *temp = next->next;
		if (next->obj && !list->weak) object_release(next->obj);
		mem_free(next);
		next = temp;
	}

	memset(&list->head, 0, sizeof(listnode_t));

	mem_release_pool(list->pool);
	list->pool = NULL;
}

listnode_t *list_insertBefore(listnode_t *node, object_t *obj)
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

listnode_t *list_insertAfter(listnode_t *node, object_t *obj)
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
	return list_insertAfter(list->head.prev, obj);
}

listnode_t *list_prepend(list_t *list, object_t *obj)
{
	return list_insertBefore(list->head.next, obj);
}

void *list_at(const list_t *list, int index)
{
	if (list->size <= (index < 0 ? (list->size + index) : index)) {
		log_error("Attempt to access contents of node at index %d beyond list bounds\n", index);
		return NULL;
	}

	listnode_t *node = list_nodeAt(list, index);
	return node->obj;
}

listnode_t *list_nodeAt(const list_t *list, int index)
{
	int absIndex = index;
	if (index < 0) absIndex = list->size + index;
	if (list->size <= absIndex) {
		log_error("Attempt to access node at index %d beyond list bounds\n", index);
		return NULL;
	}

	listnode_t *node;
	if (index < list->size / 2) {
		node = list->head.next;
		while (absIndex-- && (node = node->next));
	} else {
		/* if the index is greater than half the size of the list, just start
		   at the back of the list and go backwards */
		node = list->head.prev;
		while (++index && (node = node->prev));
	}

	return node;
}

listnode_t *list_nodeWithValue(const list_t *list, object_t *obj)
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

bool list_isEmpty(const list_t *list)
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

bool list_removeValue(list_t *list, object_t *obj)
{
	listnode_t *node = list_nodeWithValue(list, obj);
	if (node) list_remove(node);
	return !!node;
}

int list_removeAllOfValue(list_t *list, object_t *obj)
{
	int nRemoved = 0;
	listnode_t *node = list_nodeWithValue(list, obj);

	while (node) {
		listnode_t *next = node->next;
		list_remove(node);
		node = next;

		while (node->obj != obj && node != &list->head)
			node = node->next;

		if (node == &list->head)
			break;
	}

	return nRemoved;
}

listnode_t *list_firstNode(list_t *list)
{
	listnode_t *node = list->head.next;
	return node != &list->head ? node : NULL;
}

listnode_t *list_lastNode(list_t *list)
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

