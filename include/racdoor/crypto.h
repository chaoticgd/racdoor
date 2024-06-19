#ifndef _RACDOOR_CRYPTO_H
#define _RACDOOR_CRYPTO_H

#include <racdoor/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Military grade encryption algorithm. */

void xor_crypt(u32* begin, u32* end, u32 key);
u32 gen_xor_decryptor(u32* dest, u32 payload, u32 payload_end, u32 key, u32 next_stage);

#ifdef __cplusplus
}
#endif

#endif
