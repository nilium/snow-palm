#include "../time.h"
#import <QuartzCore/QuartzCore.h>

static s_time_t g_root_time = 0;

void sys_time_init(void) {
  g_root_time = CACurrentMediaTime();
}

s_time_t current_time(void) {
  return (CACurrentMediaTime() - g_root_time);
}
