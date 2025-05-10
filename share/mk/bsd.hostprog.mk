#	$NetBSD: bsd.hostprog.mk,v 1.84 2021/03/27 02:46:45 simonb Exp $
#	@(#)bsd.prog.mk	8.2 (Berkeley) 4/2/94

.include <bsd.hostinit.mk>
.include <bsd.sys.mk>

##### Basic targets

##### Default values

LIBC?=				/usr/lib/libc.a
LIBCOMPAT?=			/usr/lib/libcompat.a
LIBC_PIC?=			/usr/lib/libc_pic.a
LIBC_SO?=			/usr/lib/libc.so
LIBDBM?=			/usr/lib/libdbm.a
LIBDES?=		    /usr/lib/libdes.a
LIBEVENT?=	        /usr/lib/libevent.a
LIBEVENT_OPENSSL?=	/usr/lib/libevent_openssl.a
LIBEVENT_PTHREADS?=	/usr/lib/libevent_pthreads.a
LIBEXPAT?=			/usr/lib/libexpat.a
LIBFETCH?=			/usr/lib/libfetch.a
LIBGCC?=			/usr/lib/libgcc.a
LIBGNUCTF?=			/usr/lib/libgnuctf.a
LIBGNUMALLOC?=		/usr/lib/libgnumalloc.a
LIBL?=				/usr/lib/libl.a
LIBLZMA?=			/usr/lib/liblzma.a
LIBMAGIC?=			/usr/lib/libmagic.a
LIBNTP?=			/usr/lib/libntp.a
LIBOBJC?=			/usr/lib/libobjc.a
LIBPC?=				/usr/lib/libpc.a
LIBPLOT?=			/usr/lib/libplot.a
LIBSQLITE3?=		/usr/lib/libsqlite3.a
LIBSSP?=		    /usr/lib/libssp.a
LIBSTDCXX?=			/usr/lib/libstdc++.a
LIBSUPCXX?=			/usr/lib/libsupc++.a

LIBBZ2?=			/usr.lib/libbz2.a
LIBCRYPT?=			/usr.lib/libcrypt.a
LIBCURSES?=			/usr.lib/libcurses.a
LIBEDIT?=			/usr.lib/libedit.a
LIBFORM?=			/usr.lib/libform.a
LIBINTL?=			/usr.lib/libintl.a
LIBIPSEC?=		    /usr.lib/libipsec.a
LIBKVM?=			/usr.lib/libkvm.a
LIBM?=				/usr.lib/libm.a
LIBMENU?=			/usr.lib/libmenu.a
LIBPANEL?=			/usr.lib/libpanel.a
LIBPCAP?=			/usr.lib/libpcap.a
LIBPCI?=			/usr.lib/libpci.a
LIBPTHREAD?=		/usr.lib/libpthread.a
LIBRESOLV?=			/usr.lib/libresolv.a
LIBRPCSVC?=			/usr.lib/librpcsvc.a
LIBSKEY?=			/usr.lib/libskey.a
LIBTERMINFO?=		/usr.lib/libterminfo.a
LIBUTIL?=			/usr.lib/libutil.a
LIBWRAP?=			/usr.lib/libwrap.a
LIBY?=				/usr.lib/liby.a
LIBZ?=				/usr.lib/libz.a

# OS Compat Libraries

# 2.11BSD Libraries
LIBNDBM?=			/usr.lib/libndbm.a
LIBFORTRAN?=		/usr.lib/libfortran
LIBF77?=			${LIBFORTRAN}/libF77.a
LIBI77?=			${LIBFORTRAN}/libI77.a
LIBU77?=			${LIBFORTRAN}/libU77.a
LIBMP?=				/usr.lib/libmp.a
LIBOM?=				/usr.lib/libom.a
LIBSTUBS?=			/usr.lib/libstubs.a
#LIBTERMCAP?=		/usr.lib/libtermcap.a
LIBVMF?=			/usr.lib/libvmf.a

# NetBSD
LIBNETBSD?=			/usr/lib/libnetbsd.a

