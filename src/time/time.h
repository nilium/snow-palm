#ifndef __SNOW__TIME_H__
#define __SNOW__TIME_H__ 1

#include <snow-config.h>
#include <maths/maths.h>

typedef double s_time_t;

#ifdef __SNOW__TIME_C__
#define S_INLINE
#else
#define S_INLINE inline
#endif

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// Initialize timing
void sys_time_init(void);

// Returns the number of seconds since the program started
s_time_t current_time(void);

// Returns the frames per second for time elapsed during the duration
S_INLINE s_float_t get_fps(s_time_t duration, size_t frames) {
  if (duration < 0 || float_is_zero(duration))
    return 0;
  return frames / duration;
}

#ifdef __cplusplus
}
#endif // __cplusplus

#include <inline.end>

#endif /* end __SNOW__TIME_H__ include guard */
