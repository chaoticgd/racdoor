/* We enter through the racdoor. */
ENTRY(racdoor_entry);

SECTIONS
{
	/* This is memory that's reserved for the PS2's operating system but in
	   practice it's never used so it's a great place for a code cave. */
	. = 0x000a0000;
	
	.text :
	{
		*(.text)
	}
	
	.data ALIGN(128) :
	{
		*(.data)
	}
	
	.bss ALIGN(128) :
	{
		*(.bss)
	}
	
	/* Remove unnecessary information from the output. The only one here that
	   was really a problem was .MIPS.abiflags since there was a new segment
	   being created for it. */
	/DISCARD/ :
	{
		*(.MIPS.abiflags)
		*(.reginfo)
		*(.pdr)
		*(.comment)
		*(.gnu.attributes)
	}
	
	FontPrint = 0x005613a0;
}
