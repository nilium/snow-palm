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

void log_fatal_(const char *format, int error, ...)
{
	va_list args;
	va_start(args, error);
	vfprintf(stderr, format, args);
	va_end(args);
	fprintf(stderr, "Exiting with error code %d\n", error);
	fflush(stderr);
	exit(error);
} /* log_fatal */

#if defined(__cplusplus)
}
#endif

