.globl	_main
.p2align 2
_main:
	mov	w0, #0
	str	w0, [sp, #-16]!
	ldr	w0, [sp], #16
	cmp	w0, #0
	b.eq	.L0_skip
	mov	w0, #3
	mov	x16, #1
	svc	#0x80
	b	.L0_end
.L0_skip:
	mov	w0, #1
	str	w0, [sp, #-16]!
	ldr	w0, [sp], #16
	cmp	w0, #0
	b.eq	.L1_skip
	mov	w0, #4
	mov	x16, #1
	svc	#0x80
	b	.L0_end
.L1_skip:
	mov	w0, #5
	mov	x16, #1
	svc	#0x80
.L0_end:
	mov	w0, #0
	mov	x16, #1
	svc	#0x80
