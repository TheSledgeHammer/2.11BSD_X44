#	$NetBSD: Makefile,v 1.292 2021/04/25 23:43:48 christos Exp $
#	from: @(#)Makefile	5.25.1.1 (Berkeley) 5/7/91
#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.4.1 (2.11BSD) 1996/1/11
#

# Programs that live in subdirectories, and have makefiles of their own.
# 
# To stay in keeping with 2.11BSD: All libs are in usr.lib
# Excluding csu, libarch, libc, libcompat & libkern

.include <bsd.own.mk>

SUBDIR=		csu .WAIT

.if (${MKGCC} != "no")
SUBDIR+=	../contrib/gnu/${EXTERNAL_GCC_SUBDIR}/lib/libgcc .WAIT
.endif

SUBDIR+=	libc
SUBDIR+=	.WAIT

#
# The SUBDIRs above are included here for completeness but should be built
# and installed prior to make(dependall) in this file, as libraries listed
# below will depend on versions from DESTDIR only.
#

SUBDIR+=	libarch libcompat

USRLIBDIR= 	../usr.lib
SUBDIR+=  	${USRLIBDIR}/libbz2 		\
            ${USRLIBDIR}/libcrypt   	\
			${USRLIBDIR}/libdbm 		\
			${USRLIBDIR}/libfortran		\
			${USRLIBDIR}/libipsec 		\
			${USRLIBDIR}/libkvm 		\
			${USRLIBDIR}/libm 			\
			${USRLIBDIR}/libmp 			\
			${USRLIBDIR}/libom 			\
			${USRLIBDIR}/libresolv 		\
			${USRLIBDIR}/libpci			\
			${USRLIBDIR}/libpthread 	\
			${USRLIBDIR}/libpthread_dbg \
			${USRLIBDIR}/libstubs  		\
			${USRLIBDIR}/libtermcap 	\
			${USRLIBDIR}/libutil 		\
			${USRLIBDIR}/libvmf 		\
			${USRLIBDIR}/liby 			\
			${USRLIBDIR}/libz

.if !defined(BSD_MK_COMPAT_FILE)
SUBDIR+=	libkern
.endif

.if (${MKSKEY} != "no")
SUBDIR+=	${USRLIBDIR}/libskey
.endif

SUBDIR+=	../contrib/flex/lib
SUBDIR+=	../contrib/elftoolchain/libelf
SUBDIR+=	../contrib/liblzf/lib

SUBDIR+=	${USRLIBDIR}/libpcap

SUBDIR+=	../contrib/expat/lib

SUBDIR+=	../contrib/gnu/libmalloc

.if (${MKGCC} != "no")
SUBDIR+=	../contrib/gnu/${EXTERNAL_GCC_SUBDIR}/lib/libobjc
SUBDIR+=	../contrib/gnu/${EXTERNAL_GCC_SUBDIR}/lib/libgomp
# Should probably move GMP, MPFR and MPC builds into the GCC >= 4.5
# specific build area, but we get better parallelism this way.
# We don't build compat versions of these.
. if !defined(MLIBDIR) && (${MKGCCCMDS} != "no")
SUBDIR+=	../contrib/gnu/gmp/lib/libgmp
SUBDIR+=	../contrib/gnu/mpfr/lib/libmpfr
SUBDIR+=	../contrib/gnu/mpc/lib/libmpc
. endif
.endif

#
# Libraries that depend upon any listed previously
# (and those that depend upon these [and ...])
#
#==================== 1st library dependency barrier ====================
SUBDIR+=	.WAIT

SUBDIR+=	../contrib/xz/lib			# depends on libpthread
SUBDIR+=	../contrib/elftoolchain/libdwarf # depends on libelf
SUBDIR+=	../contrib/lua/lib          # depends on libm
SUBDIR+=	../contrib/sqlite/lib		# depends on libm
SUBDIR+= 	${USRLIBDIR}/libcurses		# depends on libterminfo/libtermcap
SUBDIR+=	${USRLIBDIR}/libedit		# depends on libterminfo
SUBDIR+=	${USRLIBDIR}/libexecinfo 	# depends on libelf

SUBDIR+=	../crypto/${EXTERNAL_OPENSSL_SUBDIR}/lib # depends on libcrypt

.if (${MKBINUTILS} != "no")
SUBDIR+=	../contrib/gnu/${EXTERNAL_BINUTILS_SUBDIR}/lib	# libbfd depends on libz
.endif

.if (${MKLIBCXX} != "no")
SUBDIR+=	../contrib/libc++
.endif

.if (${MKGCC} != "no" && ${MKCXX} != "no" && ${MKLIBSTDCXX} != "no")
SUBDIR+=	../contrib/gnu/${EXTERNAL_GCC_SUBDIR}/lib/libstdc++-v3
SUBDIR+=	../contrib/gnu/${EXTERNAL_GCC_SUBDIR}/lib/libsupc++
.endif

.if (${MKLLVMRT} != "no" && (!defined(MLIBDIR) || ${MKCOMPATX11} != "no"))
SUBDIR+=	../contrib/llvm/include
.endif

#==================== 2nd library dependency barrier ====================
SUBDIR+=	.WAIT

SUBDIR+=	${USRLIBDIR}/libwrap

.if (${MKGCC} != "no" && ${MKCXX} != "no" && ${MKLIBSTDCXX} != "no")
.for sanitizer in asan lsan ubsan
.if exists(../contrib/gnu/${EXTERNAL_GCC_SUBDIR}/lib/lib${sanitizer})
SUBDIR+= ../contrib/gnu/${EXTERNAL_GCC_SUBDIR}/lib/lib${sanitizer}
.endif
.endfor
.endif

.if (${MKLLVMRT} != "no" && (!defined(MLIBDIR) || ${MKCOMPATX11} != "no"))
SUBDIR+=	../contrib/llvm/librt
.endif

SUBDIR+=	../contrib/libarchive/lib	# depends on libxz, libcrypto
SUBDIR+=	../contrib/file/lib		# depends on libz, libbz2, libxz

SUBDIR+=	${USRLIBDIR}/libform		# depends on libcurses
SUBDIR+=	${USRLIBDIR}/libmenu		# depends on libcurses
SUBDIR+=	${USRLIBDIR}/libpanel		# depends on libcurses

SUBDIR+=	../contrib/fetch/lib	    # depends on libssl

SUBDIR+= 	../libexec/httpd/libbozohttpd 	# depends on libssl & libcrypto

#==================== 3rd library dependency barrier ====================
SUBDIR+=	.WAIT

SUBDIR+=	../crypto/openssh/lib 		# depends on libcrypto, libz

SUBDIR+=	../contrib/mdocml/lib

.include <bsd.buildinstall.mk>
.include <bsd.subdir.mk>
