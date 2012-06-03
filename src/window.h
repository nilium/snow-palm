/*
  Window creation/destruction routines
  Written by Noel Cower

  See LICENSE.md for license information
*/

#ifndef WINDOW_H

#define WINDOW_H

#include <snow-config.h>

#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

bool window_create(void);
void window_destroy(void);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* end of include guard: WINDOW_H */
