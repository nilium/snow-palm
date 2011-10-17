/*
	Logging routines / macros
	Written by Noel Cower

	See LICENSE.md for license information
*/

#include "logging.h"
#include <stdarg.h>
#include <stdlib.h>

#if defined(__cplusplus)
extern "C"
{
#endif

void log_fatal_impl(const char *format, int error, ...)
{
	va_list args;
	va_start(args, error);
#if PLATFORM_TOUCHPAD
	int length = vsprintf(format, args) + 1;
	char *str = (char *)malloc(size(char) * length);
	va_end(args);
	va_start(args, error);
	vsnprintf(str, length, format, args);
	va_end(args);
	PDL_Log("%s", str);
	free(str);
	PDL_Log("Exiting with error code %d\n", error);
#else
	vfprintf(stderr, format, args);
	va_end(args);
	fprintf(stderr, "Exiting with error code %d\n", error);
	fflush(stderr);
#endif
	exit(error);
} /* log_fatal */

#if defined(__cplusplus)
}
#endif

