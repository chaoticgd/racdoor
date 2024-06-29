#include <game/hero.h>

#include <racdoor/console.h>
#include <racdoor/hook.h>
#include <racdoor/module.h>

static const char* hero_type_strings[] = {
	/* [0] = */ "IDLE",
	/* [1] = */ "WALK",
	/* [2] = */ "FALL",
	/* [3] = */ "LEDGE",
	/* [4] = */ "JUMP",
	/* [5] = */ "GLIDE",
	/* [6] = */ "ATTACK",
	/* [7] = */ "GET_HIT",
	/* [8] = */ "SHOOT",
	/* [9] = */ "BUSY",
	/* [10] = */ "BOUNCE",
	/* [11] = */ "STOMP",
	/* [12] = */ "CROUCH",
	/* [13] = */ "GRAPPLE",
	/* [14] = */ "SWING",
	/* [15] = */ "GRIND",
	/* [16] = */ "SLIDE",
	/* [17] = */ "SWIM",
	/* [18] = */ "SURF",
	/* [19] = */ "HYDRO",
	/* [20] = */ "DEATH",
	/* [21] = */ "BOARD",
	/* [22] = */ "RACEBOARD",
	/* [23] = */ "SPIN",
	/* [24] = */ "NPC",
	/* [25] = */ "QUICKSAND",
	/* [26] = */ "ZIP",
	/* [27] = */ "HOLO",
	/* [28] = */ "CHARGE",
	/* [29] = */ "ROCKET_HOVER",
	/* [30] = */ "JET",
	/* [31] = */ "RACEBIKE",
	/* [32] = */ "SPEEDBOAT",
	/* [33] = */ "PULL",
	/* [34] = */ "LATCH",
	/* [36] = */ "LADDER",
	/* [37] = */ "SKYDIVE"
};

