#ifndef _RACDOOR_IMAGE_H
#define _RACDOOR_IMAGE_H

#include <racdoor/util.h>

void xor32(u32* begin, u32* end, u32 key);
u32 gen_xor_decryptor(u32* dest, u32 image, u32 image_end, u32 key, u32 next_stage);

#endif
