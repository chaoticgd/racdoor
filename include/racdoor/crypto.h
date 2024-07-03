#ifndef _RACDOOR_CRYPTO_H
#define _RACDOOR_CRYPTO_H

#include <racdoor/util.h>

#define DECRYPTOR_ENTRY_OFFSET 5
#define DECRYPTOR_KEY_HIGH_OFFSET 13
#define DECRYPTOR_KEY_LOW_OFFSET 18
#define DECRYPTOR_SIZE 38

/* Encrypt/decrypt a buffer of data. */
void xor_crypt(u32* begin, u32* end, u32 key);

/* Extract the key from the decryptor. */
u32 extract_key(u32* decryptor);

/* Generate the MIPS code for the decryptor. */
void gen_xor_decryptor(u32* dest, u32 payload, u32 payload_end, u32 key, u32 next_stage);

#endif
