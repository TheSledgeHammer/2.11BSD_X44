#	$NetBSD: Makefile,v 1.285 2020/08/20 21:30:46 riastradh Exp $
#	from: @(#)Makefile	5.20 (Berkeley) 6/12/93

.include <bsd.own.mk>

SUBDIR=	boot0cfg btxld 											\
		catman chown chroot config cron 						\
		dev_mkdb 												\
		etcupdate 												\
		link                                                    \
        makefs mtree moused                                            \
		periodic pwd_mkdb 							\
        rmt                                                     \
		unlink  												\
		vnconfig 												\
		wsconscfg wsfontload wsmoused wsmuxctl					\
        zdump zic

.if ${MKMAKEMANDB} != "no"
SUBDIR+= makemandb
.endif

.if ${TOOLCHAIN_MISSING} != "no"
SUBDIR+= mdsetimage
.endif

.include <bsd.subdir.mk>
