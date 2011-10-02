#ifndef LOCK_H

#define LOCK_H

#include "config.h"

#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

#if USE_PTHREADS

typedef pthread_mutex_t lock_t;

#else

typedef int lock_t;

#endif

void lock_init(lock_t *lock);
void lock_destroy(lock_t *lock);

void lock_lock(lock_t *lock);
int lock_trylock(lock_t *lock);
void lock_unlock(lock_t *lock);

#endif /* end of include guard: LOCK_H */

