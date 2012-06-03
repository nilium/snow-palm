/*
	Autorelease pool routines
	Written by Noel Cower

	See LICENSE.md for license information
*/

#ifndef AUTORELEASEPOOL_H

#define AUTORELEASEPOOL_H

#include <snow-config.h>
#include "../object.h"

#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

/** Autorelease pools depend on the object, memory, and TLS systems. */

void autoreleasepool_push(void);
void autoreleasepool_add_object(object_t *object);
void autoreleasepool_pop(void);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* end of include guard: AUTORELEASEPOOL_H */
