#include <racdoor/crypto.h>
#include <racdoor/elf.h>
#include <racdoor/mips.h>
#include <racdoor/save.h>
#include <racdoor/util.h>

#include <string.h>

void inject_rac(SaveSlot* save, Buffer rdx);
u32 pack_rdx(u8* output, u32 output_size, Buffer rdx);
u32 lookup_symbol(Buffer object, const char* symbol);

int main(int argc, char** argv)
{
	CHECK(argc == 4, "usage: %s <input saveN.bin> <input rdx> <output saveN.bin>\n",
		argc > 0 ? argv[0] : "inject");
	
	const char* input_save_path = argv[1];
	const char* input_rdx_path = argv[2];
	const char* output_save_path = argv[3];
	
	Buffer file = read_file(input_save_path);
	SaveSlot save = parse_save(file);
	
	Buffer rdx = read_file(input_rdx_path);
	
	switch (save.game.block_count)
	{
		case RAC_GAME_BLOCK_COUNT:
			inject_rac(&save, rdx);
			break;
		case GC_GAME_BLOCK_COUNT:
			printf("not yet implemented\n");
			break;
		case UYA_GAME_BLOCK_COUNT:
			printf("not yet implemented\n");
			break;
		case DL_GAME_BLOCK_COUNT:
			printf("not yet implemented\n");
			break;
		default:
			ERROR("Unexpected block count.\n");
	}
	
	update_checksums(&save);
	write_file(output_save_path, file);
}

typedef struct {
	u16 flag;
	u16 unknown_2;
	u32 unknown_4;
} HelpDatum;

