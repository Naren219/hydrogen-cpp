.globl	_main
.p2align 2
_main:
	mov	w1, #0
	str	w1, [sp, #-16]!
	# Default exit
	mov	w0, #0
	mov	x16, #1
	svc	#0x80
