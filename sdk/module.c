#include <racdoor/module.h>

#include <racdoor/transition.h>

extern Module _racdoor_modules;
extern Module _racdoor_modules_end;

void load_modules()
{
	install_transition_hooks();
	
	for (Module* module = &_racdoor_modules; module < &_racdoor_modules_end; module++)
		if (module->load)
			module->load();
}

void unload_modules()
{
	for (Module* module = &_racdoor_modules; module < &_racdoor_modules_end; module++)
		if (module->unload)
			module->unload();
}
