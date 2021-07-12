#	$NetBSD: Makefile,v 1.14 2018/05/01 19:59:46 christos Exp $

.include <bsd.hostinit.mk>

HOSTPROGNAME=	${MACHINE_GNU_PLATFORM}-install
HOST_SRCDIR=	usr.bin/xinstall

.include "${.CURDIR}/../Makefile.host"

# Use uninstalled copy of the install program
INSTALL=	./xinstall