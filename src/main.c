/*
  Snow entrypoint
  Written by Noel Cower

  See LICENSE.md for license information
*/

#include <snow-config.h>
#include <system/system.h>
#include <memory/memory.h>
#include <entity.h>
#include <threads/threadstorage.h>
#include <events/events.h>
#include <time/time.h>

static void main_shutdown(void)
{
  sys_events_shutdown();
  sys_entity_shutdown();
  sys_tls_shutdown();
  sys_pool_shutdown();

  if ( ! PHYSFS_deinit()) {
    const char *error = PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode());
    s_log_error("Error shutting down PhysFS: %s", error);
  }
}

int main(int argc, const char *argv[])
{
  PHYSFS_init(argv[0]);
  sys_time_init();
  sys_pool_init(g_default_allocator);
  sys_tls_init(g_default_allocator);
  sys_entity_init(g_default_allocator);
  sys_events_init(g_default_allocator);

  atexit(main_shutdown);

  sys_main(argc, argv);

  return 0;
}

