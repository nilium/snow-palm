/*
  Snow entrypoint
  Written by Noel Cower

  See LICENSE.md for license information
*/

#include <snow-config.h>
#include "memory.h"
#include "entity.h"
#include "autoreleasepool.h"
#include "threadstorage.h"
#include "window.h"

#include <SDL/SDL.h>
#include <GLES2/gl2.h>

static void main_shutdown(void)
{
  autoreleasepool_pop();
  window_destroy();

  sys_entity_shutdown();
  sys_tls_shutdown();
    sys_object_shutdown();
  sys_mem_shutdown();
}

int main(int argc, char **argv)
{
  sys_mem_init();
    sys_object_init();
  sys_tls_init();

  autoreleasepool_push();

  sys_entity_init();
  atexit(main_shutdown);

  window_create();
  
  { /* main block */
    SDL_Event event;
    bool running = true;
    bool waiting = false;

    glClearColor(0.25, 0.3, 0.45, 1.0);

    while (running) {
      int eventRet;

      autoreleasepool_push();

      if (waiting) {
        eventRet = SDL_WaitEvent(&event);
      } else {
        eventRet = SDL_PollEvent(&event);
      }

      while (eventRet) {
        switch (event.type) {
        case SDL_QUIT:
          running = false;
        break;
        case SDL_ACTIVEEVENT:
          if (event.active.state == SDL_APPACTIVE)
            waiting = !event.active.gain;
        break;
        default:
        break;
        }

        eventRet = SDL_PollEvent(&event);
      } /* eventRet */

      glClear(GL_COLOR_BUFFER_BIT);
      /* rendering */
      SDL_GL_SwapBuffers();

      autoreleasepool_pop();
    } /* while(running) */
  } /* main block */

  return 0;
}

