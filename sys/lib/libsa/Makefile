#	$NetBSD: Makefile,v 1.101 2018/06/05 00:57:47 christos Exp $

LIB= 			sa
LIBISPRIVATE?= 	yes

SA_INCLUDE_NET?= 		YES		# Netboot via TFTP, NFS (No NFS Support)
SA_USE_LOADFILE?= 		no		# Generic executable loading support
SA_ENABLE_BIENDIAN?= 	no		# Biendian support
								# (currently only for loadfile_elf*)
							
.include "Makefile.libsa"

.ifndef ARCHSUBDIR
.BEGIN:
	@echo no ARCHSUBDIR for ${MACHINE_ARCH} nor ${MACHINE_CPU}
	@false
.endif

# only needed during build
libinstall::

.undef DESTDIR
.include <bsd.lib.mk>

lib${LIB}.o:: ${OBJS:O} __buildstdlib

lib${LIB}.po:: ${POBJS:O} __buildproflib

showsources: ${SRCS:O}
	@echo ${.ALLSRC}
