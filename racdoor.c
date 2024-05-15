#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// **** Build list ****

typedef struct {
	uint32_t HelpLog;
	uint32_t GetGadgetEvent_jal_FastMemCpy;
} Gadgets;

typedef struct {
	const char* id;
	const char* serial;
	Gadgets gadgets;
} Build;

Build builds[] = {
	{
		.id = "dl-pal-black",
		.serial = "sces-50916",
		{
			.HelpLog                            = 0x001f7810,
			.GetGadgetEvent_jal_FastMemCpy      = 0x006c48c8
		}
	}
};
int32_t build_count = sizeof(builds) / sizeof(Build);

// **** Utilities ****

#define CHECK(condition, ...) if(!(condition)) { fprintf(stderr, __VA_ARGS__); exit(1); }

void* checked_malloc(unsigned long size);
int32_t align32(int32_t value, int32_t alignment);

// **** File I/O ****

typedef struct {
	char* data;
	int32_t size;
} Buffer;

Buffer read_file(const char* path);
void write_file(const char* path, Buffer buffer);
void* buffer_get(Buffer buffer, int32_t offset, int32_t size, const char* thing);
Buffer sub_buffer(Buffer buffer, int32_t offset, int32_t size, const char* thing);

// **** Save format ****

enum SaveBlockType {
	BLOCK_HelpDataMessages = 16,
	BLOCK_HelpDataMisc = 17,
	BLOCK_HelpDataGadgets = 18,
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
	/* 0x0 */ uint32_t ident_magic;
	/* 0x4 */ uint8_t ident_class;
	/* 0x5 */ uint8_t ident_endianess;
	/* 0x6 */ uint8_t ident_version;
	/* 0x7 */ uint8_t ident_abi;
	/* 0x8 */ uint8_t ident_abi_version;
	/* 0x9 */ uint8_t pad[7];
	/* 0x10 */ uint16_t type;
	/* 0x12 */ uint16_t machine;
	/* 0x14 */ uint32_t version;
	/* 0x18 */ uint32_t entry;
	/* 0x1c */ uint32_t phoff;
	/* 0x20 */ uint32_t shoff;
	/* 0x24 */ uint32_t flags;
	/* 0x28 */ uint16_t ehsize;
	/* 0x2a */ uint16_t phentsize;
	/* 0x2c */ uint16_t phnum;
	/* 0x2e */ uint16_t shentsize;
	/* 0x30 */ uint16_t shnum;
	/* 0x32 */ uint16_t shstrndx;
 } ElfFileHeader;

typedef struct {
	/* 0x00 */ uint32_t type;
	/* 0x04 */ uint32_t offset;
	/* 0x08 */ uint32_t vaddr;
	/* 0x0c */ uint32_t paddr;
	/* 0x10 */ uint32_t filesz;
	/* 0x14 */ uint32_t memsz;
	/* 0x18 */ uint32_t flags;
	/* 0x1c */ uint32_t align;
} ElfProgramHeader;

typedef struct {
	uint32_t address;
	uint32_t size;
	uint32_t section_type;
	uint32_t entry_point;
} RatchetSectionHeader;

void convert_elf(char* output, int32_t output_size, Buffer input);

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

#define MIPS_ZERO 0
#define MIPS_A0 4
#define MIPS_A1 5

#define MIPS_ADDIU(dest, source, immediate) (((immediate) & 0xffff) | ((dest) << 16) | ((source) << 21) | (0b001001 << 26))
#define MIPS_LUI(dest, immediate) ((immediate) & 0xffff) | ((dest) << 16) | (0b001111 << 26)
#define MIPS_JAL(target) ((target) & 0x3ffffff) | (0b000011 << 26)
#define MIPS_NOP() 0

void mutate_save_game(SaveSlot* save, Gadgets* gadgets);

