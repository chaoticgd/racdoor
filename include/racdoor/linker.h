#ifndef _RACDOOR_LINKER_H
#define _RACDOOR_LINKER_H

#include <racdoor/util.h>

void apply_relocations();
void apply_relocation(u32* dest, u8 type, u32 value);

#endif
