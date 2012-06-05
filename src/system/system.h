#ifndef __SNOW_SYSTEM_H__
#define __SNOW_SYSTEM_H__ 1

#include <snow-config.h>
#include <time/time.h>

// Implemented in platform-specific code
void sys_main(int argc, const char *argv[]);

// Routine that initializes the game/app.
// Returns 0 if successful, otherwise it's an error code.
// GL context will be available for this call.
int sys_init();
// Routine that runs a frame in the game/app.
// Returns 0 if successful, otherwise it's an error code.
// GL cohntext will be available for this call.
int sys_frame(const s_time_t frame_time);
// Routine that prepares the app/game to shut down.
// Has no success/failure conditions - if the engine crashes at this point, it
// was going down anyway, just make sure to log it.
void sys_quit();

// Tells the app/game to shut down right now
// Implemented in platform-specific code
void sys_terminate();

#endif /* end __SNOW_SYSTEM_H__ include guard */
