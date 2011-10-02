#include "lock.h"

#if USE_PTHREADS
#include <errno.h>
#endif

#if defined(__cplusplus)
extern "C"
{
#endif /* __cplusplus */

#if USE_PTHREADS

/* possible errors */
static const char *lock_err_unknown			= "unknown error";
static const char *lock_err_invalid_mutex	= "mutex value is invalid";
static const char *lock_err_invalid_attr	= "mutex attribute value is invalid";
static const char *lock_err_permission		= "the current thread does not hold a lock on the mutex";
static const char *lock_err_no_memory_attr	= "the system cannot allocate enough memory for new mutex attributes";
static const char *lock_err_no_memory_mutex	= "the system cannot allocate enough memory for a new mutex";
static const char *lock_err_temp_no_memory	= "the system temporarily lacks the memory for a new mutex";
static const char *lock_err_attr_settype	= "invalid attributes value or mutex type";
static const char *lock_err_deadlock		= "a deadlock will occur if locked";

static pthread_mutexattr_t g_normal_attr;
static pthread_once_t g_attr_once;

static void destroy_normal_attr(void)
{
	pthread_mutexattr_destroy(&g_normal_attr);
}

static void init_normal_attr(void)
{
	int error = 0;
	
	error = pthread_mutexattr_init(&g_normal_attr);
	if (error) log_error("Mutex attribute could not be initialized: %s\n", lock_err_no_memory_attr);
	
	#if defined(NDEBUG)
	error = pthread_mutexattr_settype(&g_normal_attr, PTHREAD_MUTEX_NORMAL);
	#else
	// NOTE: maybe later just make this the default regardless of whether
	// debugging or not
	error = pthread_mutexattr_settype(&g_normal_attr, PTHREAD_MUTEX_ERRORCHECK);
	#endif
	if (error) log_error("Mutex attribute type could not be set: %s\n", lock_err_attr_settype);
	atexit(destroy_normal_attr);
}

void lock_init(lock_t *lock)
{
	pthread_once(&g_attr_once, init_normal_attr);
	int error = pthread_mutex_init(lock, &g_normal_attr);

	if (error) {
		const char *reason = lock_err_unknown;
		
		switch (error) {
		case EINVAL: reason = lock_err_invalid_attr; break;
		case EAGAIN: reason = lock_err_temp_no_memory; break;
		case ENOMEM: reason = lock_err_no_memory_mutex; break;
		default: break;
		}
		
		log_error("Error initializing mutex: %s\n", reason);
	}
}

void lock_destroy(lock_t *lock)
{
	int error = pthread_mutex_destroy(lock);
	
	if (error) {
		const char *reason = lock_err_unknown;
		switch (error) {
		case EINVAL: reason = lock_err_invalid_mutex; break;
		case EBUSY: reason = lock_err_permission; break;
		default: break;
		}
		log_error("Error destroying mutex: %s\n", reason);
	}
}

void lock_lock(lock_t *lock)
{
	int error = pthread_mutex_lock(lock);
	
	if (error) {
		const char *reason = lock_err_unknown;
		switch (error) {
		case EINVAL: reason = lock_err_invalid_mutex; break;
		case EDEADLK: reason = lock_err_deadlock; break;
		default: break;
		}
		log_error("Error locking mutex (lock): %s\n", reason);
	}
}

int lock_trylock(lock_t *lock)
{
	int error = pthread_mutex_trylock(lock);
	
	/* though EBUSY is an error code, in this case, it's not a bad error code */
	if (error != EBUSY) {
		const char *reason = lock_err_unknown;
		switch (error) {
		case EINVAL: reason = lock_err_invalid_mutex; break;
		// case EBUSY: reason = lock_err_permission; break;
		default: break;
		}
		log_error("Error trying to lock mutex (tryLock): %s\n", reason);
	}
	
	return !error;
}

void lock_unlock(lock_t *lock)
{
	int error = pthread_mutex_unlock(lock);
	
	if (error) {
		const char *reason = lock_err_unknown;
		switch (error) {
		case EINVAL: reason = lock_err_invalid_mutex; break;
		case EPERM: reason = lock_err_permission; break;
		default: break;
		}
		
		log_error("Error unlocking mutex: %s\n", reason);
	}
}

#else /* USE_PTHREADS */

#error "No lock implementation for this platform"

#endif

#if defined(__cplusplus)
}
#endif /* __cplusplus */

