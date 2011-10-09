/*
	Snow entrypoint
	Written by Noel Cower

	See LICENSE.md for license information
*/

#include "config.h"
#include "memory.h"
#include "entity.h"
#include "autoreleasepool.h"
#include "threadstorage.h"

#include <SDL/SDL.h>
#include <GLES2/gl2.h>

static void main_shutdown(void)
{
	sys_entity_shutdown();
	sys_tls_shutdown();
    sys_object_shutdown();
	sys_mem_shutdown();
}

int main(int argc, char **argv)
{
	sys_mem_init();
    sys_object_init();
	sys_tls_init();

	autoreleasepool_push();

	sys_entity_init();
	atexit(main_shutdown);

	window_create();
	
	SDL_Event event;
	bool running = true;

	glClearColor(0.25, 0.3, 0.45, 1.0);

	while (running) {
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				running = false;
			break;
			default:
			break;
			}
		}

		glClear(GL_COLOR_BUFFER_BIT);
		SDL_GL_SwapBuffers();
	}

	/* do stuff! */

	autoreleasepool_pop();
	window_destroy();

	return 0;
}

