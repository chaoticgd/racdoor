#ifndef _GAME_DL_H
#define _GAME_DL_H

extern "C" {

struct MEMSLOTS { // 0xe4
	/* 0x00 */ void *os;
	/* 0x04 */ void *code;
	/* 0x08 */ void *base;
	/* 0x0c */ void *vu1_chain[2];
	/* 0x14 */ void *tie_cache;
	/* 0x18 */ void *moby_joint_cache;
	/* 0x1c */ void *joint_cache_entry_list;
	/* 0x20 */ void *level_base;
	/* 0x24 */ void *level_nav;
	/* 0x28 */ void *level_tfrag;
	/* 0x2c */ void *level_occl;
	/* 0x30 */ void *level_sky;
	/* 0x34 */ void *level_coll;
	/* 0x38 */ void *level_vram;
	/* 0x3c */ void *level_part_vram;
	/* 0x40 */ void *level_fx_vram;
	/* 0x44 */ void *level_mobys;
	/* 0x48 */ void *level_ties;
	/* 0x4c */ void *level_shrubs;
	/* 0x50 */ void *level_ratchet;
	/* 0x54 */ void *level_gameplay;
	/* 0x58 */ void *level_global_nav_data;
	/* 0x5c */ void *level_mission_load_buffer;
	/* 0x60 */ void *level_mission_pvar_buffer;
	/* 0x64 */ void *level_mission_class_buffer_1;
	/* 0x68 */ void *level_mission_class_buffer_2;
	/* 0x6c */ void *level_mission_moby_insts;
	/* 0x70 */ void *level_mission_moby_pvars;
	/* 0x74 */ void *level_mission_moby_groups;
	/* 0x78 */ void *level_mission_moby_shared;
	/* 0x7c */ void *level_art;
	/* 0x80 */ void *level_help;
	/* 0x84 */ void *level_tie_insts;
	/* 0x88 */ void *level_shrub_insts;
	/* 0x8c */ void *level_moby_insts;
	/* 0x90 */ void *level_moby_insts_backup;
	/* 0x94 */ void *level_moby_pvars;
	/* 0x98 */ void *level_moby_pvars_backup;
	/* 0x9c */ void *level_misc_insts;
	/* 0xa0 */ void *level_part_insts;
	/* 0xa4 */ void *level_moby_sound_remap;
	/* 0xa8 */ void *level_end;
	/* 0xac */ void *perm_base;
	/* 0xb0 */ void *perm_armor;
	/* 0xb4 */ void *perm_armor2;
	/* 0xb8 */ void *perm_skin;
	/* 0xbc */ void *perm_patch;
	/* 0xc0 */ void *hud;
	/* 0xc4 */ void *gui;
	/* 0xc8 */ void *net_overlay;
	/* 0xcc */ void *heap;
	/* 0xd0 */ void *stack;
	/* 0xd4 */ void *occl_points;
	/* 0xd8 */ void *occl_grids;
	/* 0xdc */ void *profile;
	/* 0xe0 */ void *debug;
};

extern MEMSLOTS MemSlots;

}

#endif
