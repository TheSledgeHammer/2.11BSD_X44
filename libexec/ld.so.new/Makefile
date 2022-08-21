#	$NetBSD: Makefile,v 1.69 2004/03/24 08:01:26 skrll Exp $

.include <bsd.own.mk>			# for OBJECT_FMT definition
.include <bsd.shlib.mk>			# for SHLINKINSTALLDIR definition

ARCHSUBDIR=	${MACHINE_CPU}

LD_AOUT= 	aout
LD_ELF= 	elf
LD_ECOFF= 	ecoff
LD_XCOFF= 	xcoff
LD_DEFUALT= ${LD_ELF}

PROG_NAME=	ld.so

.if ${SHLINKINSTALLDIR} != "/usr/libexec"
SYMLINKS+=	${SHLINKINSTALLDIR}/${PROG_NAME} /usr/libexec/${PROG_NAME}
.endif

.include <bsd.prog.mk>
.else

MAN=		ld.so.1

.include <bsd.man.mk>
.endif