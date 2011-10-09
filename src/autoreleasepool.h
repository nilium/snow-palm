#ifndef AUTORELEASEPOOL_H

#define AUTORELEASEPOOL_H

#include "config.h"
#include "object.h"

#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

/** Autorelease pools depend on the object, memory, and TLS systems. */

void autoreleasepool_push();
void autoreleasepool_addObject(object_t *object);
void autoreleasepool_pop();

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* end of include guard: AUTORELEASEPOOL_H */