#	$NetBSD: Makefile,v 1.61 2012/02/07 19:13:31 joerg Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/4/93

.include <bsd.own.mk>

SUBDIR= acctd \
		comsat \
		fingerd ftpd \
		getty \
		httpd \
		identd \
		ld.aout_so ld.elf_so lfs_cleanerd \
		makekey \
		talkd tftpd \
		utmp_update

.if ${MKMAKEMANDB} == "no"
SUBDIR+= getNAME makewhatis
.endif

.include <bsd.subdir.mk>

#
# TODO:
# Below cannot be built. Will halt compilation once
# reached, if added to SUBDIR.
# fix: 
# - atrun: 
#	- compiler will output cannot make atrun.1
# - ctimed: 
# 	- compiler will output cannot make ctimed.1
#
