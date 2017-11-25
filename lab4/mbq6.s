	.file	1 "./mbq6.c"

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

	.loc	1 10
	.ent	main
main:
	.frame	$sp,56,$31		# vars= 0, regs= 9/0, args= 16, extra= 0
	.mask	0x80ff0000,-8
	.fmask	0x00000000,0
	subu	$sp,$sp,56
	sw	$31,48($sp)
	sw	$23,44($sp)
	sw	$22,40($sp)
	sw	$21,36($sp)
	sw	$20,32($sp)
	sw	$19,28($sp)
	sw	$18,24($sp)
	sw	$17,20($sp)
	sw	$16,16($sp)
	jal	__main
	li	$4,0x00010000		# 65536
	jal	malloc
	move	$23,$2
	li	$4,0x00010000		# 65536
	jal	malloc
	move	$22,$2
	li	$4,0x00010000		# 65536
	jal	malloc
	move	$21,$2
	li	$4,0x00010000		# 65536
	jal	malloc
	move	$20,$2
	li	$4,0x00010000		# 65536
	jal	malloc
	move	$19,$2
	li	$4,0x00010000		# 65536
	jal	malloc
	move	$18,$2
	move	$16,$0
	move	$17,$0
$L25:
	li	$2,-1840700269			# 0x92492493
	mult	$17,$2
	.set	noreorder
	mfhi	$3
	mflo	$2
	#nop
	#nop
	.set	reorder
	srl	$2,$3,0
	move	$3,$0
	addu	$2,$17,$2
	sra	$2,$2,3
	sra	$4,$17,31
	subu	$2,$2,$4
	sll	$5,$2,3
	subu	$5,$5,$2
	sll	$5,$5,1
	li	$4,0x00000002		# 2
	subu	$5,$17,$5
	jal	pow
	move	$3,$2
	bgez	$3,$L26
	addu	$2,$3,16383
$L26:
	sra	$2,$2,14
	sll	$2,$2,14
	subu	$2,$3,$2
	sll	$2,$2,2
	addu	$2,$2,$23
	.set	noreorder
	lw	$2,0($2)
	#nop
	.set	reorder
	addu	$16,$16,$2
	move	$2,$3
	bgez	$3,$L27
	addu	$2,$3,16383
$L27:
	sra	$2,$2,14
	sll	$2,$2,14
	subu	$2,$3,$2
	sll	$2,$2,2
	addu	$2,$2,$22
	.set	noreorder
	lw	$2,0($2)
	#nop
	.set	reorder
	addu	$16,$16,$2
	move	$2,$3
	bgez	$3,$L28
	addu	$2,$3,16383
$L28:
	sra	$2,$2,14
	sll	$2,$2,14
	subu	$2,$3,$2
	sll	$2,$2,2
	addu	$2,$2,$21
	.set	noreorder
	lw	$2,0($2)
	#nop
	.set	reorder
	addu	$16,$16,$2
	move	$2,$3
	bgez	$3,$L29
	addu	$2,$3,16383
$L29:
	sra	$2,$2,14
	sll	$2,$2,14
	subu	$2,$3,$2
	sll	$2,$2,2
	addu	$2,$2,$20
	.set	noreorder
	lw	$2,0($2)
	#nop
	.set	reorder
	addu	$16,$16,$2
	move	$2,$3
	bgez	$3,$L30
	addu	$2,$3,16383
$L30:
	sra	$2,$2,14
	sll	$2,$2,14
	subu	$2,$3,$2
	sll	$2,$2,2
	addu	$2,$2,$19
	.set	noreorder
	lw	$2,0($2)
	#nop
	.set	reorder
	addu	$16,$16,$2
	move	$2,$3
	bgez	$3,$L31
	addu	$2,$3,16383
$L31:
	sra	$2,$2,14
	sll	$2,$2,14
	subu	$2,$3,$2
	sll	$2,$2,2
	addu	$2,$2,$18
	.set	noreorder
	lw	$2,0($2)
	.set	reorder
	addu	$17,$17,1
	addu	$16,$16,$2
	li	$2,0x00009fff		# 40959
	slt	$2,$2,$17
	beq	$2,$0,$L25
	move	$2,$16
	lw	$31,48($sp)
	lw	$23,44($sp)
	lw	$22,40($sp)
	lw	$21,36($sp)
	lw	$20,32($sp)
	lw	$19,28($sp)
	lw	$18,24($sp)
	lw	$17,20($sp)
	lw	$16,16($sp)
	addu	$sp,$sp,56
	j	$31
	.end	main
