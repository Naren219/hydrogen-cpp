.globl	_main
.p2align 2
_main:
	mov	w0, #10
	str	w0, [sp, #-16]!
	mov	w0, #3
	str	w0, [sp, #-16]!
	ldr	w1, [sp], #16
	ldr	w0, [sp], #16
	add	w0, w0, w1
	str	w0, [sp, #-16]!
	mov	w0, #2
	str	w0, [sp, #-16]!
	ldr	w1, [sp], #16
	ldr	w0, [sp], #16
	mul	w0, w0, w1
	str	w0, [sp, #-16]!
	ldr	w0, [sp, #0]
	mov	x16, #1
	svc	#0x80
