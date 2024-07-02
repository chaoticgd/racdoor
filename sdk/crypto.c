#include <racdoor/crypto.h>

#include <racdoor/mips.h>
#include <racdoor/util.h>

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

void gen_xor_decryptor(u32* dest, u32 payload, u32 payload_end, u32 entry, u32 key)
{
	u32* ptr = dest;
	
	/* EE hardware errata: Prevent code and data from being too close. */
	*ptr++ = MIPS_NOP();
	*ptr++ = MIPS_NOP();
	*ptr++ = MIPS_NOP();
	*ptr++ = MIPS_NOP();
	*ptr++ = MIPS_NOP();
	
	HOST_CHECK(ptr - dest == DECRYPTOR_ENTRY_OFFSET, "Decryptor code out of sync with DECRYPTOR_ENTRY_OFFSET.\n");
	
	/* Save overwritten registers on the stack. */
	*ptr++ = MIPS_SQ(MIPS_T0, 0x110 - 0x1c0, MIPS_SP);
	*ptr++ = MIPS_SQ(MIPS_T1, 0x10 - 0x1c0, MIPS_SP);
	*ptr++ = MIPS_SQ(MIPS_T2, 0x90 - 0x1c0, MIPS_SP);
	*ptr++ = MIPS_SQ(MIPS_T3, 0xc0 - 0x1c0, MIPS_SP);
	*ptr++ = MIPS_SQ(MIPS_S4, 0x30 - 0x1c0, MIPS_SP);
	
	/* Load immediates. */
	*ptr++ = MIPS_LUI(MIPS_T0, payload >> 16);
	*ptr++ = MIPS_ORI(MIPS_T0, MIPS_T0, payload);
	*ptr++ = MIPS_LUI(MIPS_T1, payload_end >> 16);
	*ptr++ = MIPS_ORI(MIPS_T1, MIPS_T1, payload_end);
	*ptr++ = MIPS_LUI(MIPS_S4, key >> 16);
	*ptr++ = MIPS_ORI(MIPS_S4, MIPS_S4, key);
	*ptr++ = MIPS_ADD(MIPS_T2, MIPS_S4, MIPS_ZERO);
	
	/* Decrypt the payload. */
	*ptr++ = MIPS_SLL(MIPS_T3, MIPS_T2, 4);
	*ptr++ = MIPS_XOR(MIPS_T2, MIPS_T2, MIPS_T3);
	*ptr++ = MIPS_SRL(MIPS_T3, MIPS_T2, 11);
	*ptr++ = MIPS_XOR(MIPS_T2, MIPS_T2, MIPS_T3);
	*ptr++ = MIPS_SLL(MIPS_T3, MIPS_T2, 9);
	*ptr++ = MIPS_XOR(MIPS_T2, MIPS_T2, MIPS_T3);
	
	*ptr++ = MIPS_LWU(MIPS_T3, 0, MIPS_T0);
	*ptr++ = MIPS_XOR(MIPS_T3, MIPS_T3, MIPS_T2);
	
	*ptr++ = MIPS_ADDIU(MIPS_T0, MIPS_T0, 4);
	
	*ptr++ = MIPS_BNE(MIPS_T0, MIPS_T1, -10);
	*ptr++ = MIPS_SW(MIPS_T3, -4, MIPS_T0);
	
	HOST_CHECK(ptr - dest == DECRYPTOR_JUMP_OFFSET, "Decryptor code out of sync with DECRYPTOR_JUMP_OFFSET.\n");
	
	/* Jump to the loader. */
	*ptr++ = MIPS_J(entry);
	*ptr++ = MIPS_NOP();
	
	/* EE hardware errata: Prevent code and data from being too close. */
	*ptr++ = MIPS_NOP();
	*ptr++ = MIPS_NOP();
	*ptr++ = MIPS_NOP();
	*ptr++ = MIPS_NOP();
	*ptr++ = MIPS_NOP();
}
