	.text
	.globl	_snprintf
	.def	_snprintf
_snprintf:
	jmp		*__imp___snprintf
