/*
  Threading routines
  Written by Noel Cower

  See LICENSE.md for license information
*/

#ifndef __SNOW__THREAD_H__

#define __SNOW__THREAD_H__

#include <snow-config.h>

#ifdef __SNOW__THREAD_C__
#define S_INLINE
#else
#define S_INLINE inline
#endif

#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

typedef void *(*thread_fn_t)(void *context);

#if S_USE_PTHREADS
typedef pthread_t thread_t;

/*! Initializes a thread. */
S_INLINE void thread_create(thread_t *thread, thread_fn_t fn, void *context)
{
  pthread_create(thread, NULL, fn, context);
}

S_INLINE void thread_kill(thread_t thread)
{
  pthread_cancel(thread);
}

S_INLINE int thread_equals(thread_t left, thread_t right)
{
  return pthread_equal(left, right);
}

S_INLINE void thread_detach(thread_t thread)
{
  pthread_detach(thread);
}

S_INLINE void thread_join(thread_t thread, void **return_value)
{
  pthread_join(thread, return_value);
}

S_INLINE void thread_exit(void *return_value)
{
  pthread_exit(return_value);
}

S_INLINE thread_t thread_current_thread(void)
{
  return pthread_self();
}

#else /* S_USE_PTHREADS */

/*! Initializes a thread. */
void thread_create(thread_t *thread, thread_fn_t fn, void *context);
void thread_create(thread_t *thread, thread_fn_t fn, void *context);
void thread_kill(thread_t thread);
int thread_equals(thread_t left, thread_t right);
void thread_detach(thread_t thread);
void thread_join(thread_t thread, void **return_value);
void thread_exit(void *return_value);
thread_t thread_current_thread(void);

#endif

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#include <inline.end>

#endif /* end of include guard: __SNOW__THREAD_H__ */
