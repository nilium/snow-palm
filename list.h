#ifndef LIST_H

#define LIST_H

#include "config.h"
#include "memory.h"

#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

typedef struct s_listnode listnode_t;
typedef struct s_list list_t;

struct s_listnode
{
	list_t *list;
	listnode_t *next;
	listnode_t *prev;
	void *p;
};

struct s_list
{
	listnode_t head;
	int size;
	memory_pool_t *pool;
};

/*! \p A Note on List Indices
 * List indices, in the context of linked lists in snow, go two ways:
 * 1. Positive indices are from zero to N, and begin at the start of the list
 *    and go to the end of the list.
 * 2. Negative indices, from -1 to -N, go from the list tail to the start of
 *    the list.  They relative to the end of the list, and equate to the same
 *    as writing `list_count(list) + [negative index]`.
 */

/*! \brief Allocates a linked list.
 *  \param[in] list The list to initialize.
 *  \param[in] pool The pool to allocate list nodes from.
 */
void list_init(list_t *list, memory_pool_t *pool);
void list_destroy(list_t *list);

listnode_t *list_insertBefore(listnode_t *node, void *p);
listnode_t *list_insertAfter(listnode_t *node, void *p);

inline listnode_t *list_append(list_t *list, void *p)
{
	return list_insertAfter(list->head.prev, p);
}

inline listnode_t *list_prepend(list_t *list, void *p)
{
	return list_insertBefore(list->head.next, p);
}

void *list_at(const list_t *list, int index);
listnode_t *list_nodeAt(const list_t *list, int index);
listnode_t *list_nodeWithValue(const list_t *list, void *p);

int list_isEmpty(const list_t *list);

void list_clear(list_t *list);
void list_remove(listnode_t *node);
void list_removeValue(list_t *list, void *p);
int list_removeAllOfValue(list_t *list, void *p);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* end of include guard: LIST_H */
