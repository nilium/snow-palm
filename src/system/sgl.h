/* General header to include OpenGL headers if they're specific to a platform */

#ifndef __SNOW_SGL_H_0003__
#define __SNOW_SGL_H_0003__ 1

#ifdef S_PLATFORM_MAC
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif

// #include <GL/glew.h>

#endif /* end __SNOW_SGL_H_0003__ include guard */