int main(int argc, char** argv)
{
	CHECK(argc == 4, "usage: %s <input saveN.bin> <elf to inject> <output saveN.bin>\n", argc > 0 ? argv[0] : "racdoor");
	
	const char* input_save_path = argv[1];
	const char* elf_path = argv[2];
	const char* output_save_path = argv[3];
	
	Buffer file = read_file(input_save_path);
	SaveSlot save = parse_save(file);
	
	//Buffer elf = read_file(elf_path);
	
	Gadgets gadgets = builds[0].gadgets;
	mutate_save_game(&save, &gadgets);
	
	update_checksums(&save);
	write_file(output_save_path, file);
}

void mutate_save_game(SaveSlot* save, Gadgets* gadgets)
{

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

void convert_elf(char* output, int32_t output_size, Buffer input)
{
	ElfFileHeader* elf_header = buffer_get(input, 0, sizeof(ElfFileHeader), "ELF file header");
	CHECK(elf_header->ident_magic == 0x464c457f, "ELF file has bad magic number.\n");
	CHECK(elf_header->ident_class == 1, "ELF file isn't 32 bit.\n");
	CHECK(elf_header->machine == 8, "ELF file isn't compiled for MIPS.\n");
	
	for(uint32_t i = 0; i < elf_header->phnum; i++)
	{
		int32_t program_header_offset = elf_header->phoff + i * sizeof(ElfProgramHeader);
		ElfProgramHeader* program_header = buffer_get(input, program_header_offset, sizeof(ElfProgramHeader), "ELF program header");
		
		int32_t size = align32(sizeof(RatchetSectionHeader) + program_header->filesz, 4);
		CHECK(size >= 0 && output_size >= size, "ELF too big!\n");
		
		RatchetSectionHeader* output_header = (RatchetSectionHeader*) output;
		output_header->address = program_header->vaddr;
		output_header->size = program_header->filesz;
		output_header->section_type = 0;
		output_header->entry_point = elf_header->entry;
		
		char* segment = buffer_get(input, program_header->offset, program_header->filesz, "ELF segment data");
		memcpy(output + sizeof(RatchetSectionHeader), segment, program_header->filesz);
		
		output += size;
		output_size -= size;
	}
	
	CHECK(output_size >= sizeof(RatchetSectionHeader), "ELF too big!\n");
	memset(output, 0, sizeof(RatchetSectionHeader));
}

Buffer read_file(const char* path)
{
	FILE* file = fopen(path, "rb");
	CHECK(file, "Failed to open input file '%s'.\n", path);
	
	CHECK(fseek(file, 0, SEEK_END) == 0, "Failed to seek to beginning of input file '%s'.\n", path);
	long file_size = ftell(file);
	CHECK(file_size > 0 && file_size < UINT32_MAX, "Cannot determine file size for input file '%s'.\n", path);
	
	char* file_data = checked_malloc(file_size);
	CHECK(fseek(file, 0, SEEK_SET) == 0, "Failed to seek to beginning of input file '%s'.\n", path);
	CHECK(fread(file_data, file_size, 1, file) == 1, "Failed to read input file '%s'.\n", path);
	
	fclose(file);
	
	Buffer buffer = {
		.data = file_data,
		.size = file_size
	};
	return buffer;
}

void write_file(const char* path, Buffer buffer)
{
	FILE* file = fopen(path, "wb");
	CHECK(file, "Failed to open output file '%s'.\n", path);
	
	CHECK(fwrite(buffer.data, buffer.size, 1, file) == 1, "Failed to write output file '%s'.\n", path);
	
	fclose(file);
}

void* buffer_get(Buffer buffer, int32_t offset, int32_t size, const char* thing)
{
	CHECK(offset > -1 && offset + size <= buffer.size, "Out of bounds %s.\n", thing);
	return buffer.data + offset;
}

Buffer sub_buffer(Buffer buffer, int32_t offset, int32_t size, const char* thing)
{
	CHECK(offset > -1 && offset + size <= buffer.size, "Out of bounds %s.\n", thing);
	Buffer result = {
		.data = buffer.data + offset,
		.size = size
	};
	return result;
}

void* checked_malloc(unsigned long size)
{
	void* ptr = malloc(size);
	CHECK(ptr || size == 0, "Failed to allocate memory.\n");
	return ptr;
}

int32_t align32(int32_t value, int32_t alignment)
{
	return value + (-value & (alignment - 1));
}
