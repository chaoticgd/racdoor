#ifndef _RACDOOR_MODULE_H
#define _RACDOOR_MODULE_H

typedef struct {
	void (*load)();
	void (*unload)();
} Module;

#define MODULE(load, unload) \
	Module _module_##load __attribute__ ((section (".racdoor.modules"))) = {load, unload};

void load_modules();
void unload_modules();

#endif
