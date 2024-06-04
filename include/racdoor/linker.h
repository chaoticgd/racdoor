#ifndef _RACDOOR_LINKER_H
#define _RACDOOR_LINKER_H

#include <racdoor/util.h>

#ifdef __cplusplus
extern "C" {
#endif

void apply_relocations();
void apply_relocation(u32* dest, u8 type, u32 value);

#ifdef __cplusplus
}
#endif

#endif