static const char* hero_state_strings[] = {
	/* [0] = */ "IDLE",
	/* [1] = */ "LOOK",
	/* [2] = */ "WALK",
	/* [3] = */ "SKID",
	/* [4] = */ "CROUCH",
	/* [5] = */ "QUICK_TURN",
	/* [6] = */ "FALL",
	/* [7] = */ "JUMP",
	/* [8] = */ "GLIDE",
	/* [9] = */ "RUN_JUMP",
	
	/* [10] = */ "LONG_JUMP",
	/* [11] = */ "FLIP_JUMP",
	/* [12] = */ "JINK_JUMP",
	/* [13] = */ "ROCKET_JUMP",
	/* [14] = */ "DOUBLE_JUMP",
	/* [15] = */ "HELI_JUMP",
	/* [16] = */ "CHARGE_JUMP",
	/* [17] = */ "WALL_JUMP",
	/* [18] = */ "WATER_JUMP",
	/* [19] = */ "COMBO_ATTACK",
	
	/* [20] = */ "JUMP_ATTACK",
	/* [21] = */ "THROW_ATTACK",
	/* [22] = */ "GET_HIT",
	/* [23] = */ "LEDGE_GRAB",
	/* [24] = */ "LEDGE_IDLE",
	/* [25] = */ "LEDGE_TRAVERSE_LEFT",
	/* [26] = */ "LEDGE_TRAVERSE_RIGHT",
	/* [27] = */ "LEDGE_JUMP",
	/* [28] = */ "VISIBOMB",
	/* [29] = */ "TARGETING",
	
	/* [30] = */ "GUN_WAITING",
	/* [31] = */ "WALLOPER_ATTACK",
	/* [32] = */ "ATTACK_BOUNCE",
	/* [33] = */ "ROCKET_STOMP",
	/* [34] = */ "GLOVE_ATTACK",
	/* [35] = */ "GRAPPLE_SHOOT",
	/* [36] = */ "GRAPPLE_PULL",
	/* [37] = */ "GRAPPLE_PULL_VEHICLE",
	/* [38] = */ "SUCK_CANNON",
	/* [39] = */ "GRIND",
	
	/* [40] = */ "GRIND_JUMP",
	/* [41] = */ "GRIND_SWITCH_JUMP",
	/* [42] = */ "GRIND_ATTACK",
	/* [43] = */ "SWING",
	/* [44] = */ "SWING_FALL",
	/* [45] = */ "RECOIL",
	/* [46] = */ "ICE_WALK",
	/* [47] = */ "DEVASTATOR",
	/* [48] = */ "SLIDE",
	/* [49] = */ "VEHICLE",
	
	/* [50] = */ "SWIMUNDER",
	/* [51] = */ "IDLEUNDER",
	/* [52] = */ "CHARGEUNDER",
	/* [53] = */ "SWIMSURF",
	/* [54] = */ "IDLESURF",
	/* [55] = */ "BOLT_CRANK",
	/* [56] = */ "LAVA_JUMP",
	/* [57] = */ "DEATH",
	/* [58] = */ "BOARD",
	/* [59] = */ "MAGNE_WALK",
	
	/* [60] = */ "UNKNOWN_60",
	/* [61] = */ "UNKNOWN_61",
	/* [62] = */ "GRIND_HIT",
	/* [63] = */ "GRIND_JUMP_TURN",
	/* [64] = */ "UNKNOWN_64",
	/* [65] = */ "UNKNOWN_65",
	/* [66] = */ "UNKNOWN_66",
	/* [67] = */ "UNKNOWN_67",
	/* [68] = */ "UNKNOWN_68",
	/* [69] = */ "UNKNOWN_69",
	
	/* [70] = */ "UNKNOWN_70",
	/* [71] = */ "UNKNOWN_71",
	/* [72] = */ "UNKNOWN_72",
	/* [73] = */ "UNKNOWN_73",
	/* [74] = */ "UNKNOWN_74",
	/* [75] = */ "UNKNOWN_75",
	/* [76] = */ "UNKNOWN_76",
	/* [77] = */ "UNKNOWN_77",
	/* [78] = */ "UNKNOWN_78",
	/* [79] = */ "UNKNOWN_79",
	
	/* [80] = */ "UNKNOWN_80",
	/* [81] = */ "UNKNOWN_81",
	/* [82] = */ "UNKNOWN_82",
	/* [83] = */ "UNKNOWN_83",
	/* [84] = */ "UNKNOWN_84",
	/* [85] = */ "UNKNOWN_85",
	/* [86] = */ "UNKNOWN_86",
	/* [87] = */ "UNKNOWN_87",
	/* [88] = */ "UNKNOWN_88",
	/* [89] = */ "UNKNOWN_89",
	
	/* [90] = */ "UNKNOWN_90",
	/* [91] = */ "UNKNOWN_91",
	/* [92] = */ "UNKNOWN_92",
	/* [93] = */ "UNKNOWN_93",
	/* [94] = */ "UNKNOWN_94",
	/* [95] = */ "UNKNOWN_95",
	/* [96] = */ "UNKNOWN_96",
	/* [97] = */ "UNKNOWN_97",
	/* [98] = */ "VENDOR_BOOTH",
	/* [99] = */ "NPC",
	
	/* [100] = */ "WALK_TO_POS",
	/* [101] = */ "SKID_TO_POS",
	/* [102] = */ "IDLE_TO_POS",
	/* [103] = */ "JUMP_TO_POS",
	/* [104] = */ "QUICKSAND_SINK",
	/* [105] = */ "QUICKSAND_JUMP",
	/* [106] = */ "DROWN",
	/* [107] = */ "UNKNOWN_107",
	/* [108] = */ "UNKNOWN_108",
	/* [109] = */ "UNKNOWN_109",
	
	/* [110] = */ "UNKNOWN_110",
	/* [111] = */ "MAGNE_ATTACK",
	/* [112] = */ "MAGNE_JUMP",
	/* [113] = */ "CUT_SCENE",
	/* [114] = */ "WADE",
	/* [115] = */ "ZIP",
	/* [116] = */ "GET_HIT_SURF",
	/* [117] = */ "GET_HIT_UNDER",
	/* [118] = */ "DEATH_FALL",
	/* [119] = */ "UNKNOWN_119",
	
	/* [120] = */ "SLOPESLIDE",
	/* [121] = */ "JUMP_BOUNCE",
	/* [122] = */ "DEATHSAND_SINK",
	/* [123] = */ "LAVA_DEATH",
	/* [124] = */ "UNKNOWN_124",
	/* [125] = */ "CHARGE",
	/* [126] = */ "ICEWATER_FREEZE",
	/* [127] = */ "ELECTRIC_DEATH",
	/* [128] = */ "ROCKET_HOVER",
	/* [129] = */ "ELECTRIC_DEATH_UNDER",
	
	/* [130] = */ "SKATE",
	/* [131] = */ "MOON_JUMP",
	/* [132] = */ "JET",
	/* [133] = */ "THROW_SHURIKEN",
	/* [134] = */ "RACEBIKE",
	/* [135] = */ "SPEEDBOAT",
	/* [136] = */ "HOVERPLANE",
	/* [137] = */ "LATCH_GRAB",
	/* [138] = */ "LATCH_IDLE",
	/* [139] = */ "LATCH_JUMP",
	
	/* [140] = */ "PULLSHOT_ATTACH",
	/* [141] = */ "PULLSHOT_PULL",
	/* [142] = */ "GET_FLATTENED",
	/* [143] = */ "SKYDIVE",
	/* [144] = */ "ELECTRIC_GET_HIT",
	/* [145] = */ "FLAIL_ATTACK",
	/* [146] = */ "MAGIC_TELEPORT",
	/* [147] = */ "TELEPORT_IN",
	/* [148] = */ "DEATH_NO_FALL",
	/* [149] = */ "TAUNT_SQUAT",
	
	/* [150] = */ "TAUNT_ASSPOINT",
	/* [151] = */ "TAUNT_ASSRUB",
	/* [152] = */ "TURRET_DRIVER",
	/* [153] = */ "WAIT_FOR_RESURRECT",
	/* [154] = */ "WAIT_FOR_JOIN",
	/* [155] = */ "DROPPED"
};

