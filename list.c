#include "list.h"

#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

const int32_t LIST_ALLOC_TAG = 0x00115700;

void list_init(list_t *list, memory_pool_t *pool)
{
	list->head.next = list->head.prev = list->head.p = &list->head;
	list->head.list = list;
	list->pool = mem_retain_pool(pool);
}

void list_destroy(list_t *list)
{
	list->head.prev->next = NULL;
	listnode_t *next = list->head.next;
	while (next) {
		listnode_t *temp = next->next;
		mem_free(next);
		next = temp;
	}

	memset(&list->head, 0, sizeof(listnode_t));

	mem_release_pool(list->pool);
	list->pool = NULL;
}

listnode_t *list_insertBefore(listnode_t *node, void *p)
{
	listnode_t *newnode = mem_alloc(node->list->pool, sizeof(listnode_t), LIST_ALLOC_TAG);
	node->list->size += 1;
	
	newnode->list = node->list;
	newnode->p = p;
	
	newnode->next = node;
	newnode->prev = node->prev;

	newnode->prev->next = newnode;
	newnode->next->prev = newnode;

	return newnode;
}

listnode_t *list_insertAfter(listnode_t *node, void *p)
{
	listnode_t *newnode = mem_alloc(node->list->pool, sizeof(listnode_t), LIST_ALLOC_TAG);
	node->list->size += 1;
	
	newnode->list = node->list;
	newnode->p = p;
	
	newnode->prev = node;
	newnode->next = node->next;

	newnode->prev->next = newnode;
	newnode->next->prev = newnode;

	return newnode;
}

void *list_at(list_t *list, int index)
{
	if (list->size <= (index < 0 ? (list->size + index) : index)) {
		log_error("Attempt to access contents of node at index %d beyond list bounds\n", index);
		return NULL;
	}

	listnode_t *node = list_nodeAt(list, index);
	return node->p;
}

listnode_t *list_nodeAt(list_t *list, int index)
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

listnode_t *list_nodeWithValue(list_t *list, void *p)
{
	listnode_t *node = list->head.next;

	while (node != &list->head) {
		if (node->p == p)
			return node;
		node = node->next;
	}

	return NULL;
}

void list_remove(listnode_t *node)
{
	node->next->prev = node->prev;
	node->prev->next = node->next;
	mem_free(node);
}

void list_removeValue(list_t *list, void *p)
{
	listnode_t *node = list_nodeWithValue(list, p);
	if (node) list_remove(node);
}

int list_removeAllOfValue(list_t *list, void *p)
{
	int nRemoved = 0;
	listnode_t *node = list_nodeWithValue(list, p);

	while (node) {
		listnode_t *next = node->next;
		list_remove(node);
		node = next;

		while (node->p != p && node != &list->head)
			node = node->next;

		if (node == &list->head)
			break;
	}

	return nRemoved;
}

#if defined(__cplusplus)
}
#endif /* __cplusplus */

