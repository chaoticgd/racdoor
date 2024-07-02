#ifndef _RACDOOR_CRYPTO_H
#define _RACDOOR_CRYPTO_H

#include <racdoor/util.h>

/* Military grade encryption algorithm. */

#define DECRYPTOR_ENTRY_OFFSET 5
#define DECRYPTOR_JUMP_OFFSET 28

void xor_crypt(u32* begin, u32* end, u32 key);
void gen_xor_decryptor(u32* dest, u32 payload, u32 payload_end, u32 key, u32 next_stage);

#endif
