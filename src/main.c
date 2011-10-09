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

static void main_shutdown(void)
{
	sys_entity_shutdown();
	sys_tls_shutdown();
    sys_object_shutdown();
	sys_mem_shutdown();
}

int main(int argc, const char *argv[])
{
	sys_mem_init();
    sys_object_init();
	sys_tls_init();

	autoreleasepool_push();

	sys_entity_init();
	atexit(main_shutdown);


	entity_t *entity = entity_new("foo", NULL);
	entity_t *child = entity_new("bar", entity);
	entity_t *child2 = entity_new("baz", entity);
	object_release(entity);
	object_release(child);
	object_release(child2);
	
	/* do stuff! */

	autoreleasepool_pop();

	return 0;
}

