#define __SNOW__SYSTEM_C__

#include "system.h"
#include "sgl.h"
#include <events/events.h>
#include <threads/mutex.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

mutex_t g_system_locks[SYS_LOCK_COUNT];

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
        case EVENT_WINDOW_CLOSE:
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
  size_t lockidx;

  s_log_note("Initializing");

  for (lockidx = 0; lockidx < SYS_LOCK_COUNT; ++lockidx)
    mutex_init(&g_system_locks[lockidx], false);

  sys_mount_archives();

  com_add_event_handler(event_foo, NULL, 0);

  glClearColor(0.2, 0.3, 0.4, 1.0);

  return 0;
}

int sys_frame(const s_time_t frame_time)
{
  sys_lock(SYS_LOCK_FRAME);

  glClear(GL_COLOR_BUFFER_BIT);
  com_process_event_queue();

  sys_unlock(SYS_LOCK_FRAME);
  return 0;
}

void sys_quit()
{

}

int sys_lock(unsigned int lock)
{
  if (SYS_LOCK_COUNT <= lock) {
    s_log_error("Invalid system lock");
    return -1;
  }

  return mutex_lock(&g_system_locks[lock]);
}

int sys_unlock(unsigned int lock)
{
  if (SYS_LOCK_COUNT <= lock) {
    s_log_error("Invalid system lock");
    return -1;
  }

  return mutex_unlock(&g_system_locks[lock]);
}

int sys_trylock(unsigned int lock)
{
  if (SYS_LOCK_COUNT <= lock) {
    s_log_error("Invalid system lock");
    return -1;
  }

  return mutex_trylock(&g_system_locks[lock]);
}


#ifdef __cplusplus
}
#endif /* __cplusplus */

