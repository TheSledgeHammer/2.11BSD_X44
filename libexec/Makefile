#	$NetBSD: Makefile,v 1.61 2012/02/07 19:13:31 joerg Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/4/93

.include <bsd.own.mk>

SUBDIR= acctd atrun ctimed getty ld.aout_so ld.elf_so lfs_cleanerd makekey

.if ${MKMAKEMANDB} == "no"
SUBDIR+= getNAME makewhatis
.endif

.include <bsd.subdir.mk>
