#include <racdoor/linker.h>

#include <racdoor/elf.h>
#include <racdoor/mips.h>

#ifdef _EE

extern u8 _racdoor_levelmap;
extern u32 _racdoor_addrtbl;
extern RacdoorRelocation _racdoor_relocs;
extern RacdoorRelocation _racdoor_relocs_end;

extern int Level;

extern void FlushCache(int mode);

void apply_relocations()
{
	/* The addresses we want to apply will change depending on which level
	   overlay is currently loaded, so we calculate which address table we want
	   to use here. */
	u8 overlay_index = (&_racdoor_levelmap)[Level];
	u32 symbol_count = _racdoor_addrtbl;
	u32* table = &_racdoor_addrtbl + 1 + overlay_index * symbol_count;
	
	for (RacdoorRelocation* reloc = &_racdoor_relocs; reloc < &_racdoor_relocs_end; reloc++)
	{
		u32 type = reloc->info & 0xff;
		u32 index = reloc->info >> 8;
		
		void* insn = (void*) reloc->address;
		u32 value = table[index];
		
		apply_relocation(insn, type, value);
	}
	
	FlushCache(0);
	FlushCache(2);
}

#endif

void apply_relocation(u32* dest, u8 type, u32 value)
{
	u32 old = *dest;
	u32 field;
	
	switch (type)
	{
		case R_MIPS_32:
			*dest += value;
			break;
		case R_MIPS_26:
			field = old + (value >> 2);
			*dest = (old & ~MIPS_TARGET_MASK) | (field & MIPS_TARGET_MASK);
			break;
		case R_MIPS_HI16:
			/* If the sign bit is set on the lower half the matching lower
			   relocation will actually make the instruction subtract instead of
			   add, hence we need to add an extra one to the upper half. */
			field = old + (value >> 16) + ((value & 0x8000) == 0x8000);
			*dest = (old & ~MIPS_IMMEDIATE_MASK) | (field & MIPS_IMMEDIATE_MASK);
			break;
		case R_MIPS_LO16:
			field = old + (value & 0xffff);
			*dest = (old & ~MIPS_IMMEDIATE_MASK) | (field & MIPS_IMMEDIATE_MASK);
			break;
		default:
			ERROR("Unsupported relocation type %hhu!\n", type);
	}
}