void inject_rac(SaveSlot* save, Buffer rdx)
{
	/* Lookup special symbols used for configuring the exploit. */
	u32 initial_hook = lookup_symbol(rdx, "_racdoor_initial_hook");
	u32 trampoline = lookup_symbol(rdx, "_racdoor_trampoline");
	u32 trampoline_offset = lookup_symbol(rdx, "_racdoor_trampoline_offset");
	u32 trampoline_block = lookup_symbol(rdx, "_racdoor_trampoline_block");
	u32 decryptor = lookup_symbol(rdx, "_racdoor_decryptor");
	u32 decryptor_block = lookup_symbol(rdx, "_racdoor_decryptor_block");
	u32 image = lookup_symbol(rdx, "_racdoor_image");
	u32 image_end = lookup_symbol(rdx, "_racdoor_image_end");
	
	/* Pack and encrypt the payload image. */
	u32 block_size = 2048;
	u32 max_size = block_size * save->level_count;
	
	u8* data = checked_malloc(max_size);
	memset(data, 0, sizeof(max_size));
	
	u32 entry_point_offset = pack_rdx(data, max_size, rdx);
	
	/* Enable help text so that our exploit runs, but disable the help audio
	   since playing the same help message again and again would otherwise get
	   quite repetitive. */
	SaveBlock* voice_on = lookup_block(&save->game, BLOCK_HelpVoiceOn);
	SaveBlock* text_on = lookup_block(&save->game, BLOCK_HelpTextOn);
	CHECK(voice_on->size == 1, "Incorrectly sized VoiceOn block.\n");
	CHECK(text_on->size == 1, "Incorrectly sized VoiceOn block.\n");
	*(u8*) voice_on->data = 0;
	*(u8*) text_on->data = 1;
	
	/* TODO: More than 8 items must be unlocked for the help message we want to
	   use to be run. */
	   
	/* Disable all the help messages except for the one we want to use to
	   trigger the initial out of bounds write. */
	
	u32 quicksel_help_message = 80;
	u32 precondition_help_message = 21;
	
	SaveBlock* help_data_messages = lookup_block(&save->game, BLOCK_HelpDataMessages);
	for (u32 i = 0; i < help_data_messages->size / sizeof(HelpDatum); i++)
	{
		HelpDatum* message = &((HelpDatum*) help_data_messages->data)[i];
		message->flag = (i == quicksel_help_message) ? 0 : -1;
		message->unknown_2 = 0;
		message->unknown_4 = 0;
	}
	
	SaveBlock* help_data_gadgets = lookup_block(&save->game, BLOCK_HelpDataGadgets);
	for (u32 i = 0; i < help_data_gadgets->size / sizeof(HelpDatum); i++)
	{
		HelpDatum* message = &((HelpDatum*) help_data_gadgets->data)[i];
		message->flag = -1;
		message->unknown_2 = 0;
		message->unknown_4 = 0;
	}
	
	SaveBlock* help_data_misc = lookup_block(&save->game, BLOCK_HelpDataMisc);
	for (u32 i = 0; i < help_data_misc->size / sizeof(HelpDatum); i++)
	{
		HelpDatum* message = &((HelpDatum*) help_data_misc->data)[i];
		message->flag = (i == precondition_help_message) ? 0 : -1;
		message->unknown_2 = 0;
		message->unknown_4 = 0;
	}
	
	/* Overrwrite the HelpLogPos variable so that the help message index is
	   written over the high byte the immediate field from a relative branch
	   instruction such as to make the processor jump into and start executing
	   code loaded from the memory card. */
	SaveBlock* help_log_pos = lookup_block(&save->game, BLOCK_HelpLogPos);
	CHECK(help_log_pos->size == 4, "Incorrectly sized HelpLogPos block.\n");
	*(u32*) help_log_pos->data = (initial_hook + 1) - 0x00141e08;
	
	/* Because we cannot choose exactly where in the memory card data the
	   processor will jump to, we setup a trampoline to jump to a more desirable
	   location. */
	SaveBlock* trampoline_save = lookup_block(&save->game, trampoline_block);
	CHECK((u64) trampoline_offset + 8 < trampoline_save->size, "Symbol _racdoor_target_offset is too big.\n");
	*(u32*) &trampoline_save->data[trampoline_offset + 0] = MIPS_J(decryptor);
	*(u32*) &trampoline_save->data[trampoline_offset + 4] = MIPS_NOP();
	
	/* Write out some assembly to decrypt the payload. */
	u32 max_decryptor_size = 256;
	u8 decryptor_asm[256];
	memset(decryptor_asm, 0, sizeof(decryptor_asm));
	
	gen_xor_decryptor((u32*) decryptor_asm, image, image_end, 0, image + entry_point_offset);
	
	u64 decryptor_asm_offset = 0;
	for (u32 i = 0; i < save->level_count; i++)
	{
		SaveBlock* dest = lookup_block(&save->levels[i], decryptor_block);
		u32 copy_size = MIN(dest->size, max_decryptor_size - decryptor_asm_offset);
		memcpy(dest->data, decryptor_asm + decryptor_asm_offset, copy_size);
		
		decryptor_asm_offset += copy_size;
	}
	
	/* Finally, we write the encrypted payload image to the save file. */
	for (u32 i = 0; i < save->level_count; i++)
	{
		SaveBlock* dest = lookup_block(&save->levels[i], BLOCK_MapMask);
		memcpy(dest->data, data + i * block_size, block_size);
		
		u32 offset = (u32) (dest->data - (char*) save->header);
		printf("Written block for level %d at 0x%x.\n", i, offset);
	}
}

