/*
  Window creation/destruction routines
  Written by Noel Cower

  See LICENSE.md for license information
*/

#include "window.h"
#include <SDL/SDL.h>

#if PLATFORM_TOUCHPAD
#include <PDL.h>
#endif

#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

static struct {
  SDL_Surface *surface;
} g_wnd;

bool window_create(void)
{
  /* init SDL and PDL */
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE);
  PDL_Init(0);
  
  /* prep for OpenGL ES 2 */
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  
  g_wnd.surface = SDL_SetVideoMode(0, 0, 0, SDL_OPENGL);

  return true;
}

void window_destroy(void)
{
  PDL_Quit();
  SDL_Quit();
}

#if defined(__cplusplus)
}
#endif /* __cplusplus */
