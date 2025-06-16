.globl	_main
.p2align 2
_main:
	mov	w1, #2
	str	w1, [sp, #-16]!
	ldr	w0, [sp, #0]
	str	w0, [sp, #-16]!
	mov	w0, #2
	str	w0, [sp, #-16]!
	ldr	w1, [sp], #16
	ldr	w0, [sp], #16
	add	w0, w0, w1
	str	w0, [sp, #-16]!
	ldr	w0, [sp], #16
	cmp	w0, #0
	b.eq	.L0
	mov	w0, #4
	mov	x16, #1
	svc	#0x80
	.L0:
	mov	w0, #5
	mov	x16, #1
	svc	#0x80
	mov	w0, #0
	mov	x16, #1
	svc	#0x80
