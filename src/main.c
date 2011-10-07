#include "config.h"
#include "memory.h"
//#include "entity.h"

static void main_shutdown(void)
{
//	sys_entity_shutdown();
	sys_mem_shutdown();
}

int main(int argc, const char *argv[])
{
	sys_mem_init();
//	sys_entity_init();
	atexit(main_shutdown);
	
	/* do stuff! */

	return 0;
}

