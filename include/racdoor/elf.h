#ifndef _RACDOOR_ELF_H
#define _RACDOOR_ELF_H

#include <stdint.h>

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
	uint32_t sh_name;
	uint32_t sh_type;
	uint32_t sh_flags;
	uint32_t sh_addr;
	uint32_t sh_offset;
	uint32_t sh_size;
	uint32_t sh_link;
	uint32_t sh_info;
	uint32_t sh_addralign;
	uint32_t sh_entsize;
} ElfSectionHeader;

#endif
