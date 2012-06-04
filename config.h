#ifndef CONFIG_H

#define CONFIG_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* define NULL ifndef */
#if !defined(NULL)
#	if defined(__cplusplus)
#		define NULL 0
#	else /* defined(__cplusplus) */
#		define NULL ((void *)0)
#	endif /* !__cplusplus */
#endif /* !defined(NULL) */

/* set up some macros for platforms */
#define PLATFORM_UNIX (defined(unix) || defined(__unix) || defined(__unix__))
#define PLATFORM_APPLE (defined(__APPLE__))
#define PLATFORM_WINDOWS (defined(_WIN32) || defined(__MINGW32__))
#define PLATFORM_LINUX (defined(__linux__) || defined(linux) || defined(__linux))

#if !defined(USE_PTHREADS)
#define USE_PTHREADS (PLATFORM_UNIX || PLATFORM_APPLE || PLATFORM_LINUX)
#endif /* !defined(USE_PTHREADS) */

#if USE_PTHREADS
#include <pthread.h>
#endif /* USE_PTHREADS */

#include "logging.h"

#endif /* end of include guard: CONFIG_H */
