#ifndef __SNOW_SYSTEM_H__
#define __SNOW_SYSTEM_H__ 1

#include <snow-config.h>
#include <time/time.h>

void sys_main(int argc, const char *argv[]);

// routine that initializes the game/app
// returns 0 if successful
int sys_init();
// routine that runs a frame in the game/app
// returns 0 if successful
int sys_frame(const s_time_t frame_time);
// routine that prepares the app/game to shut down
// has no success/failure conditions - if the engine crashes at this point, it
// was going down anyway, just make sure to log it
void sys_quit();

// tells the app/game to shut down right now
void sys_terminate();

#endif /* end __SNOW_SYSTEM_H__ include guard */
