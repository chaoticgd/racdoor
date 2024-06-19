#include <racdoor/mips.h>
#include <racdoor/util.h>

void xor32(u32* begin, u32* end, u32 key)
{
	for (u32* ptr = begin; ptr < end; ptr++)
	{
		printf("%x\n", key);
		u32 value = *ptr ^ key;
		*ptr = value;
		key ^= key >> 7;
		key ^= key << 9;
		key ^= key >> 13;
	}
}

u32 gen_xor_decryptor(u32* dest, u32 image, u32 image_end, u32 key, u32 next_stage)
{
	u32* ptr = dest;
	
	/* EE hardware errata: Prevent code and data from being too close. */
	*ptr++ = MIPS_NOP();
	*ptr++ = MIPS_NOP();
	*ptr++ = MIPS_NOP();
	*ptr++ = MIPS_NOP();
	*ptr++ = MIPS_NOP();
	
	/* Save overwritten registers on the stack. */
	*ptr++ = MIPS_SQ(MIPS_T0, 0x110 - 0x1c0, MIPS_SP);
	*ptr++ = MIPS_SQ(MIPS_T1, 0x10 - 0x1c0, MIPS_SP);
	*ptr++ = MIPS_SQ(MIPS_T2, 0x90 - 0x1c0, MIPS_SP);
	*ptr++ = MIPS_SQ(MIPS_T3, 0xc0 - 0x1c0, MIPS_SP);
	
	/* Load immediates . */
	*ptr = MIPS_LUI(MIPS_T0, image >> 16);
	*ptr = MIPS_ORI(MIPS_T0, MIPS_T0, image);
	*ptr = MIPS_LUI(MIPS_T1, image_end >> 16);
	*ptr = MIPS_ORI(MIPS_T1, MIPS_T1, image_end);
	*ptr = MIPS_LUI(MIPS_T2, key >> 16);
	*ptr = MIPS_ORI(MIPS_T2, MIPS_T2, key);
	
	/* Decrypt the image. */
	
	
	/* Jump to the loader. */
	*ptr++ = MIPS_J(next_stage);
	*ptr++ = MIPS_NOP();
	
	/* EE hardware errata: Prevent code and data from being too close. */
	*ptr++ = MIPS_NOP();
	*ptr++ = MIPS_NOP();
	*ptr++ = MIPS_NOP();
	*ptr++ = MIPS_NOP();
	
	return ptr - dest;
}
