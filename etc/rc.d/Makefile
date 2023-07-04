# $NetBSD: Makefile,v 1.16 2001/01/14 15:37:22 minoura Exp $
# $FreeBSD$

.include <bsd.own.mk>

CONFIGFILES=	DAEMON DISKS LOGIN NETWORKING SERVERS 	\
		bootconf.sh bootparams			\
		ccd cron fsck ldconfig local 		\
		makemandb motd mountcritlocal moused root \
		ttys wscons wsmoused

FILESDIR=		/etc/rc.d
FILESMODE=		${BINMODE}

NOPROG= noprog

.include <bsd.prog.mk>
