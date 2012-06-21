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

static void sys_mount_archives_callback(const char *filename, size_t name_len)
{
  allocator_t *alloc = g_default_allocator;
  const char *real_dir;
  size_t dir_len;
  char *filepath;

  real_dir = PHYSFS_getRealDir(filename);

  if (!real_dir) {
    s_log_note("<%s> is not a real file, not mounting.", filename);
    return;
  }

  dir_len = strlen(real_dir);

  filepath = com_malloc(alloc, dir_len + name_len + 2);

  strncpy(filepath, real_dir, dir_len);

  filepath[dir_len] = '/';
  filepath[dir_len + 1] = '\0';

  strncat(filepath, filename, name_len);
  s_log_note("Mounting archive <%s>.", filepath);

  PHYSFS_mount(filepath, NULL, TRUE);

  com_free(alloc, filepath);
}

static void sys_mount_archives(void)
{
  // based mostly on PhysFS's own source code (right down to that callback)
  // albeit with hardcoded stuff because I'm lazy.
  char **filenames;
  char **iter;

  filenames = PHYSFS_enumerateFiles("/");

  for (iter = filenames; *iter != NULL; ++iter) {
    size_t name_len = strlen(*iter);

    if (name_len <= 4 || (*iter)[name_len - 4] != '.')
      continue;
    else if (strncasecmp(*iter + (name_len - 3), "zip", 3) == 0) {
      sys_mount_archives_callback(*iter, name_len);
    }
  }

  PHYSFS_freeList(filenames);
}

int sys_init()
{
  s_log_note("Initializing");

  sys_mount_archives();

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

