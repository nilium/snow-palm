/*
  Linked list object container
  Written by Noel Cower

  See LICENSE.md for license information
*/

#ifndef __SNOW__LIST_H__
#define __SNOW__LIST_H__

#include <snow-config.h>
#include <memory/allocator.h>

#ifdef __SNOW__LIST_C__
#define S_INLINE
#else
#define S_INLINE inline
#endif

#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

typedef struct s_listnode listnode_t;
typedef struct s_list list_t;
typedef bool (*is_equal_fn_t)(const void *left, const void *right);
typedef void (*list_iter_fn_t)(const void *ptr, void *context, bool *stop);

struct s_listnode
{
  list_t *list;
  listnode_t *next;
  listnode_t *prev;
  void *pointer;
};

struct s_list
{
  listnode_t head;
  size_t size;
  bool release;
  allocator_t *allocator;
};

/*! \p A Note on List Indices
 * List indices, in the context of linked lists in snow, go two ways:
 * 1. Positive indices are from zero to N, and begin at the start of the list
 *    and go to the end of the list.
 * 2. Negative indices, from -1 to -N, go from the list tail to the start of
 *    the list.  They relative to the end of the list, and equate to the same
 *    as writing `list_count(list) + [negative index]`.
 *
 *  \p On Memory Management
 * List init/destroy routines do not allocate an entirely new list or free an
 * existing list from memory, they only prepare a list for use and destroy its
 * nodes, respectively.  If you allocate a list, you must also free it.
 */


/*! \brief Allocates and initializes a linked list.
 *  \param[in] list The list to be initialized.
 */
list_t *list_init(list_t *list, allocator_t *alloc);
void list_destroy(list_t *self);

bool list_insert_node_before(listnode_t *succ, listnode_t *pred);
bool list_insert_node_after(listnode_t *pred, listnode_t *succ);
listnode_t *list_insert_before(listnode_t *node, void *value);
listnode_t *list_insert_after(listnode_t *node, void *value);

listnode_t *list_append(list_t *list, void *value);
listnode_t *list_prepend(list_t *list, void *value);

void *list_at(const list_t *list, size_t index);
listnode_t *list_node_at(const list_t *list, size_t index);
listnode_t *list_node_with_pointer(const list_t *list, const void *ptr);
listnode_t *list_node_with_value(const list_t *list, const void *ptr, is_equal_fn_t equals);

/*! Gets the length of the list.
  \returns The length of the list, [0,N).  Returns -1 if the list is NULL.
*/
size_t list_count(const list_t *list);
/*! Determines whether or not the list is empty.
  \returns 1 if the list is empty, 0 if not.  Returns -1 if the list is NULL.
*/
bool list_is_empty(const list_t *list);

void list_clear(list_t *list);
void list_remove(listnode_t *node);
bool list_remove_pointer(list_t *list, const void *ptr);
size_t list_remove_all_of_pointer(list_t *list, const void *ptr);
bool list_remove_value(list_t *list, const void *ptr, is_equal_fn_t equals);
size_t list_remove_all_of_value(list_t *list, const void *ptr, is_equal_fn_t equals);

listnode_t *list_first_node(list_t *list);
listnode_t *list_last_node(list_t *list);

listnode_t *listnode_next(listnode_t *node);
listnode_t *listnode_previous(listnode_t *node);

void list_each(list_t *list, list_iter_fn_t iter, void *context);

#if defined(__cplusplus)
}
#endif

#include <inline.end>

#endif /* end of include guard: __SNOW__LIST_H__ */
