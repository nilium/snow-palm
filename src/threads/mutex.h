/*
  Mutex object
  Written by Noel Cower

  See LICENSE.md for license information
*/

#ifndef MUTEX_H

#define MUTEX_H

#include <snow-config.h>

#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

#if S_USE_PTHREADS

typedef pthread_mutex_t mutex_t;

#else

typedef int mutex_t;

#endif

void mutex_init(mutex_t *lock, bool recursive);
void mutex_destroy(mutex_t *lock);

void mutex_lock(mutex_t *lock);
bool mutex_trylock(mutex_t *lock);
void mutex_unlock(mutex_t *lock);

#if defined(__cplusplus)
}
#endif

#endif /* end of include guard: MUTEX_H */

