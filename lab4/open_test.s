	.file	1 "open_test.c"

 # GNU C 2.7.2.3 [AL 1.1, MM 40, tma 0.1] SimpleScalar running sstrix compiled by GNU C

 # Cc1 defaults:
 # -mgas -mgpOPT

 # Cc1 arguments (-G value = 8, Cpu = default, ISA = 1):
 # -quiet -dumpbase -O1 -o

gcc2_compiled.:
__gnu_compiled_c:
	.text
	.align	2
	.globl	main

	.extern	stdin, 4
	.extern	stdout, 4

	.text

	.loc	1 14
	.ent	main
main:
	.frame	$sp,48,$31		# vars= 8, regs= 6/0, args= 16, extra= 0
	.mask	0x801f0000,-4
	.fmask	0x00000000,0
	subu	$sp,$sp,48
	sw	$31,44($sp)
	sw	$20,40($sp)
	sw	$19,36($sp)
	sw	$18,32($sp)
	sw	$17,28($sp)
	sw	$16,24($sp)
	jal	__main
	li	$4,0x00004000		# 16384
	jal	malloc
	li	$4,0x00004000		# 16384
	jal	malloc
	li	$4,0x00004000		# 16384
	jal	malloc
	li	$4,0x00004000		# 16384
	jal	malloc
	move	$19,$0
	move	$20,$0
	move	$18,$0
	move	$17,$0
$L25:
	bne	$20,$0,$L26
	li	$4,0x00000004		# 4
	jal	malloc
	move	$18,$2
	move	$20,$18
	j	$L27
$L26:
	li	$4,0x00000004		# 4
	jal	malloc
	sw	$2,0($18)
	move	$18,$2
$L27:
	addu	$19,$19,1
	move	$16,$0
	blez	$19,$L24
$L31:
	li	$4,0x00000004		# 4
	jal	malloc
	addu	$16,$16,1
	slt	$2,$16,$19
	bne	$2,$0,$L31
$L24:
	addu	$17,$17,1
	slt	$2,$17,256
	bne	$2,$0,$L25
	sw	$20,0($18)
	move	$17,$0
	li	$3,0x00063fff		# 409599
$L37:
	addu	$17,$17,1
	slt	$2,$3,$17
	beq	$2,$0,$L37
	move	$2,$0
	lw	$31,44($sp)
	lw	$20,40($sp)
	lw	$19,36($sp)
	lw	$18,32($sp)
	lw	$17,28($sp)
	lw	$16,24($sp)
	addu	$sp,$sp,48
	j	$31
	.end	main
