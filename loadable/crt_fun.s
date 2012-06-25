	.set noreorder
	.set volatile
	.set noat
	.set nomacro
	.arch ev6
	.text
	.align 2
	.align 4
	.globl _start
	.ent _start
_start:
	.frame $18,16,$15,0
#MT: unsupported .mask 0x4000000,-16
	ldah $17,0($14)		!gpdisp!1
	lda $17,0($17)		!gpdisp!1
$_start..ng:
	ldq $14,lmain($17)		!literal!2
	lda $18,-16($18)
	stq $15,0($18)
	.prologue 1
	jsr $15,($14),lmain		!lituse_jsr!2
	ldah $17,0($15)		!gpdisp!3
	ldq $15,0($18)
	lda $17,0($17)		!gpdisp!3
	lda $18,16($18)
	ret $31,($15),1
	.end _start
	.ident	"GCC: (GNU) 4.5.2"
	.section	.note.GNU-stack,"",@progbits
