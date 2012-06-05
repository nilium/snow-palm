#include "system.h"
#include "sgl.h"
#include <events/events.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

static bool event_foo(event_t *event, void *context)
{
  (void)context;
  if (!event) {
    s_fatal_error(1, "Received NULL event");
    return false;
  }
  switch (event->kind) {
    case EVENT_KEYBOARD:
      if (event->key.character == L'q' && !event->key.pressed) {
        sys_terminate();
        return true;
      }
    default:
    break;
  }
  return false;
}

int sys_init()
{
  s_log_note("Initializing");

  com_add_event_handler(event_foo, NULL, 0);

  glClearColor(0.2, 0.3, 0.4, 1.0);

  return 0;
}

int sys_frame(const s_time_t frame_time)
{
  glClear(GL_COLOR_BUFFER_BIT);
  com_process_event_queue();

  return 0;
}

void sys_quit()
{

}

#ifdef __cplusplus
}
#endif /* __cplusplus */

