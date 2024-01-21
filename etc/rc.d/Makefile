# $NetBSD: Makefile,v 1.16 2001/01/14 15:37:22 minoura Exp $
# $FreeBSD$

.include <bsd.own.mk>

CONFIGFILES=	DAEMON DISKS LOGIN NETWORKING SERVERS 		\
                accounting                                  \
				bootconf.sh bootparams						\
				ccd cron                                    \
                dmesg                                       \
                fsck                                        \
                identd ipsec                                \
                ldconfig local 				                \
				makemandb motd mountcritlocal mountd moused \
                mrouted                                     \
                named network                               \
                pf pflogd ppp                               \
                root route6d routed rpcbind                 \
                savecore securelevel swap1 swap2            \
				timed ttys                                  \
                wscons wsmoused

FILESDIR=		/etc/rc.d
FILESMODE=		${BINMODE}

NOPROG= noprog

.include <bsd.prog.mk>
