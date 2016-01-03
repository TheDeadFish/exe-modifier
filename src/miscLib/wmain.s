	.globl	_RawEntryPoint
	.def	_RawEntryPoint;	.scl	2;	.type	32;	.endef
_HookEntryPoint:
	subl	$12, %esp
	pushl	$0
	pushl	%esp
	pushl	$1
	leal	20(%esp), %eax
	pushl	%eax
	leal	20(%esp), %eax
	pushl	%eax
	leal	20(%esp), %eax
	pushl	%eax
	call	*__imp____wgetmainargs
	addl	$24, %esp
	call	__ctor_wmain
	pushl	%eax
	call	*__imp___cexit
	call	_ExitProcess@4
