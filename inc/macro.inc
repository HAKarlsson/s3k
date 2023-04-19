// See LICENSE file for copyright and license details.
.extern stack_top
.macro load_sp tmp
	la	sp,_sp
	csrr	\tmp,mhartid
	slli	\tmp,\tmp,LOG_STACK_SIZE
	sub	sp,sp,\tmp
.endm
