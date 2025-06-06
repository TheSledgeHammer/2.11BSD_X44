#	$NetBSD: Makefile,v 1.1 1996/02/21 02:45:47 jtk Exp $


#MANSUBDIR=i386

.if (${MACHINE} == "i386" || ${MLIBDIR:Unone} == "i386")
SRCS+=	i386_iopl.c i386_vm86.c \
		i386_get_fsbase.c i386_get_gsbase.c i386_get_ldt.c i386_get_ioperm.c \
		i386_set_fsbase.c i386_set_gsbase.c i386_set_ldt.c i386_set_ioperm.c 
.endif

MAN+=	i386_get_fsbase.2 i386_get_gsbase.2 i386_get_ldt.2 i386_get_ioperm.2 \
		i386_iopl.2 i386_vm86.2
		
MLINKS+=i386_get_fsbase.2 i386_set_fsbase.2 \
		i386_get_gsbase.2 i386_set_gsbase.2	\
		i386_get_ldt.2 i386_set_ldt.2 		\
		i386_get_ioperm.2 i386_set_ioperm.2 \
