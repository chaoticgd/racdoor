ENTRY(loader);

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
	
	/* Assembly code that loads the other sections by copying, filling or
	   decompressing them. */
	.racdoor.loader :
	{
		/* EE hardware errata: Prevent code and data from being too close. */
		. += 0x14;
		
		*(.racdoor.loader)
	} :text
	
	.text :
	{
		*(.text)
		*(.text.*)
		*(.gnu.linkonce.t*)
		
		/* EE hardware errata: Prevent code and data from being too close. */
		. += 0x14;
	}
	
	.data ALIGN(128) :
	{
		*(.data)
		*(.data.*)
		*(.gnu.linkonce.d*)
		
		/* Function hooks that are installed automatically. */
		. = ALIGN(8);
		_racdoor_autohooks = .;
		*(.racdoor.autohooks)
		_racdoor_autohooks_end = .;
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
	
	/* Function pointers used for loading, updating and unloading modules. */
	.racdoor.modules :
	{
		_racdoor_modloadfuncs = .;
		*(.racdoor.modloadfuncs)
		_racdoor_modloadfuncs_end = .;
		
		_racdoor_modupdatefuncs = .;
		*(.racdoor.modupdatefuncs)
		_racdoor_modupdatefuncs_end = .;
		
		_racdoor_modunloadfuncs = .;
		*(.racdoor.modunloadfuncs)
		_racdoor_modunloadfuncs_end = .;
	}
	
	/* Level to overlay index mapping table. Cannot be compressed since it is
	   used by the loader. */
	.racdoor.overlaymap :
	{
		_racdoor_overlaymap = .;
		*(.racdoor.overlaymap)
	}
	
	/* Runtime address table used for performing relocation. */
	.racdoor.addrtbl :
	{
		_racdoor_addrtbl = .;
		*(.racdoor.addrtbl)
	}
	
	/* Array of function pointers for the FastDecompress function, one element
	   for each overlay. Cannot be compressed since it is used by the loader. */
	.racdoor.fastdecompress :
	{
		_racdoor_fastdecompress = .;
		*(.racdoor.fastdecompress)
	}
	
	/* Runtime relocation table. This is filled in by rdxlink and only contains
	   relocations that are needed for linking against the level overlays. */
	.racdoor.relocs :
	{
		_racdoor_relocs = .;
		*(.racdoor.relocs)
	}
}
