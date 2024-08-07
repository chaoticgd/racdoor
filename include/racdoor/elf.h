/* This file is part of Racdoor.
   Copyright (c) 2024 chaoticgd. All rights reversed.
   Released under the BSD-1-Clause license. */

#ifndef _RACDOOR_ELF_H
#define _RACDOOR_ELF_H

#include <racdoor/buffer.h>
#include <racdoor/util.h>

/* ELF file format. */

typedef struct {
	/* 0x0 */ u32 ident_magic;
	/* 0x4 */ u8 ident_class;
	/* 0x5 */ u8 ident_endianess;
	/* 0x6 */ u8 ident_version;
	/* 0x7 */ u8 ident_abi;
	/* 0x8 */ u8 ident_abi_version;
	/* 0x9 */ u8 pad[7];
	/* 0x10 */ u16 type;
	/* 0x12 */ u16 machine;
	/* 0x14 */ u32 version;
	/* 0x18 */ u32 entry;
	/* 0x1c */ u32 phoff;
	/* 0x20 */ u32 shoff;
	/* 0x24 */ u32 flags;
	/* 0x28 */ u16 ehsize;
	/* 0x2a */ u16 phentsize;
	/* 0x2c */ u16 phnum;
	/* 0x2e */ u16 shentsize;
	/* 0x30 */ u16 shnum;
	/* 0x32 */ u16 shstrndx;
 } ElfFileHeader;

typedef struct {
	/* 0x00 */ u32 type;
	/* 0x04 */ u32 offset;
	/* 0x08 */ u32 vaddr;
	/* 0x0c */ u32 paddr;
	/* 0x10 */ u32 filesz;
	/* 0x14 */ u32 memsz;
	/* 0x18 */ u32 flags;
	/* 0x1c */ u32 align;
} ElfProgramHeader;

typedef struct {
	/* 0x00 */ u32 name;
	/* 0x04 */ u32 type;
	/* 0x08 */ u32 flags;
	/* 0x0c */ u32 addr;
	/* 0x10 */ u32 offset;
	/* 0x14 */ u32 size;
	/* 0x18 */ u32 link;
	/* 0x1c */ u32 info;
	/* 0x20 */ u32 addralign;
	/* 0x24 */ u32 entsize;
} ElfSectionHeader;

typedef enum {
	SHT_NULL = 0,
	SHT_PROGBITS = 1,
	SHT_SYMTAB = 2,
	SHT_STRTAB = 3,
	SHT_RELA = 4,
	SHT_HASH = 5,
	SHT_DYNAMIC = 6,
	SHT_NOTE = 7,
	SHT_NOBITS = 8,
	SHT_REL = 9,
	SHT_SHLIB = 10,
	SHT_DYNSYM = 11,
	SHT_INIT_ARRAY = 14,
	SHT_FINI_ARRAY = 15,
	SHT_PREINIT_ARRAY = 16,
	SHT_GROUP = 17,
	SHT_SYMTAB_SHNDX = 18
} ElfSectionType;

typedef struct {
	/* 0x0 */ u32 name;
	/* 0x4 */ u32 value;
	/* 0x8 */ u32 size;
	/* 0xc */ u8 info;
	/* 0xd */ u8 other;
	/* 0xe */ u16 shndx;
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

typedef struct {
	u32 offset;
	u32 info;
} ElfRelocation;

typedef enum {
	R_MIPS_NONE = 0,
	R_MIPS_16 = 1,
	R_MIPS_32 = 2,
	R_MIPS_REL32 = 3,
	R_MIPS_26 = 4,
	R_MIPS_HI16 = 5,
	R_MIPS_LO16 = 6,
	R_MIPS_GPREL16 = 7,
	R_MIPS_LITERAL = 8,
	R_MIPS_GOT16 = 9,
	R_MIPS_PC16 = 10,
	R_MIPS_CALL16 = 11,
	R_MIPS_GPREL32 = 12
} ElfRelocationType;

#ifdef _HOST

ElfFileHeader* parse_elf_header(Buffer object);
ElfSectionHeader* lookup_section(Buffer object, const char* section);
u32 lookup_symbol(Buffer object, const char* symbol);

#endif

/* Racdoor specific. */

#define RDX_FORMAT_VERSION 1

typedef struct {
	u32 max_level;
	u32 help_message;
	u32 help_gadget;
	u32 help_log;
	u32 initial_hook;
	u32 return_to_game;
	u32 original_instruction;
	u32 trampoline;
	u32 trampoline_offset;
	u32 trampoline_block;
	u32 decryptor;
	u32 decryptor_block;
	u32 payload;
	u32 payload_end;
	u32 payload_block;
	u32 modload_hook_ofs;
	u32 modupdate_hook_ofs;
	u32 modunload_hook_ofs;
} RacdoorSymbolHeader;

/* RDX file header. */
typedef struct {
	u32 magic;
	u32 version;
	u32 payload_ofs;
	u32 payload_size;
	char muffin[64];
	char serial[16];
	u32 entry;
	RacdoorSymbolHeader symbols;
} RacdoorFileHeader;

typedef struct {
	u32 dest;
	u16 source;
	u16 size;
} RacdoorLoadHeader;

typedef struct {
	u8 copy_count;
	u8 fill_count;
	u8 decompress_count;
	u8 unused;
	RacdoorLoadHeader loads[];
} RacdoorPayloadHeader;

typedef struct {
	u32 string_offset;
	u32 runtime_index;
} RacdoorSymbolMapEntry;

typedef struct {
	u32 symbol_count;
	RacdoorSymbolMapEntry entries[];
} RacdoorSymbolMapHead;

typedef struct {
	u32 address;
	u32 info;
} RacdoorRelocation;

#ifdef _HOST

u32 lookup_runtime_symbol_index(Buffer symbolmap, const char* name);

#endif

#endif
