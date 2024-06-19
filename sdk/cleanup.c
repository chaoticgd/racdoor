#include <racdoor/cleanup.h>

#include <racdoor/util.h>

extern u32 _racdoor_initial_hook;
extern void _racdoor_original_instruction;
extern int HelpLogPos;

void FlushCache(int mode);

void cleanup()
{
	/* Restore the instruction that was modified. */
	_racdoor_initial_hook = (u32) &_racdoor_original_instruction;
	
	/* Prevent any further instructions from being modified. */
	HelpLogPos = 0;
	
	/* Flush the instruction cache. */
	FlushCache(2);
}
