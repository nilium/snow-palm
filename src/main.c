#include "config.h"
#include "memory.h"
//#include "entity.h"

static void main_shutdown(void)
{
	mem_shutdown();
//	sys_entity_shutdown();
}

int main(int argc, const char *argv[])
{
	mem_init();
//	sys_entity_init();
	atexit(main_shutdown);
	
	/* do stuff! */

	return 0;
}

