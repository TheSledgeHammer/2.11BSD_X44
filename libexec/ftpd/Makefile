#	$NetBSD: Makefile,v 1.63 2011/08/14 11:46:28 christos Exp $
#	@(#)Makefile	8.2 (Berkeley) 4/4/94

.include <bsd.own.mk>

PROG=	ftpd
SRCS=	cmds.c conf.c ftpd.c ftpcmd.y logutmp.c logwtmp.c popen.c
CPPFLAGS+=-I${.CURDIR} -DSUPPORT_UTMP -DSUPPORT_UTMPX -DLOGIN_CAP
DPADD+=	${LIBCRYPT} ${LIBUTIL}
LDADD+=	-lcrypt -lutil
MAN=	ftpd.conf.5 ftpusers.5 ftpd.8
MLINKS=	ftpusers.5 ftpchroot.5

.if defined(NO_INTERNAL_LS)
CPPFLAGS+=-DNO_INTERNAL_LS
.else
# for `internal' ls
SRCS+=	ls.c cmp.c print.c util.c
.PATH:	${NETBSDSRCDIR}/bin/ls
.endif

.if (${USE_INET6} != "no")
CPPFLAGS+=-DINET6
.endif

.if (${USE_PAM} != "no")
CPPFLAGS+=-DUSE_PAM
DPADD+= ${LIBPAM} ${PAM_STATIC_DPADD}
LDADD+= -lpam ${PAM_STATIC_LDADD}
.else	# USE_PAM == no
.if (${USE_SKEY} != "no")
CPPFLAGS+=-DSKEY
DPADD+= ${LIBSKEY}
LDADD+= -lskey
.endif
.endif	# USE_PAM == no

ftpd.o ftpcmd.o: version.h

#.if (${USE_KERBEROS} != "no")
#
#.PATH:	${NETBSDSRCDIR}/usr.bin/login
#
#SRCS+=	k5login.c
#CPPFLAGS+=-DKERBEROS5
#DPADD+=	${LIBKRB5} ${LIBASN1}
#LDADD+= -lkrb5 -lasn1
#
#DPADD+=	${LIBCRYPTO} ${{LIBCRYPT} ${LIBROKEN} ${LIBCOM_ERR}
#LDADD+=	-lcrypto -lcrypt -lroken -lcom_err
#
#.endif

COPTS.ftpd.c+=		${GCC_NO_STRINGOP_TRUNCATION} ${GCC_NO_FORMAT_TRUNCATION}
COPTS.logwtmp.c+=	${GCC_NO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
