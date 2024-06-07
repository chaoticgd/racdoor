#define MIPS_ZERO 0
#define MIPS_A0 4
#define MIPS_A1 5
#define MIPS_A2 6
#define MIPS_A3 7
#define MIPS_T0 8
#define MIPS_T1 9
#define MIPS_T2 10
#define MIPS_T3 11
#define MIPS_SP 29

#define MIPS_OP_MASK        0xfc000000
#define MIPS_RS_MASK        0x03e00000
#define MIPS_RT_MASK        0x001f0000
#define MIPS_RD_MASK        0x0000f800
#define MIPS_SA_MASK        0x000007c0
#define MIPS_FUNCTION_MASK  0x0000003f
#define MIPS_IMMEDIATE_MASK 0x0000ffff
#define MIPS_TARGET_MASK    0x03ffffff

#define MIPS_NOP() 0
#define MIPS_ADD(dest, rs, rt) 0x20 | ((dest) << 11) | ((rt) << 16) | ((rs) << 21) | (0x0 << 26)
#define MIPS_J(target) ((target >> 2) & MIPS_TARGET_MASK) | (0x2 << 26)
#define MIPS_JAL(target) ((target >> 2) & MIPS_TARGET_MASK) | (0x3 << 26)
#define MIPS_BEQ(rs, rt, offset) ((offset) & MIPS_IMMEDIATE_MASK) | ((rt) << 16) | ((rs) << 21) | (0x4 << 26)
#define MIPS_ADDIU(dest, source, immediate) (((immediate) & MIPS_IMMEDIATE_MASK) | ((dest) << 16) | ((source) << 21) | (0x9 << 26))
#define MIPS_ORI(dest, source, immediate) (((immediate) & MIPS_IMMEDIATE_MASK) | ((dest) << 16) | ((source) << 21) | (0xd << 26))
#define MIPS_LUI(dest, immediate) ((immediate) & MIPS_IMMEDIATE_MASK) | ((dest) << 16) | (0xf << 26)
#define MIPS_LW(dest, offset, base) (offset & MIPS_IMMEDIATE_MASK) | ((dest) << 16) | ((base) << 21) | (0x23 << 26)
#define MIPS_LBU(dest, offset, base) (offset & MIPS_IMMEDIATE_MASK) | ((dest) << 16) | ((base) << 21) | (0x24 << 26)
#define MIPS_SW(src, offset, base) (offset & MIPS_IMMEDIATE_MASK) | ((src) << 16) | ((base) << 21) | (0x2b << 26)
