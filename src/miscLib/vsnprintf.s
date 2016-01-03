	.text
	.globl	_vsnprintf
	.def	_vsnprintf
_vsnprintf:
	jmp		*__imp___vsnprintf
