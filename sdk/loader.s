	.set noat
	.set noreorder
	.set nomacro
	.section .racdoor.loader
	.global loader_entry
loader_entry:
	addiu $sp, $sp, -0x1c0

loader_save_regs:
	sq $s2, 0xa0($sp)
	# sq $t3, 0xc0($sp)
	sq $t8, 0x190($sp)
	sq $t7, 0x160($sp)
	sq $a1, 0x1a0($sp)
	sq $fp, 0x80($sp)
	# sq $t2, 0x90($sp)
	sq $t5, 0x60($sp)
	sq $s5, 0x40($sp)
	# sq $t0, 0x110($sp)
	sq $gp, 0x150($sp)
	# sq $t1, 0x10($sp)
	sq $s1, 0x140($sp)
	sq $ra, 0x100($sp)
	sq $s7, 0x180($sp)
	sq $s0, 0xb0($sp)
	sq $a2, 0x70($sp)
	sq $a3, 0x120($sp)
	sq $s3, 0xd0($sp)
	sq $s6, 0x170($sp)
	sq $t6, 0x130($sp)
	sq $t9, 0x1b0($sp)
	sq $a0, 0xf0($sp)
	sq $s4, 0x30($sp)
	sq $t4, 0x0($sp)
	sq $v1, 0x20($sp)
	sq $at, 0xe0($sp)
	sq $v0, 0x50($sp)

loader_unpack:
	b unpack_initial
	nop

loader_run:
	jal FlushCache
	addiu $a0, $zero, 0
	jal FlushCache
	addiu $a0, $zero, 2
	
	jal apply_relocations
	nop
	jal cleanup
	nop
	jal load_modules
	nop

loader_restore_regs:
	lq $t5, 0x60($sp)
	lq $a2, 0x70($sp)
	lq $t0, 0x110($sp)
	lq $s1, 0x140($sp)
	lq $s6, 0x170($sp)
	lq $s4, 0x30($sp)
	lq $t7, 0x160($sp)
	lq $a3, 0x120($sp)
	lq $at, 0xe0($sp)
	lq $t1, 0x10($sp)
	lq $v1, 0x20($sp)
	lq $t6, 0x130($sp)
	lq $s3, 0xd0($sp)
	lq $s2, 0xa0($sp)
	lq $ra, 0x100($sp)
	lq $t2, 0x90($sp)
	lq $a0, 0xf0($sp)
	lq $fp, 0x80($sp)
	lq $v0, 0x50($sp)
	lq $a1, 0x1a0($sp)
	lq $t9, 0x1b0($sp)
	lq $gp, 0x150($sp)
	lq $t4, 0x0($sp)
	lq $s5, 0x40($sp)
	lq $t8, 0x190($sp)
	lq $t3, 0xc0($sp)
	lq $s7, 0x180($sp)
	lq $s0, 0xb0($sp)

loader_return_to_game:
	j _racdoor_return_to_game
	addiu $sp, $sp, 0x1c0

	.global unpack
unpack:
	addiu $sp, $sp, -0x50
	sq $s0, 0x0($sp)
	sq $s1, 0x10($sp)
	sq $s2, 0x20($sp)
	sq $s3, 0x30($sp)
	sq $ra, 0x40($sp)
	
unpack_initial:
	lui $s0, %hi(_racdoor_payload)
	addiu $s0, $s0, %lo(_racdoor_payload)
	lbu $s1, 0x0($s0)
	lbu $s2, 0x1($s0)
	addiu $s3, $s0, 4
	
unpack_copy_loop:
	lwu $a0, 0x0($s3)
	lhu $a1, 0x4($s3)
	add $a1, $a1, $s0
	jal memcpy
	lhu $a2, 0x6($s3)
	
	addiu $s1, $s1, -1
	bgtz $s1, unpack_copy_loop
	addiu $s3, $s3, 8
	
unpack_fill_loop:
	lwu $a0, 0x0($s3)
	lhu $a1, 0x4($s3)
	add $a1, $a1, $s0
	jal memset
	lhu $a2, 0x6($s3)
	
	addiu $s2, $s2, -1
	bgtz $s2, unpack_fill_loop
	addiu $s3, $s3, 8

unpack_finish:
	lbu $s1, 0x3($s0)
	beq $s1, $zero, loader_run
	nop

unpack_return:
	lq $s0, 0x0($sp)
	lq $s1, 0x10($sp)
	lq $s2, 0x20($sp)
	lq $s3, 0x30($sp)
	lq $ra, 0x40($sp)
	jr $ra
	addiu $sp, $sp, 0x50
