#ifndef MUTEX_H

#define MUTEX_H

#include "config.h"

#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

#if USE_PTHREADS

typedef pthread_mutex_t mutex_t;

#else

typedef int mutex_t;

#endif

void mutex_init(mutex_t *lock, int recursive);
void mutex_destroy(mutex_t *lock);

void mutex_lock(mutex_t *lock);
int mutex_trylock(mutex_t *lock);
void mutex_unlock(mutex_t *lock);

#endif /* end of include guard: MUTEX_H */

