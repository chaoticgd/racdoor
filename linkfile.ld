/* We enter through the racdoor. */
ENTRY(racdoor_entry);

MEMORY
{
	/* This memory range will be used by OPL if the user has that setup so we
	   don't want to touch it. */
	opl (rwx) : ORIGIN = 0x00084000, LENGTH = 304K
	
	/* This memory range, like the range above, is technically reserved for the
	   PS2's operating system but in practice it's never used so it's a great
	   place for a code cave. */
	cave (rwx) : ORIGIN = 0x000D0000, LENGTH = 192K
}

PHDRS
{
	text PT_LOAD;
}

SECTIONS
{
	.text :
	{
		*(.text)
		*(.text.*)
		*(.gnu.linkonce.t*)
	} >cave :text
	
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
	
	racdoor.addrtbl :
	{
		*(racdoor.addrtbl)
	}
}
