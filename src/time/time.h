#ifndef __TIME_HH__
#define __TIME_HH__ 1

#include <snow-config.h>
#include <maths/maths.h>

// Initialize timing
void sys_time_init(void);

// Returns the number of seconds since the program started
s_time_t current_time(void);

// Returns the frames per second for time elapsed during the duration
inline s_float_t get_fps(s_time_t duration, size_t frames) {
  if (duration < 0 || float_is_zero(duration))
    return 0;
  return frames / duration;
}

#endif /* end __TIME_HH__ include guard */
