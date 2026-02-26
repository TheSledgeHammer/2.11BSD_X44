#	$NetBSD: Makefile,v 1.4 2012/08/10 12:20:12 joerg Exp $

BINDIR=	/usr/bin

PROG=	ssh-keygen
SRCS=	ssh-keygen.c sshsig.c

COPTS.ssh-keygen.c+=	-Wno-pointer-sign ${GCC_NO_FORMAT_TRUNCATION}
COPTS.sshsig.c+= -Wno-pointer-sign

.include <bsd.prog.mk>
