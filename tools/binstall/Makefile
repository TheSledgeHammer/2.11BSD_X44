#	$NetBSD: Makefile,v 1.14 2018/05/01 19:59:46 christos Exp $

.include <bsd.hostinit.mk>

HOSTPROGNAME=	${MACHINE_GNU_PLATFORM}-install
HOST_SRCDIR=	usr.bin/xinstall
CPPFLAGS+=		-I${.CURDIR}/../compat/sys
CPPFLAGS+=		-DTARGET_STRIP=\"${STRIP}\"

# Prevent Makefile.host from trying to use the version of libnbcompat
# from ${TOOLDIR}.
NOCOMPATLIB=

# Use uninstalled copy of host-mkdep
HOST_MKDEP_OBJ!= cd ${.CURDIR}/../host-mkdep && ${PRINTOBJDIR}
HOST_MKDEP=	${HOST_MKDEP_OBJ}/host-mkdep
MKDEP=		${HOST_MKDEP}

# Use uninstalled copy of libnbcompat and associated *.h files
COMPATLIB_UNINSTALLED= yes
COMPATOBJ!=	cd ${NETBSDSRCDIR}/tools/compat && ${PRINTOBJDIR}
.-include	"${COMPATOBJ}/defs.mk"

.include "${.CURDIR}/../Makefile.host"

# Use uninstalled copy of the install program
INSTALL=	./xinstall