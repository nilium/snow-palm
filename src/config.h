/*
	Configuration macros & project-wide includes
	Written by Noel Cower

	See LICENSE.md for license information
*/

#ifndef CONFIG_H

#define CONFIG_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

/* define NULL ifndef */
#if !defined(NULL)
#	if defined(__cplusplus)
#		define NULL 0
#	else /* defined(__cplusplus) */
#		define NULL ((void *)0)
#	endif /* !__cplusplus */
#endif /* !defined(NULL) */

/* Define YES/NO constants to map 1 and 0 respectively.
   These are an Apple-ism, and I like that they can contribute to code
   readability in some odd ways.  On the off chance that Cocoa.h is included
   before config.h in a Mac OS build, we can just assume YES/NO are 1 and 0 or
   at least mapped similarly.
*/
#if !defined(YES)
#	define YES (1)
#endif

#if !defined(NO)
#	define NO (0)
#endif

/* set up some macros for platforms */
#define PLATFORM_UNIX (defined(unix) || defined(__unix) || defined(__unix__))
#define PLATFORM_APPLE (defined(__APPLE__))
#define PLATFORM_WINDOWS (defined(_WIN32) || defined(__MINGW32__))
#define PLATFORM_LINUX (defined(__linux__) || defined(linux) || defined(__linux))

/* architectures */
#define ARCH_ARM (__arm__)
#define ARCH_ARM_NEON (ARCH_ARM && __ARM_NEON__)
#define ARCH_ARM_7 (ARCH_ARM && __ARM_ARCH_7A__)
#define ARCH_x86_64 (__x86_64__ || __x86_64 || __amd64__ || __amd64 || _M_X64)
#define ARCH_x86 (__i386 || __i386__ || i386 || _M_IX86 || _X86_ || __i486__ || __i586 || __i686__)
#define ARCH_PPC (__powerpc || __powerpc__ || __POWERPC__ || __ppc__ || _M_PPC)

#if (PLATFORM_UNIX || PLATFORM_APPLE) && !defined(__USE_UNIX98)
#	define __USE_UNIX98
#endif

/* specify the use of pthreads on supported platforms */
#if !defined(USE_PTHREADS)
#	define USE_PTHREADS (PLATFORM_UNIX || PLATFORM_APPLE || PLATFORM_LINUX)
#endif /* !defined(USE_PTHREADS) */

#if USE_PTHREADS
#	include <pthread.h>
#else
#	error "No threading API available"
#endif /* USE_PTHREADS */

#include "logging.h"

#endif /* end of include guard: CONFIG_H */

