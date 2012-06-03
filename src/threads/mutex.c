/*
  Mutex object
  Written by Noel Cower

  See LICENSE.md for license information
*/

#include "mutex.h"

#if S_USE_PTHREADS
#include <errno.h>
#endif

#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

#if S_USE_PTHREADS

/* possible errors */
static const char *c_mutex_err_unknown          = "unknown error";
static const char *c_mutex_err_invalid_mutex    = "mutex value is invalid";
static const char *c_mutex_err_invalid_attr     = "mutex attribute value is invalid";
static const char *c_mutex_err_permission       = "the current thread does not hold a lock on the mutex";
static const char *c_mutex_err_currently_locked = "the mutex is currently locked by another thread";
static const char *c_mutex_err_no_memory_attr   = "the system cannot allocate enough memory for new mutex attributes";
static const char *c_mutex_err_no_memory_mutex  = "the system cannot allocate enough memory for a new mutex";
static const char *c_mutex_err_temp_no_memory   = "the system temporarily lacks the memory for a new mutex";
static const char *c_mutex_err_attr_settype     = "invalid attributes value or mutex type";
static const char *c_mutex_err_deadlock         = "a deadlock will occur if locked";

static pthread_mutexattr_t g_normal_attr;
static pthread_mutexattr_t g_recursive_attr;
static pthread_once_t g_mutex_attr_once = PTHREAD_ONCE_INIT;

static void destroy_mutex_attr(void)
{
  pthread_mutexattr_destroy(&g_normal_attr);
  pthread_mutexattr_destroy(&g_recursive_attr);
}

static void init_mutex_attr(void)
{
  int error = 0;
  
  error = pthread_mutexattr_init(&g_normal_attr);
  if (error) s_log_error("Non-recursive mutex attribute could not be initialized: %s", c_mutex_err_no_memory_attr);
  
  /* init normal mutex attr */
  #if defined(NDEBUG)
  error = pthread_mutexattr_settype(&g_normal_attr, PTHREAD_MUTEX_NORMAL);
  #else
  /*
   * NOTE: maybe later just make this the default regardless of whether
   * debugging or not
   */
  error = pthread_mutexattr_settype(&g_normal_attr, PTHREAD_MUTEX_ERRORCHECK);
  #endif
  if (error) s_log_error("Non-recursive mutex attribute type could not be set: %s", c_mutex_err_attr_settype);
  
  /* init recursive mutex attr */
  error = pthread_mutexattr_init(&g_recursive_attr);
  if (error) s_log_error("Recursive mutex attribute could not be initialized: %s", c_mutex_err_no_memory_attr);
  
  error = pthread_mutexattr_settype(&g_recursive_attr, PTHREAD_MUTEX_RECURSIVE);
  if (error) s_log_error("Recursive mutex attribute type could not be set: %s", c_mutex_err_attr_settype);

  atexit(destroy_mutex_attr);
}

void mutex_init(mutex_t *lock, int recursive)
{
  pthread_mutexattr_t *attr = (recursive ? &g_recursive_attr : &g_normal_attr);
  pthread_once(&g_mutex_attr_once, init_mutex_attr);
  int error = pthread_mutex_init((pthread_mutex_t *)lock, attr);

  if (error) {
    const char *reason = c_mutex_err_unknown;
    
    switch (error) {
    case EINVAL: reason = c_mutex_err_invalid_attr; break;
    case EAGAIN: reason = c_mutex_err_temp_no_memory; break;
    case ENOMEM: reason = c_mutex_err_no_memory_mutex; break;
    default: break;
    }
    
    s_log_error("Error initializing mutex: %s", reason);
  }
}

void mutex_destroy(mutex_t *lock)
{
  int error = pthread_mutex_destroy((pthread_mutex_t *)lock);
  
  if (error) {
    const char *reason = c_mutex_err_unknown;
    switch (error) {
    case EINVAL: reason = c_mutex_err_invalid_mutex; break;
    case EBUSY: reason = c_mutex_err_currently_locked; break;
    default: break;
    }
    s_log_error("Error destroying mutex: %s", reason);
  }
}

void mutex_lock(mutex_t *lock)
{
  int error = pthread_mutex_lock((pthread_mutex_t *)lock);
  
  if (error) {
    const char *reason = c_mutex_err_unknown;
    switch (error) {
    case EINVAL: reason = c_mutex_err_invalid_mutex; break;
    case EDEADLK: reason = c_mutex_err_deadlock; break;
    default: break;
    }
    s_log_error("Error locking mutex (lock): %s", reason);
  }
}

int mutex_trylock(mutex_t *lock)
{
  int error = pthread_mutex_trylock((pthread_mutex_t *)lock);
  
  /* though EBUSY is an error code, in this case, it's not a bad error code */
  if (error != EBUSY) {
    const char *reason = c_mutex_err_unknown;
    switch (error) {
    case EINVAL: reason = c_mutex_err_invalid_mutex; break;
    /* case EBUSY: reason = c_mutex_err_permission; break; */
    default: break;
    }
    s_log_error("Error trying to lock mutex (trylock): %s", reason);
  }
  
  return !error;
}

void mutex_unlock(mutex_t *lock)
{
  int error = pthread_mutex_unlock((pthread_mutex_t *)lock);
  
  if (error) {
    const char *reason = c_mutex_err_unknown;
    switch (error) {
    case EINVAL: reason = c_mutex_err_invalid_mutex; break;
    case EPERM: reason = c_mutex_err_permission; break;
    default: break;
    }
    
    s_log_error("Error unlocking mutex: %s", reason);
  }
}

#else /* S_USE_PTHREADS */

#error "No lock implementation for this platform"

#endif

#if defined(__cplusplus)
}
#endif /* __cplusplus */

