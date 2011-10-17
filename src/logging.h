/*
	Logging routines / macros
	Written by Noel Cower

	See LICENSE.md for license information
*/

#ifndef LOG_H_B31TON6M
#define LOG_H_B31TON6M

#include <stdio.h>

#if PLATFORM_TOUCHPAD
#include <PDL.h>
#endif

/*! Force the inclusion of logging macros regardless of release/debug mode. */
#if !defined(FORCE_LOGGING)
#define FORCE_LOGGING 0
#endif

#if defined(__cplusplus)
extern "C"
{
#endif

/*! Writes an error message to stderr then kills the program with the given
 *  error code.  This should be used only when an error absolutely cannot be
 *  recovered from.  It should be obvious, but this function does not return.
 *  Preferably, you should use the ::log_fatal macro to additionally pass file
 *  and line number information along with the error message.
 *
 *  @param[in] format The format for the error message.
 *  @param[in] error The error code to exit with.
 */
void log_fatal_impl(const char *format, int error, ...);

/*! \brief Macro around ::log_fatal_ to pass in additional file, line number,
	and callee information to the format string.  This is never a no-op.
*/
#define log_fatal(FORMAT, ERROR, args...) log_fatal_impl("Fatal Error [%s:%s:%d]: " FORMAT, (ERROR), __FILE__, __FUNCTION__, __LINE__, ##args)

#if PLATFORM_TOUCHPAD
#define log(STR, args...) PDL_Log((STR), ##args)
#else
#define log(STR, args...) fprintf(stderr, (STR), ##args)
#endif

/* if debugging or logging is forcibly enabled, then provide additional logging macros */
#if !defined(NDEBUG) || FORCE_LOGGING
#if !defined(log_error)
#define log_error(FORMAT, args...)   log("Error [%s:%s:%d]: "   FORMAT, __FILE__, __FUNCTION__, __LINE__, ##args)
#endif
#if !defined(log_warning)
#define log_warning(FORMAT, args...) log("Warning [%s:%s:%d]: " FORMAT, __FILE__, __FUNCTION__, __LINE__, ##args)
#endif
#if !defined(log_note)
#define log_note(FORMAT, args...)    log("Note [%s:%s:%d]: "    FORMAT, __FILE__, __FUNCTION__, __LINE__, ##args)
#endif
#else
/* unless previously defined, log macros do nothing in release builds */
#if !defined(log_error)
#define log_error(FORMAT, args...) 
#endif
#if !defined(log_warning)
#define log_warning(FORMAT, args...) 
#endif
#if !defined(log_note)
#define log_note(FORMAT, args...) 
#endif
#endif

/*! \def log_error(FORMAT, args...)
	\brief Macro to write an error message with the given format and parameters
	to stderr.  Includes file, line number, and callee information in the format
	string automatically.
	
	In builds with NDEBUG defined, this will be a no-op.
*/

/*! \def log_warning(FORMAT, args...)
	\brief Macro to write an warning message with the given format and parameters
	to stderr.  Includes file, line number, and callee information in the format
	string automatically.
	
	In builds with NDEBUG defined, this will be a no-op.
*/

/*! \def log_note(FORMAT, args...)
	\brief Macro to write a general message with the given format and parameters
	to stderr.  Includes file, line number, and callee information in the format
	string automatically.
	
	In builds with NDEBUG defined, this will be a no-op.
*/

#if defined(__cplusplus)
}
#endif

#endif /* end of include guard: LOG_H_B31TON6M */
