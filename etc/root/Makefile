#	$NetBSD: Makefile,v 1.4 2012/11/17 23:08:38 uwe Exp $
#
# Public domain - 1997/1/10 - sms
#
#	@(#)Makefile	1.0 (2.11BSD) 1997/1/10
#
# DESTDIR is defined here to be something which does *NOT* exist - it must be
#  specified on the command line when doing a "make DESTDIR=/mnt distribution".
#  This is aimed at avoiding overwriting the system disk's /etc files.

CONFIGFILES=			dot.cshrc dot.klogin dot.login dot.profile dot.shrc

FILESDIR=				/root
FILESMODE=				644
FILESMODE_dot.klogin=	600
.for F in ${CONFIGFILES}
FILESNAME_${F}=			${F:S/dot//}
.endfor

CONFIGLINKS=			/root/.cshrc	/.cshrc 	\
						/root/.profile	/.profile

.include <bsd.files.mk>
.include <bsd.links.mk>