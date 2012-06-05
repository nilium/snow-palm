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
  sys_mem_shutdown();
}

int main(int argc, char *argv[])
{
  sys_time_init();
  sys_mem_init(g_default_allocator);
  sys_tls_init(g_default_allocator);
  sys_entity_init(g_default_allocator);
  sys_events_init(g_default_allocator);

  atexit(main_shutdown);

  sys_main(argc, argv);

  return 0;
}

