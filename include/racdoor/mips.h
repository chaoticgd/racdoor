#define MIPS_ZERO 0
#define MIPS_AT 1
#define MIPS_V0 2
#define MIPS_V1 3
#define MIPS_A0 4
#define MIPS_A1 5
#define MIPS_A2 6
#define MIPS_A3 7
#define MIPS_T0 8
#define MIPS_T1 9
#define MIPS_T2 10
#define MIPS_T3 11
#define MIPS_T4 12
#define MIPS_T5 13
#define MIPS_T6 14
#define MIPS_T7 15
#define MIPS_S0 16
#define MIPS_S1 17
#define MIPS_S2 18
#define MIPS_S3 19
#define MIPS_S4 20
#define MIPS_S5 21
#define MIPS_S6 22
#define MIPS_S7 23
#define MIPS_T8 24
#define MIPS_T9 25
#define MIPS_K0 26
#define MIPS_K1 27
#define MIPS_GP 28
#define MIPS_SP 29
#define MIPS_FP 30
#define MIPS_RA 31

#define MIPS_OP_MASK        0xfc000000
#define MIPS_RS_MASK        0x03e00000
#define MIPS_RT_MASK        0x001f0000
#define MIPS_RD_MASK        0x0000f800
#define MIPS_SA_MASK        0x000007c0
#define MIPS_FUNCTION_MASK  0x0000003f
#define MIPS_IMMEDIATE_MASK 0x0000ffff
#define MIPS_TARGET_MASK    0x03ffffff

#define MIPS_NOP() 0
#define MIPS_SLL(rd, rt, sa) 0x0 | ((sa) << 6) | ((rt) << 16) | ((rd) << 11) | (0x0 << 26)
#define MIPS_SRL(rd, rt, sa) 0x2 | ((sa) << 6) | ((rt) << 16) | ((rd) << 11) | (0x0 << 26)
#define MIPS_ADD(rd, rs, rt) 0x20 | ((rd) << 11) | ((rt) << 16) | ((rs) << 21) | (0x0 << 26)
#define MIPS_XOR(rd, rs, rt) 0x26 | ((rd) << 11) | ((rt) << 16) | ((rs) << 21) | (0x0 << 26)
#define MIPS_J(target) (((target) >> 2) & MIPS_TARGET_MASK) | (0x2 << 26)
#define MIPS_JAL(target) (((target) >> 2) & MIPS_TARGET_MASK) | (0x3 << 26)
#define MIPS_BEQ(rs, rt, offset) ((offset) & MIPS_IMMEDIATE_MASK) | ((rt) << 16) | ((rs) << 21) | (0x4 << 26)
#define MIPS_BNE(rs, rt, offset) ((offset) & MIPS_IMMEDIATE_MASK) | ((rt) << 16) | ((rs) << 21) | (0x5 << 26)
#define MIPS_ADDIU(rt, rs, immediate) (((immediate) & MIPS_IMMEDIATE_MASK) | ((rt) << 16) | ((rs) << 21) | (0x9 << 26))
#define MIPS_ORI(rt, rs, immediate) (((immediate) & MIPS_IMMEDIATE_MASK) | ((rt) << 16) | ((rs) << 21) | (0xd << 26))
#define MIPS_LUI(rt, immediate) ((immediate) & MIPS_IMMEDIATE_MASK) | ((rt) << 16) | (0xf << 26)
#define MIPS_LQ(rt, offset, base) ((offset) & MIPS_IMMEDIATE_MASK) | ((rt) << 16) | ((base) << 21) | (0x1e << 26)
#define MIPS_SQ(rt, offset, base) ((offset) & MIPS_IMMEDIATE_MASK) | ((rt) << 16) | ((base) << 21) | (0x1f << 26)
#define MIPS_LW(rt, offset, base) ((offset) & MIPS_IMMEDIATE_MASK) | ((rt) << 16) | ((base) << 21) | (0x23 << 26)
#define MIPS_LBU(rt, offset, base) ((offset) & MIPS_IMMEDIATE_MASK) | ((rt) << 16) | ((base) << 21) | (0x24 << 26)
#define MIPS_LWU(rt, offset, base) ((offset) & MIPS_IMMEDIATE_MASK) | ((rt) << 16) | ((base) << 21) | (0x27 << 26)
#define MIPS_SW(rt, offset, base) ((offset) & MIPS_IMMEDIATE_MASK) | ((rt) << 16) | ((base) << 21) | (0x2b << 26)
