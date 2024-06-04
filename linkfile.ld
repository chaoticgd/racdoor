/* We enter through the racdoor. */
ENTRY(racdoor_entry);

PHDRS
{
	text PT_LOAD;
}

SECTIONS
{
	. = 0x0;
	
	/* This is a fake section used to hold all the symbols used for linking with
	   the game. It must have an address of zero so that the linker doesn't try
	   to relocate any of them since they already have absolute addresses. */
	.racdoor.dummy :
	{
		*(.racdoor.dummy)
	}
	
	/* This is a good spot for a code cave. It's low enough that it's not used
	   by the game, but is also high enough that in practice it's not used by
	   the operating system or OPL. */
	. = 0x000d0000;
	
	.text :
	{
		*(.text)
		*(.text.*)
		*(.gnu.linkonce.t*)
	} :text
	
	.data ALIGN(128) :
	{
		*(.data)
		*(.data.*)
		*(.gnu.linkonce.d*)
	}
	
	.rodata ALIGN(128) :
	{
		*(.rodata)
		*(.rodata.*)
		*(.gnu.linkonce.r*)
	}
	
	.sdata ALIGN(128) :
	{
		*(.sdata)
		*(.sdata.*)
		*(.gnu.linkonce.s*)
	}
	
	.sbss ALIGN(128) :
	{
		*(.sbss)
		*(.sbss.*)
		*(.gnu.linkonce.sb*)
		*(.scommon)
	}
	
	.bss ALIGN(128) :
	{
		*(.bss)
		*(.bss.*)
		*(.gnu.linkonce.b*)
		*(COMMON)
	}
	
	/* Runtime address table used for performing relocation. */
	.racdoor.addrtbl :
	{
		_racdoor_addrtbl = .;
		*(.racdoor.addrtbl)
	}
	
	/* Runtime relocation table. This is filled in while racdoor is packing the
	   final ELF into the save game and only contains relocations that are
	   needed for linking against the level overlays. */
	.racdoor.relocs :
	{
		_racdoor_relocs = .;
		*(.racdoor.relocs)
		_racdoor_relocs_end = .;
	}
}
