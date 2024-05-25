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
	/* 0x00 */ uint32_t name;
	/* 0x04 */ uint32_t type;
	/* 0x08 */ uint32_t flags;
	/* 0x0c */ uint32_t addr;
	/* 0x10 */ uint32_t offset;
	/* 0x14 */ uint32_t size;
	/* 0x18 */ uint32_t link;
	/* 0x1c */ uint32_t info;
	/* 0x20 */ uint32_t addralign;
	/* 0x24 */ uint32_t entsize;
} ElfSectionHeader;

typedef enum {
	SHT_NULL = 0,
	SHT_PROGBITS = 1,
	SHT_SYMTAB = 2,
	SHT_STRTAB = 3
} ElfSectionType;

typedef struct {
	/* 0x0 */ uint32_t name;
	/* 0x4 */ uint32_t value;
	/* 0x8 */ uint32_t size;
	/* 0xc */ uint8_t info;
	/* 0xd */ uint8_t other;
	/* 0xe */ uint16_t shndx;
} ElfSymbol;

typedef enum {
	STT_NOTYPE = 0,
	STT_OBJECT = 1,
	STT_FUNC = 2,
	STT_SECTION = 3,
	STT_FILE = 4,
	STT_COMMON = 5,
	STT_TLS = 6
} ElfSymbolType;

typedef enum {
	STB_LOCAL = 0,
	STB_GLOBAL = 1
} ElfSymbolBind;

#endif
