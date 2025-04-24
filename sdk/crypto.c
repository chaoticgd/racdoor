/* This file is part of Racdoor.
   Copyright (c) 2024 chaoticgd. All rights reversed.
   Released under the BSD-1-Clause license. */

#include <racdoor/crypto.h>

#include <racdoor/mips.h>
#include <racdoor/util.h>

/* Military grade encryption algorithm. */

void xor_crypt(u32* begin, u32* end, u32 key)
{
	for (u32* ptr = begin; ptr < end; ptr++)
	{
		key ^= key << 4;
		key ^= key >> 11;
		key ^= key << 9;
		*ptr ^= key;
	}
}

u32 extract_key(u32* decryptor)
{
	return ((decryptor[DECRYPTOR_KEY_HIGH_OFFSET] & 0xffff) << 16) | (decryptor[DECRYPTOR_KEY_LOW_OFFSET] & 0xffff);
}

#ifdef _HOST

void gen_xor_decryptor(u32* dest, u32 payload, u32 payload_end, u32 entry, u32 key)
{
	u32* ptr = dest;
	
	/* EE hardware errata: Prevent code and data from being too close. */
	*ptr++ = MIPS_NOP();
	*ptr++ = MIPS_NOP();
	*ptr++ = MIPS_NOP();
	*ptr++ = MIPS_NOP();
	*ptr++ = MIPS_NOP();
	
	CHECK(ptr - dest == DECRYPTOR_ENTRY_OFFSET, "Decryptor code out of sync with DECRYPTOR_ENTRY_OFFSET.");
	
	/* Save overwritten registers on the stack. */
	*ptr++ = MIPS_ADDIU(MIPS_SP, MIPS_SP, -0x1c0);
	*ptr++ = MIPS_SQ(MIPS_A0, 0xf0, MIPS_SP);
	*ptr++ = MIPS_SQ(MIPS_S5, 0x40, MIPS_SP);
	*ptr++ = MIPS_SQ(MIPS_T2, 0x90, MIPS_SP);
	*ptr++ = MIPS_SQ(MIPS_T7, 0x160, MIPS_SP);
	*ptr++ = MIPS_SQ(MIPS_S4, 0x30, MIPS_SP);
	*ptr++ = MIPS_NOP();
	
	/* Load immediates. */
	*ptr++ = MIPS_LUI(MIPS_S5, payload_end >> 16);
	
	CHECK(ptr - dest == DECRYPTOR_KEY_HIGH_OFFSET, "Decryptor code out of sync with DECRYPTOR_KEY_HIGH_OFFSET.");
	*ptr++ = MIPS_LUI(MIPS_T2, key >> 16);
	
	*ptr++ = MIPS_LUI(MIPS_A0, payload >> 16);
	*ptr++ = MIPS_NOP();
	*ptr++ = MIPS_ORI(MIPS_A0, MIPS_A0, payload);
	*ptr++ = MIPS_ORI(MIPS_S5, MIPS_S5, payload_end);
	
	CHECK(ptr - dest == DECRYPTOR_KEY_LOW_OFFSET, "Decryptor code out of sync with DECRYPTOR_KEY_LOW_OFFSET.");
	*ptr++ = MIPS_ORI(MIPS_T2, MIPS_T2, key);
	*ptr++ = MIPS_NOP();
	
	/* Decrypt the payload. */
	*ptr++ = MIPS_SLL(MIPS_T7, MIPS_T2, 4);
	*ptr++ = MIPS_XOR(MIPS_T2, MIPS_T2, MIPS_T7);
	*ptr++ = MIPS_SRL(MIPS_T7, MIPS_T2, 11);
	*ptr++ = MIPS_XOR(MIPS_T2, MIPS_T2, MIPS_T7);
	*ptr++ = MIPS_SLL(MIPS_T7, MIPS_T2, 9);
	*ptr++ = MIPS_XOR(MIPS_T2, MIPS_T2, MIPS_T7);
	
	*ptr++ = MIPS_LWU(MIPS_T7, 0, MIPS_A0);
	*ptr++ = MIPS_XOR(MIPS_T7, MIPS_T7, MIPS_T2);
	
	*ptr++ = MIPS_ADDIU(MIPS_A0, MIPS_A0, 4);
	
	*ptr++ = MIPS_BNE(MIPS_A0, MIPS_S5, -10);
	*ptr++ = MIPS_SW(MIPS_T7, -4, MIPS_A0);
	
	/* Jump to the loader. */
	*ptr++ = MIPS_J(entry);
	*ptr++ = MIPS_NOP();
	
	/* EE hardware errata: Prevent code and data from being too close. */
	*ptr++ = MIPS_NOP();
	*ptr++ = MIPS_NOP();
	*ptr++ = MIPS_NOP();
	*ptr++ = MIPS_NOP();
	*ptr++ = MIPS_NOP();
	
	CHECK(ptr - dest == DECRYPTOR_SIZE, "Decryptor code out of sync with DECRYPTOR_SIZE.");
}

#endif
