#include "module.h"

int module_init(moduleoption *opt)
{
	debug("Hello world!");
	debug("Weight: %d", option_int(opt, "weight"));

	return(0);
}
