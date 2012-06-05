#include "events.h"
#include <structs/dynarray.h>
#include <structs/list.h>
#include <threads/mutex.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct s_event_handler {
  listnode_t node; // .pointer = context
  int priority;
  event_handler_fn_t handler;
} event_handler_t;

static allocator_t *g_event_alloc = NULL;
static array_t g_event_array;
static list_t g_handler_list;
static mutex_t g_events_lock;
static mutex_t g_handler_lock;

static void com_dispatch_event(event_t *event)
{
  event_handler_t *node;
  node = (event_handler_t *)list_first_node(&g_handler_list);
  while (node) {
    if (node->handler(event, node->node.pointer))
      break;

    node = (event_handler_t *)listnode_next((listnode_t *)node);
  }
}

void sys_events_init(allocator_t *alloc)
{
  if (alloc == NULL)
    alloc = g_default_allocator;

  g_event_alloc = alloc;
  array_init(&g_event_array, sizeof(event_t), 128, alloc);
  list_init(&g_handler_list, alloc);

  mutex_init(&g_events_lock, false);
  mutex_init(&g_handler_lock, false);
}

void com_queue_event(event_t event)
{
  mutex_lock(&g_events_lock);
  array_push(&g_event_array, &event);
  mutex_unlock(&g_events_lock);
}

void com_process_event_queue(void)
{
  mutex_lock(&g_events_lock);

  if (!array_empty(&g_event_array)) {
    size_t length;
    event_t *events;

    mutex_lock(&g_handler_lock);

    length = array_size(&g_event_array);
    events = array_buffer(&g_event_array, NULL);
    
    while (length) {
      com_dispatch_event(events);
      ++events;
      --length;
    }
    
    array_clear(&g_event_array);

    mutex_unlock(&g_handler_lock);
  }

  mutex_unlock(&g_events_lock);
}

void com_send_event(event_t event)
{
  mutex_lock(&g_handler_lock);

  com_dispatch_event(&event);

  mutex_unlock(&g_handler_lock);
}

void com_add_event_handler(event_handler_fn_t handler, void *context, int priority)
{
  event_handler_t *search;
  event_handler_t *node;

  if (handler == NULL)
    return;

  mutex_lock(&g_handler_lock);
  
  node = com_malloc(g_event_alloc, sizeof(*node));
  node->node.list = NULL;
  node->node.next = node->node.prev = NULL;
  node->node.pointer = context;
  node->handler = handler;
  node->priority = priority;

  search = (event_handler_t *)list_first_node(&g_handler_list);
  while (search) {
    if (priority < search->priority) break;
    search = (event_handler_t *)listnode_next((listnode_t *)search);
  }

  if (search)
    list_insert_node_before((listnode_t *)search, (listnode_t *)node);
  else
    list_insert_node_before(&g_handler_list.head, (listnode_t *)node);

  mutex_unlock(&g_handler_lock);
}

void com_remove_event_handler(event_handler_fn_t handler, void *context)
{
  event_handler_t *node;

  mutex_lock(&g_handler_lock);

  node = (event_handler_t *)list_first_node(&g_handler_list);

  if (context == IGNORE_HANDLER_CONTEXT) {
    while (node) {
      if (node->handler == handler) {
        list_remove((listnode_t *)node);
        break;
      }

      node = (event_handler_t *)listnode_next((listnode_t *)node);
    }
  } else {
    while (node) {
      if (node->handler == handler && node->node.pointer == context) {
        list_remove((listnode_t *)node);
        break;
      }

      node = (event_handler_t *)listnode_next((listnode_t *)node);
    }
  }

  mutex_unlock(&g_handler_lock);
}

void com_clear_event_handlers(void)
{
  listnode_t *node;
  listnode_t *term;

  mutex_lock(&g_handler_lock);
  node = g_handler_list.head.next;
  term = &g_handler_list.head;
  
  if (node != term) {
    // detach nodes
    node->prev = NULL;
    term->prev->next = NULL;
  
    // reset list to zero size
    g_handler_list.head.next = g_handler_list.head.prev = term;
    g_handler_list.size = 0;
  } else {
    node = NULL;
  }

  mutex_unlock(&g_handler_lock);

  // free nodes at my leisure
  while (node) {
    listnode_t *next = node->next;
    com_free(g_event_alloc, node);
    node = next;
  }
}

#ifdef __cplusplus
}
#endif // __cplusplus
