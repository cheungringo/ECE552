	.file	1 "benchmark3.c"

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

	.text

	.loc	1 1
	.ent	main
main:
	.frame	$sp,24,$31		# vars= 0, regs= 1/0, args= 16, extra= 0
	.mask	0x80000000,-8
	.fmask	0x00000000,0
	subu	$sp,$sp,24
	sw	$31,16($sp)
	jal	__main
	move	$4,$0
	move	$3,$0
	li	$6,0x000f423f		# 999999
$L5:
	addu	$4,$4,1
	addu	$5,$4,1
 #APP
	lw $8, 16($sp)
	addi $7, $8, 1
 #NO_APP
	addu	$3,$3,1
	slt	$2,$6,$3
	beq	$2,$0,$L5
	move	$2,$5
	lw	$31,16($sp)
	addu	$sp,$sp,24
	j	$31
	.end	main
