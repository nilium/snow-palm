/*
	Threading routines
	Written by Noel Cower

	See LICENSE.md for license information
*/

#ifndef THREAD_H

#define THREAD_H

#include "config.h"

#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

typedef void *(*thread_fn_t)(void *context);

#if USE_PTHREADS
typedef pthread_t thread_t;

/*! Initializes a thread. */
inline void thread_create(thread_t *thread, thread_fn_t fn, void *context)
{
	pthread_create(thread, NULL, fn, context);
}

inline void thread_kill(thread_t thread)
{
	pthread_cancel(thread);
}

inline int thread_equals(thread_t left, thread_t right)
{
	return pthread_equal(left, right);
}

inline void thread_detach(thread_t thread)
{
	pthread_detach(thread);
}

inline void thread_join(thread_t thread, void **return_value)
{
	pthread_join(thread, return_value);
}

inline void thread_exit(void *return_value)
{
	pthread_exit(return_value);
}

inline thread_t thread_current_thread(void)
{
	return pthread_self();
}

#else /* USE_PTHREADS */

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

#endif /* end of include guard: THREAD_H */
