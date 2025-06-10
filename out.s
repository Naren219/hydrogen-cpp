.globl	_main
.p2align 2
_main:
	mov	w1, #3
	str	w1, [sp, #-16]!
	mov	w1, #4
	str	w1, [sp, #-16]!
	ldr	w0, [sp, #16]
	str	w0, [sp, #-16]!
	ldr	w0, [sp, #16]
	str	w0, [sp, #-16]!
	ldr	w1, [sp], #16
	ldr	w0, [sp], #16
	mul	w0, w0, w1
	str	w0, [sp, #-16]!
	mov	w0, #2
	str	w0, [sp, #-16]!
	ldr	w1, [sp], #16
	ldr	w0, [sp], #16
	add	w0, w0, w1
	str	w0, [sp, #-16]!
	ldr	w0, [sp, #0]
	str	w0, [sp, #-16]!
	mov	w0, #1
	str	w0, [sp, #-16]!
	ldr	w1, [sp], #16
	ldr	w0, [sp], #16
	add	w0, w0, w1
	str	w0, [sp, #-16]!
	ldr	w0, [sp, #48]
	str	w0, [sp, #-16]!
	ldr	w0, [sp, #48]
	str	w0, [sp, #-16]!
	ldr	w1, [sp], #16
	ldr	w0, [sp], #16
	add	w0, w0, w1
	str	w0, [sp, #-16]!
	ldr	w1, [sp], #16
	ldr	w0, [sp], #16
	mul	w0, w0, w1
	str	w0, [sp, #-16]!
	ldr	w0, [sp, #0]
	mov	x16, #1
	svc	#0x80
