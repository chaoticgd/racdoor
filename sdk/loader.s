	.set noat
	.set noreorder
	.set nomacro
	.section .racdoor.loader
	.global loader_entry
loader_entry:
	addiu $sp, $sp, -0x1c0

# Save the values that would've been in the general purpose registers right
# before the initial hook was triggered so we can restore them and jump back to
# the game later.
loader_save_regs:
	sq $s2, 0xa0($sp)
	sq $t3, 0xc0($sp)
	sq $t8, 0x190($sp)
	# sq $t7, 0x160($sp)
	sq $a1, 0x1a0($sp)
	sq $fp, 0x80($sp)
	# sq $t2, 0x90($sp)
	sq $t5, 0x60($sp)
	# sq $s5, 0x40($sp)
	sq $t0, 0x110($sp)
	sq $gp, 0x150($sp)
	sq $t1, 0x10($sp)
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
	# sq $a0, 0xf0($sp)
	# sq $s4, 0x30($sp)
	sq $t4, 0x0($sp)
	sq $v1, 0x20($sp)
	sq $at, 0xe0($sp)
	sq $v0, 0x50($sp)

loader_flush_cache:
	jal FlushCache
	addiu $a0, $zero, 0
	jal FlushCache
	addiu $a0, $zero, 2

# Load the sections from the payload into memory. This is a relative branch
# instead of a call because nothing is loaded into its final linked location in
# memory yet, and we don't know the exact address in which the loader will be
# placed into memory ahead of time.
loader_unpack:
	b unpack_initial
	nop

loader_continue:
	jal FlushCache
	addiu $a0, $zero, 0
	jal FlushCache
	addiu $a0, $zero, 2
# Now we jump to the copy of the loader that was just unpacked so that the
# disarm function called below can re-encrypt the payload to prepare it for
# being written to the save file again.
	j loader_run
	add $s4, $zero, $zero

loader_run:
	jal apply_relocations
	nop
	jal disarm
	addiu $a0, $zero, 1
	jal install_module_hooks
	nop
	jal load_modules
	nop

# Restore the GPRs so that the processor gets into the same state it was in when
# the initial hook was run.
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

# Jump back to the game as if nothing ever happenend.
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
	
unpack_set_exit_flag:
	lui $s0, %hi(_racdoor_payload)
	addiu $s0, $s0, %lo(_racdoor_payload)
	addiu $s1, $zero, 1
	sb $s1, 0x3($s0)
	
unpack_initial:
	lui $s0, %hi(_racdoor_payload)
	addiu $s0, $s0, %lo(_racdoor_payload)
	addiu $s1, $s0, 4

unpack_copy_start:
	lbu $s2, 0x0($s0)
	beq $s2, $zero, unpack_fill_start
	nop
	
unpack_copy_loop:
	lwu $a0, 0x0($s1)
	lhu $a1, 0x4($s1)
	add $a1, $a1, $s0
	jal memcpy
	lhu $a2, 0x6($s1)
	
	addiu $s2, $s2, -1
	bgtz $s2, unpack_copy_loop
	addiu $s1, $s1, 8

unpack_fill_start:
	lbu $s2, 0x1($s0)
	beq $s2, $zero, unpack_decompress_start
	nop

unpack_fill_loop:
	lwu $a0, 0x0($s1)
	lhu $a1, 0x4($s1)
	jal memset
	lhu $a2, 0x6($s1)
	
	addiu $s2, $s2, -1
	bgtz $s2, unpack_fill_loop
	addiu $s1, $s1, 8

unpack_decompress_start:
	lbu $s2, 0x2($s0)
	beq $s2, $zero, unpack_finish
# Find the FastDecompress function. First we load the level number, and use it
# to lookup the index of the current overlay. Then, we load the address of the
# decompression function from an array that was just copied from the payload.
	lui $t0, %hi(Level)
	lw $t0, %lo(Level)($t0)
	lui $t1, %hi(_racdoor_levelmap)
	addiu $t1, %lo(_racdoor_levelmap)
	add $t0, $t1, $t0
	lbu $t1, 0x0($t0) # Load overlay index.
	lui $t0, %hi(_racdoor_fastdecompress)
	addiu $t0, %lo(_racdoor_fastdecompress)
	sll $t1, $t1, 2
	addu $t0, $t0, $t1
	lwu $s3, 0x0($t0)

unpack_decompress_loop:
	lhu $a0, 0x4($s1)
	add $a0, $a0, $s0
	jalr $s3 # FastDecompress
	lwu $a1, 0x0($s1)
	
	addiu $s2, $s2, -1
	bgtz $s2, unpack_decompress_loop
	addiu $s1, $s1, 8

# Either jump back to the loader explicitly or return if unpack was called from
# the persistence code.
unpack_finish:
	lbu $s1, 0x3($s0)
	beq $s1, $zero, loader_continue
	nop

unpack_reset_exit_flag:
	sb $zero, 0x3($s0)

unpack_return:
	lq $s0, 0x0($sp)
	lq $s1, 0x10($sp)
	lq $s2, 0x20($sp)
	lq $s3, 0x30($sp)
	lq $ra, 0x40($sp)
	jr $ra
	addiu $sp, $sp, 0x50
