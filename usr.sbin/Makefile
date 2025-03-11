#	$NetBSD: Makefile,v 1.285 2020/08/20 21:30:46 riastradh Exp $
#	from: @(#)Makefile	5.20 (Berkeley) 6/12/93

.include <bsd.own.mk>

SUBDIR=	ac accton arp authpf 						\
		bad144 boot0cfg btxld 						\
		catman chown chroot config cron 			\
		dev_mkdb diskpart							\
		kvm_mkdb 									\
		link                                        \
        makefs mdconfig mrouted mtree moused        \
        netgroup_mkdb								\
		pcictl portmap pstat pwd_mkdb 					\
        rmt                                         \
		sa service services_mkdb sliplogin			\
        traceroute 									\
		unlink update user  								\
		vipw vnconfig 									\
		wsconscfg wsfontload wsmoused wsmuxctl		\
        zdump zic

.if ${MKMAKEMANDB} != "no"
SUBDIR+= makemandb
.endif

.if (${MKYP} != "no")
SUBDIR+= ypbind yppoll ypserv ypset
.endif

.if ${TOOLCHAIN_MISSING} != "no"
SUBDIR+= mdsetimage
.endif

#.if (${MKINET6} != "no")
#SUBDIR+=
#.endif

.if (${USE_INET6} != "no")
SUBDIR+= traceroute6 
.endif

.if (${USE_NDBM} != "no)
SUBDIR+= mkpasswd
#.else
#SUBDIR+= 
.endif

.include <bsd.subdir.mk>
