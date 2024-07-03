#include <game/memcard.h>

#include <racdoor/crypto.h>
#include <racdoor/elf.h>
#include <racdoor/exploit.h>
#include <racdoor/hook.h>
#include <racdoor/linker.h>
#include <racdoor/loader.h>
#include <racdoor/mips.h>
#include <racdoor/module.h>

void* ParseBin();
void* parse_bin_thunk();
AUTO_HOOK(ParseBin, parse_bin_thunk, parse_bin_trampoline, void*);

int LoadFrontData();
int load_front_data_thunk();
AUTO_HOOK(LoadFrontData, load_front_data_thunk, load_front_data_trampoline, int);

int memcard_restore_data_thunk(char* addr, int index, mc_data* mcd);
AUTO_HOOK(memcard_RestoreData, memcard_restore_data_thunk, memcard_restore_data_trampoline, int);

void memcard_make_whole_save_thunk(char *addr);
AUTO_HOOK(memcard_MakeWholeSave, memcard_make_whole_save_thunk, memcard_make_whole_save_trampoline, void);

int memcard_save_thunk(int force, int level);
AUTO_HOOK(memcard_Save, memcard_save_thunk, memcard_save_trampoline, int);

void prepare_save();

void pack_map_mask_thunk();
AUTO_HOOK(PackMapMask, pack_map_mask_thunk, pack_map_mask_trampoline, void);

extern void FlushCache(int mode);

extern u8 _racdoor_trampoline;
extern u32 _racdoor_trampoline_offset;
extern u32 _racdoor_decryptor;
extern u32 _racdoor_payload;
extern u32 _racdoor_payload_end;

u32 payload_key = 0; /* Filled in by the loader. */
u32 payload_entry = 0; /* Filled in by the disarm function. */

static char should_uninstall = 0;

void* parse_bin_thunk()
{
	uninstall_all_hooks();
	
	void* startlevel = parse_bin_trampoline();
	
	if (should_uninstall)
		return startlevel;
	
	/* The payload will have been re-encrypted after it was loaded the last
	   time, so we need to decrypt it again here. */
	xor_crypt(&_racdoor_payload, &_racdoor_payload_end, payload_key);
	
	/* FastDecompress buffers compressed data into the scratchpad using DMA. */
	FlushCache(0);
	
	/* The unpack call below will reset these variables, so we need to make a
	   backup of them here. */
	u32 payload_key_backup = payload_key;
	u32 payload_entry_backup = payload_entry;
	
	/* Unpack all the sections into memory once more, so that we can apply fresh
	   relocations for the new level to them. */
	unpack();
	
	/* Restore the key and entry point. */
	payload_key = payload_key_backup;
	payload_entry = payload_entry_backup;
	
	/* Apply the relocations for the new level. */
	apply_relocations();
	
	/* And hence we need to encrypt the payload once more here. */
	xor_crypt(&_racdoor_payload, &_racdoor_payload_end, payload_key);
	
	install_module_hooks();
	
	return startlevel;
}

int load_front_data_thunk()
{
	should_uninstall = 1;
	return load_front_data_trampoline();
}

extern char HelpVoiceOn;
extern char HelpTextOn;

extern HelpDatum HelpDataMessages[148];
extern HelpDatum HelpDataGadgets[36];
extern HelpDatum HelpDataMisc[37];

extern int HelpLogPos;

extern int HelpLog;
extern u32 _racdoor_initial_hook;

extern u32 _racdoor_help_message;
extern u32 _racdoor_help_gadget;

void memcard_make_whole_save_thunk(char *addr)
{
	prepare_save();
	
	memcard_make_whole_save_trampoline(addr);
	
	disarm(0);
}

int memcard_save_thunk(int force, int level)
{
	prepare_save();
	
	int result = memcard_save_trampoline(force, level);
	
	disarm(0);
	
	return result;
}

void prepare_save()
{
	gen_xor_decryptor(&_racdoor_decryptor, (u32) &_racdoor_payload, (u32) &_racdoor_payload_end, payload_entry, payload_key);
	
	/* Gather together a bunch of variables that are needed to setup the initial
	   hook. This is all explained in the arm function itself. */
	ExploitParams params;
	
	params.help_voice_on = &HelpVoiceOn;
	params.help_text_on = &HelpTextOn;
	
	params.help_data_messages = HelpDataMessages;
	params.help_data_messages_count = ARRAY_SIZE(HelpDataMessages);
	params.enabled_help_message = (u32) &_racdoor_help_message;
	
	params.help_data_gadgets = HelpDataGadgets;
	params.help_data_gadgets_count = ARRAY_SIZE(HelpDataGadgets);
	params.enabled_help_gadget = (u32) &_racdoor_help_gadget;
	
	params.help_data_misc = HelpDataMisc;
	params.help_data_misc_count = ARRAY_SIZE(HelpDataMisc);
	
	params.help_log_pos = &HelpLogPos;
	
	params.help_log = (u32) &HelpLog;
	params.initial_hook = (u32) &_racdoor_initial_hook;
	
	params.trampoline = (u32*) (&_racdoor_trampoline + (u32) &_racdoor_trampoline_offset);
	params.trampoline_target = (u32) &_racdoor_decryptor + DECRYPTOR_ENTRY_OFFSET;
	
	/* Setup the initial hook. */
	arm(&params);
}

void pack_map_mask_thunk()
{
	/* We intentionally do not call the trampoline so that the payload data will
	   not be overwritten. */
}

int memcard_restore_data_thunk(char* addr, int index, mc_data* mcd)
{
	should_uninstall = 1;
	return memcard_restore_data_trampoline(addr, index, mcd);
}
