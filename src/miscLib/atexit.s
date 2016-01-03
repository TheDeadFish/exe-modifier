	.text
	.globl	_atexit
	.def	_atexit
_atexit:
	jmp		*__imp__atexit
