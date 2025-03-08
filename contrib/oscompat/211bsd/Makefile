#	$211BSD: Makefile,v 1.0 2025/03/08 21:15:27 Exp $

.include <bsd.own.mk>
.include "twolib.mk"

.if (${MKLIBNDBM} != "no")
SUBDIR+= 	libndbm
.endif

.if (${MKLIBFORTRAN} != "no")
SUBDIR+= 	libfortran
.endif

.if (${MKLIBMP} != "no")
SUBDIR+= 	libmp
.endif

.if (${MKLIBOM} != "no")
SUBDIR+= 	libom
.endif

.if (${MKLIBSTUBS} != "no")
SUBDIR+= 	libstubs
.endif

.if (${MKLIBVMF} != "no")
SUBDIR+= 	libvmf
.endif

.include <bsd.subdir.mk>