u32 pack_rdx(u8* output, u32 output_size, Buffer rdx)
{
	u32 offset = 0;
	
	printf("%.2fkb total\n", output_size / 1024.f);
	
	CHECK((u64) offset + sizeof(RacdoorImageHeader) < output_size, "RDX too big!\n");
	RacdoorImageHeader* image_header = (RacdoorImageHeader*) &output[offset];
	offset += sizeof(RacdoorImageHeader);
	
	memset(image_header, 0, sizeof(RacdoorImageHeader));
	
	ElfFileHeader* rdx_header = buffer_get(rdx, 0, sizeof(ElfFileHeader), "RDX file header");
	CHECK(rdx_header->ident_magic == 0x464c457f, "RDX file has bad magic number.\n");
	CHECK(rdx_header->ident_class == 1, "RDX file isn't 32 bit.\n");
	CHECK(rdx_header->machine == 8, "RDX file isn't compiled for MIPS.\n");
	
	ElfSectionHeader* sections = buffer_get(rdx, rdx_header->shoff, rdx_header->shnum * sizeof(ElfSectionHeader), "RDX section headers");
	
	/* Make room for the load headers. */
	for (u32 i = 0; i < rdx_header->shnum; i++)
	{
		if (sections[i].addr == 0 || sections[i].size == 0)
			continue;
		
		if (sections[i].type != SHT_PROGBITS && sections[i].type != SHT_NOBITS)
			continue;
		
		CHECK((u64) offset + sizeof(RacdoorLoadHeader) < output_size, "RDX too big!\n");
		offset += sizeof(RacdoorLoadHeader);
	}
	
	u32 load_index = 0;
	u32 entry_point_offset = 0;
	
	/* Convert PROGBITS sections to copy headers. */
	for (u32 i = 0; i < rdx_header->shnum; i++)
	{
		if (sections[i].addr == 0 || sections[i].size == 0)
			continue;
		
		int is_entry = rdx_header->entry >= sections[i].addr && rdx_header->entry < (sections[i].addr + sections[i].size);
		
		if (sections[i].type != SHT_PROGBITS && !is_entry)
			continue;
		
		RacdoorLoadHeader* load_header = &image_header->loads[load_index++];
		
		CHECK(offset <= 0xffff, "Cannot encode load offset.\n");
		CHECK(sections[i].size <= 0xffff, "Cannot encode load size.\n");
		load_header->dest = sections[i].addr;
		load_header->source = (u16) offset;
		load_header->size = (u16) sections[i].size;
		
		u8* data = buffer_get(rdx, sections[i].offset, sections[i].size, "RDX section header");
		
		CHECK((u64) offset + sections[i].size < output_size, "RDX too big!\n");
		memcpy(&output[offset], data, sections[i].size);
		
		if (is_entry)
			entry_point_offset = offset + (rdx_header->entry - sections[i].addr);
		
		offset += sections[i].size;
		image_header->copy_count++;
	}
	
	/* Convert NOBITS sections to fill headers. */
	for (u32 i = 0; i < rdx_header->shnum; i++)
	{
		if (sections[i].addr == 0 || sections[i].size == 0)
			continue;
		
		int is_entry = rdx_header->entry >= sections[i].addr && rdx_header->entry < sections[i].addr + sections[i].size;
		
		if (sections[i].type != SHT_NOBITS || is_entry)
			continue;
		
		RacdoorLoadHeader* load_header = &image_header->loads[load_index++];
		
		CHECK(offset <= 0xffff, "Cannot encode load offset.\n");
		CHECK(sections[i].size <= 0xffff, "Cannot encode load size.\n");
		load_header->dest = sections[i].addr;
		load_header->source = 0; /* Fill value. */
		load_header->size = (u16) sections[i].size;
		
		image_header->fill_count++;
	}
	
	printf("%.2fkb used\n", offset / 1024.f);
	printf("%.2fkb left\n", (output_size - offset) / 1024.f);
	
	return entry_point_offset;
}

u32 lookup_symbol(Buffer object, const char* symbol)
{
	ElfFileHeader* header = buffer_get(object, 0, sizeof(ElfFileHeader), "ELF header");
	u32 shstrtab_offset = header->shoff + header->shstrndx * sizeof(ElfSectionHeader);
	ElfSectionHeader* shstrtab = buffer_get(object, shstrtab_offset, sizeof(ElfSectionHeader), "shstr section header");
	
	/* Find the symbol table section. */
	ElfSectionHeader* symtab = NULL;
	for (u32 i = 0; i < header->shnum; i++)
	{
		u32 section_offset = header->shoff + i * sizeof(ElfSectionHeader);
		ElfSectionHeader* section = buffer_get(object, section_offset, sizeof(ElfSectionHeader), "section header");
		
		const char* name = buffer_string(object, shstrtab->offset + section->name, "section name");
		if (strcmp(name, ".symtab") == 0)
			symtab = section;
	}
	
	CHECK(symtab, "No .symtab section.\n");
	
	/* Find the string table section. */
	ElfSectionHeader* strtab = NULL;
	if (symtab->link != 0)
	{
		u32 link_offset = header->shoff + symtab->link * sizeof(ElfSectionHeader);
		strtab = buffer_get(object, link_offset, sizeof(ElfSectionHeader), "linked section header");
	}
	
	CHECK(strtab, "No linked string table section.\n");
	
	/* Lookup the symbol by name. */
	ElfSymbol* symbols = buffer_get(object, symtab->offset, symtab->size, "symbol table");
	for (u32 i = 0; i < symtab->size / sizeof(ElfSymbol); i++)
	{
		const char* name = buffer_string(object, strtab->offset + symbols[i].name, "symbol name");
		if (strcmp(name, symbol) == 0)
		{
			printf("%s: %x\n", symbol, symbols[i].value);
			return symbols[i].value;
		}
	}
	
	ERROR("Undefined special symbol '%s'.\n", symbol);
}
