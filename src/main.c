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
  sys_tls_shutdown();
  sys_pool_shutdown();
}

int main(int argc, const char *argv[])
{
  sys_time_init();
  sys_pool_init(g_default_allocator);
  sys_tls_init(g_default_allocator);
  sys_events_init(g_default_allocator);

  atexit(main_shutdown);

  sys_main(argc, argv);

  return 0;
}