# Plan9 Libraries
LIBBIO?=			/usr/lib/libbio.a
LIBFMT?=            		/usr/lib/libfmt.a
LIBREGEXP?=         		/usr/lib/libregexp.a
LIBUTF?=            		/usr/lib/libutf.a

MKDEP_SUFFIXES?=	.lo .ln .d

# Override these:
INSTALL:=	${INSTALL:NSTRIP=*}
MKDEP:=		${HOST_MKDEP}
MKDEPCXX:=	${HOST_MKDEPCXX}

.if ${TOOLCHAIN_MISSING} == "no" || defined(EXTERNAL_TOOLCHAIN)
OBJHOSTMACHINE=	# set
.endif

##### Build rules
.if defined(HOSTPROG_CXX)
HOSTPROG=	${HOSTPROG_CXX}
.endif

.if defined(HOSTPROG)
SRCS?=			${HOSTPROG}.c

_YHPSRCS=		${SRCS:M*.[ly]:C/\..$/.c/} ${YHEADER:D${SRCS:M*.y:.y=.h}}
DPSRCS+=		${_YHPSRCS}
CLEANFILES+=	${_YHPSRCS}

.if !empty(SRCS:N*.h:N*.sh)
OBJS+=		${SRCS:N*.h:N*.sh:R:S/$/.lo/g}
LOBJS+=		${LSRCS:.c=.ln} ${SRCS:M*.c:.c=.ln}
.endif

.if defined(OBJS) && !empty(OBJS)
.NOPATH: ${OBJS} ${HOSTPROG} ${_YHPSRCS}

${OBJS} ${LOBJS}: ${DPSRCS}
${HOSTPROG}: ${OBJS} ${DPADD}
	${_MKTARGET_LINK}
	${HOST_LINK.c} ${HOST_LDSTATIC} -o ${.TARGET} ${OBJS} ${LDADD}
.if !empty(.MAKE.OS:M*CYGWIN*)
	${HOST_SH} ${NETBSDSRCDIR}/tools/binstall/mkmanifest ${HOSTPROG}
.endif


.endif	# defined(OBJS) && !empty(OBJS)

.if !defined(MAN)
MAN=	${HOSTPROG}.1
.endif	# !defined(MAN)
.endif	# defined(HOSTPROG)

realall: ${HOSTPROG}

CLEANFILES+= a.out [Ee]rrs mklog core *.core ${HOSTPROG} ${OBJS} ${LOBJS}

beforedepend:
CFLAGS:=	${HOST_CFLAGS}
CPPFLAGS:=	${HOST_CPPFLAGS:N-Wp,-iremap,*}

lint: ${LOBJS}
.if defined(LOBJS) && !empty(LOBJS)
	${LINT} ${LINTFLAGS} ${LDFLAGS:C/-L[  ]*/-L/Wg:M-L*} ${LOBJS} ${LDADD}
.endif

##### Pull in related .mk logic
LINKSMODE?= ${BINMODE}
.include <bsd.man.mk>
.include <bsd.nls.mk>
.include <bsd.files.mk>
.include <bsd.inc.mk>
.include <bsd.links.mk>
.include <bsd.dep.mk>
.include <bsd.clean.mk>

${TARGETS}:	# ensure existence

# Override YACC/LEX rules so nbtool_config.h can be forced as the 1st include
.l.c:
	${_MKTARGET_LEX}
	${LEX.l} -o${.TARGET} ${.IMPSRC}
	echo '#if HAVE_NBTOOL_CONFIG_H' > ${.TARGET}.1
	echo '#include "nbtool_config.h"' >> ${.TARGET}.1
	echo '#endif' >> ${.TARGET}.1
	cat ${.TARGET} >> ${.TARGET}.1
	${MV} ${.TARGET}.1 ${.TARGET}
.y.c:
	${_MKTARGET_YACC}
	${YACC.y} -o ${.TARGET} ${.IMPSRC}
	echo '#if HAVE_NBTOOL_CONFIG_H' > ${.TARGET}.1
	echo '#include "nbtool_config.h"' >> ${.TARGET}.1
	echo '#endif' >> ${.TARGET}.1
	cat ${.TARGET} >> ${.TARGET}.1
	${MV} ${.TARGET}.1 ${.TARGET}
