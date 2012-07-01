/*
  Mutex object
  Written by Noel Cower

  See LICENSE.md for license information
*/

#ifndef __SNOW__MUTEX_H__

#define __SNOW__MUTEX_H__

#include <snow-config.h>

#ifdef __SNOW__MUTEX_C__
#define S_INLINE
#else
#define S_INLINE inline
#endif

#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

#if S_USE_PTHREADS

typedef pthread_mutex_t mutex_t;

#else

typedef int mutex_t;

#endif

/* All mutex routines return 0 on success, negative values on failure. */

int mutex_init(mutex_t *lock, bool recursive);
int mutex_destroy(mutex_t *lock);

int mutex_lock(mutex_t *lock);
/* trylock returns 1 on a failure to lock (which isn't an error), returns
  0 on success (meaning the lock was acquired). */
int mutex_trylock(mutex_t *lock);
int mutex_unlock(mutex_t *lock);

#if defined(__cplusplus)
}
#endif

#include <inline.end>

#endif /* end of include guard: __SNOW__MUTEX_H__ */

