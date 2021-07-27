#	@(#)Makefile	8.1 (Berkeley) 6/4/93

.include <bsd.own.mk>

USE_SHLIBDIR=	yes

LIB=			termcap
WARNS?=			5

CPPFLAGS+=		-I${.CURDIR}
SRCS=			termcap.c tgoto.c tputs.c
INCSDIR=		/usr/include

COPTS.tparm.c = -Wno-format-nonliteral

MAN=	termcap.3
MLINKS=	termcap.3 tgetent.3 termcap.3 tgetflag.3 termcap.3 tgetnum.3 \
		termcap.3 tgetstr.3 termcap.3 tgoto.3 termcap.3 tputs.3

CPPFLAGS+=		-I${.OBJDIR}

.include <bsd.shlib.mk>
		
.if ${MKLINKLIB} != "no"
SYMLINKS+=	libtermcap.a ${LIBDIR}/libtermcap.a
SYMLINKS+=	libtermcap.a ${LIBDIR}/libtermlib.a
.endif

.if ${MKPROFILE} != "no"
SYMLINKS+=	libtermcap_p.a ${LIBDIR}/libtermcap_p.a
SYMLINKS+=	libtermcap_p.a ${LIBDIR}/libtermlib_p.a
.endif

.if ${MKPIC} != "no"

.if ${MKPICINSTALL} != "no"
SYMLINKS+=	libtermcap_pic.a ${LIBDIR}/libtermcap_pic.a
SYMLINKS+=	libtermcap_pic.a ${LIBDIR}/libtermlib_pic.a
.endif

if exists(${.CURDIR}/shlib_version)
SYMLINKS+=	libtermcap.so.${SHLIB_FULLVERSION} \
		${_LIBSODIR}/libtermcap.so.0.6
SYMLINKS+=	libtermcap.so.${SHLIB_FULLVERSION} \
		${_LIBSODIR}/libtermlib.so.0.6
.if ${_LIBSODIR} != ${LIBDIR}
SYMLINKS+=	libtermcap.so.${SHLIB_FULLVERSION} \
		${LIBDIR}/libtermcap.so.0.6
SYMLINKS+=	libtermcap.so.${SHLIB_FULLVERSION} \
		${LIBDIR}/libtermlib.so.0.6
.endif

SYMLINKS+=	libtermcap.so.${SHLIB_MAJOR} \
		${_LIBSODIR}/libtermcap.so.0
SYMLINKS+=	libtermcap.so ${_LIBSODIR}/libtermcap.so
SYMLINKS+=	libtermcap.so.${SHLIB_MAJOR} \
		${_LIBSODIR}/libtermlib.so.0
SYMLINKS+=	libtermcap.so ${_LIBSODIR}/libtermlib.so
.if ${_LIBSODIR} != ${LIBDIR}
SYMLINKS+=	libtermcap.so.${SHLIB_MAJOR} \
		${LIBDIR}/libtermcap.so.0
SYMLINKS+=	libtermcap.so ${LIBDIR}/libtermcap.so
SYMLINKS+=	libtermcap.so.${SHLIB_MAJOR} \
		${LIBDIR}/libtermlib.so.0
SYMLINKS+=	libtermcap.so ${LIBDIR}/libtermlib.so
.endif
.endif	# exists shlib_version

.endif	# ${MKPIC} != "no"

.include <bsd.lib.mk>