STATIC_ASSERT(ARRAY_SIZE(hero_state_strings) == 156, hero_state_strings_not_contiguous);

FuncHook hero_set_type_hook = {};
TRAMPOLINE(hero_set_type_trampoline, int);
int hero_set_type_thunk(char new_type, HERO_STATE_ENUM new_state, int unknown);

FuncHook hero_set_state_hook = {};
TRAMPOLINE(hero_set_state_trampoline, int);
int hero_set_state_thunk(HERO_STATE_ENUM new_state, int unknown);

void heromonitor_load(void)
{
	install_hook(&hero_set_type_hook, hero_SetType, hero_set_type_thunk, hero_set_type_trampoline);
	install_hook(&hero_set_state_hook, hero_SetState, hero_set_state_thunk, hero_set_state_trampoline);
}

MODULE_LOAD_FUNC(heromonitor_load);

int hero_set_type_thunk(char new_type, HERO_STATE_ENUM new_state, int unknown)
{
	con_puts("hero_SetType(");
	if (new_type >= 0 && new_type < ARRAY_SIZE(hero_type_strings))
		con_puts(hero_type_strings[(int) new_type]);
	else
		con_puts("UNKNOWN");
	con_puts(",");
	if (new_state >= 0 && new_state < ARRAY_SIZE(hero_state_strings))
		con_puts(hero_state_strings[new_state]);
	else
		con_puts("UNKNOWN");
	con_puts(")\n");
	
	return hero_set_type_trampoline(new_type, new_state, unknown);
}

int hero_set_state_thunk(HERO_STATE_ENUM new_state, int unknown)
{
	con_puts("hero_SetState(");
	if (new_state >= 0 && new_state < ARRAY_SIZE(hero_state_strings))
		con_puts(hero_state_strings[new_state]);
	else
		con_puts("UNKNOWN");
	con_puts(")\n");
	
	return hero_set_state_trampoline(new_state, unknown);
}
