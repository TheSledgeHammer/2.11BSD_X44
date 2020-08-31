#	$NetBSD: bsd.lib.mk,v 1.117.2.3 1998/11/07 00:22:23 cgd Exp $
#	@(#)bsd.lib.mk	8.3 (Berkeley) 4/22/94

.if exists(${.CURDIR}/../Makefile.inc)
.include "${.CURDIR}/../Makefile.inc"
.endif

.include <bsd.own.mk>

.if exists(${.CURDIR}/shlib_version)
.include "${.CURDIR}/shlib_version"
.if defined(LIB) && defined(LIB${LIB}_VERSION)
SHLIB_MAJOR=${LIB${LIB}_VERSION:R}
SHLIB_MINOR=${LIB${LIB}_VERSION:E}
.else
SHLIB_MAJOR=${major}
SHLIB_MINOR=${minor}
.endif
.endif

.MAIN:		all

# add additional suffixes not exported.
# .po is used for profiling object files.
# .so is used for PIC object files.
.SUFFIXES:
.SUFFIXES: .out .o .po .so .S .s .c .cc .C .f .y .l .ln .m4

.c.o:
		@echo "${COMPILE.c} ${.IMPSRC}"
		@${COMPILE.c} ${.IMPSRC}  -o ${.TARGET}.o
		@${LD} -x -r ${.TARGET}.o -o ${.TARGET}
		@rm -f ${.TARGET}.o

.c.po:
		@echo "${COMPILE.c} -p ${.IMPSRC} -o ${.TARGET}"
		@${COMPILE.c} -p ${.IMPSRC} -o ${.TARGET}.o
		@${LD} -X -r ${.TARGET}.o -o ${.TARGET}
		@rm -f ${.TARGET}.o

.c.so:
		@echo "${COMPILE.c} ${PICFLAG} -DPIC ${.IMPSRC} -o ${.TARGET}"
		@${COMPILE.c} ${PICFLAG} -DPIC ${.IMPSRC} -o ${.TARGET}.o
		@${LD} -x -r ${.TARGET}.o -o ${.TARGET}
		@rm -f ${.TARGET}.o

.c.ln:
		${LINT} ${LINTFLAGS} ${CFLAGS:M-[IDU]*} -i ${.IMPSRC}

.cc.o .C.o:
		@echo "${COMPILE.cc} ${.IMPSRC}"
		@${COMPILE.cc} ${.IMPSRC} -o ${.TARGET}.o
		@${LD} -x -r ${.TARGET}.o -o ${.TARGET}
		@rm -f ${.TARGET}.o

.cc.po .C.po:
		@echo "${COMPILE.cc} -p ${.IMPSRC} -o ${.TARGET}"
		@${COMPILE.cc} -p ${.IMPSRC} -o ${.TARGET}.o
		@${LD} -X -r ${.TARGET}.o -o ${.TARGET}
		@rm -f ${.TARGET}.o

.cc.so .C.so:
		@echo "${COMPILE.cc} ${PICFLAG} -DPIC ${.IMPSRC} -o ${.TARGET}"
		@${COMPILE.cc} ${PICFLAG} -DPIC ${.IMPSRC} -o ${.TARGET}.o
		@${LD} -x -r ${.TARGET}.o -o ${.TARGET}
		@rm -f ${.TARGET}.o

.S.o .s.o:
		@echo "${CPP} ${CPPFLAGS} ${CFLAGS:M-[ID]*} ${AINC} ${.IMPSRC} | \
			${AS} -o ${.TARGET}"
		@${CPP} ${CPPFLAGS} ${CFLAGS:M-[ID]*} ${AINC} ${.IMPSRC} | \
	    	${AS} -o ${.TARGET}.o
		@${LD} -x -r ${.TARGET}.o -o ${.TARGET}
		@rm -f ${.TARGET}.o

.S.po .s.po:
		@echo "${CPP} -DPROF ${CPPFLAGS} ${CFLAGS:M-[ID]*} ${AINC} ${.IMPSRC} |\
	    	${AS} -o ${.TARGET}"
		@${CPP} -DPROF ${CPPFLAGS} ${CFLAGS:M-[ID]*} ${AINC} ${.IMPSRC} | \
		    ${AS} -o ${.TARGET}.o
		@${LD} -X -r ${.TARGET}.o -o ${.TARGET}
		@rm -f ${.TARGET}.o

.S.so .s.so:
		@echo "${CPP} -DPIC ${CPPFLAGS} ${CFLAGS:M-[ID]*} ${AINC} ${.IMPSRC} | \
		    ${AS} -k -o ${.TARGET}"
		@${CPP} -DPIC ${CPPFLAGS} ${CFLAGS:M-[ID]*} ${AINC} ${.IMPSRC} | \
		    ${AS} -k -o ${.TARGET}.o
		@${LD} -x -r ${.TARGET}.o -o ${.TARGET}
		@rm -f ${.TARGET}.o

	
.if !defined(PICFLAG)
PICFLAG=-fpic
.endif

.if !defined(NOPROFILE)
_LIBS=lib${LIB}.a lib${LIB}_p.a
.else
_LIBS=lib${LIB}.a
.endif

.if !defined(NOPIC)
.if defined(SHLIB_MAJOR) && defined(SHLIB_MINOR)
_LIBS+=lib${LIB}.so.${SHLIB_MAJOR}.${SHLIB_MINOR}
.endif
.endif

.if !defined(NOLINT)
_LIBS+=llib-l${LIB}.ln
.endif

all: 	${_LIBS} _SUBDIRUSE

OBJS+=	${SRCS:N*.h:R:S/$/.o/g}

lib${LIB}.a:: ${OBJS}
		@echo building standard ${LIB} library
		@rm -f lib${LIB}.a
		@${AR} cq lib${LIB}.a `lorder ${OBJS} | tsort -q`
		${RANLIB} lib${LIB}.a

POBJS+=	${OBJS:.o=.po}
lib${LIB}_p.a:: ${POBJS}
		@echo building profiled ${LIB} library
		@rm -f lib${LIB}_p.a
		@${AR} cq lib${LIB}_p.a `lorder ${POBJS} | tsort -q`
		${RANLIB} lib${LIB}_p.a

SOBJS+=	${OBJS:.o=.so}
lib${LIB}_pic.a:: ${SOBJS}
		@echo building shared object ${LIB} library
		@rm -f lib${LIB}_pic.a
		@${AR} cq lib${LIB}_pic.a `lorder ${SOBJS} | tsort -q`
		${RANLIB} lib${LIB}_pic.a

lib${LIB}.so.${SHLIB_MAJOR}.${SHLIB_MINOR}: lib${LIB}_pic.a ${DPADD}
		@echo building shared ${LIB} library \(version ${SHLIB_MAJOR}.${SHLIB_MINOR}\)
		@rm -f lib${LIB}.so.${SHLIB_MAJOR}.${SHLIB_MINOR}
		$(LD) -x -Bshareable -Bforcearchive \
	    	-o lib${LIB}.so.${SHLIB_MAJOR}.${SHLIB_MINOR} lib${LIB}_pic.a ${LDADD}

LOBJS+=	${LSRCS:.c=.ln} ${SRCS:M*.c:.c=.ln}
# the following looks XXX to me... -- cgd
LLIBS?=	-lc
llib-l${LIB}.ln: ${LOBJS}
		@echo building llib-l${LIB}.ln
		@rm -f llib-l${LIB}.ln
		@${LINT} -C${LIB} ${LOBJS} ${LLIBS}

.if !target(clean)
clean: _SUBDIRUSE
		rm -f a.out [Ee]rrs mklog core *.core ${CLEANFILES}
		rm -f lib${LIB}.a ${OBJS}
		rm -f lib${LIB}_p.a ${POBJS}
		rm -f lib${LIB}_pic.a lib${LIB}.so.*.* ${SOBJS}
		rm -f llib-l${LIB}.ln ${LOBJS}
.endif

cleandir: _SUBDIRUSE clean

.if defined(SRCS)
afterdepend: .depend
	@(TMP=/tmp/_depend$$$$; \
	    sed -e 's/^\([^\.]*\).o[ ]*:/\1.o \1.po \1.so:/' \
	      < .depend > $$TMP; \
	    mv $$TMP .depend)
.endif

.if !target(install)
.if !target(beforeinstall)
beforeinstall:
.endif

realinstall:
#	ranlib lib${LIB}.a
	install ${COPY} -o ${LIBOWN} -g ${LIBGRP} -m 600 lib${LIB}.a \
	    ${DESTDIR}${LIBDIR}
	${RANLIB} -t ${DESTDIR}${LIBDIR}/lib${LIB}.a
	chmod ${LIBMODE} ${DESTDIR}${LIBDIR}/lib${LIB}.a
.if !defined(NOPROFILE)
#	ranlib lib${LIB}_p.a
	install ${COPY} -o ${LIBOWN} -g ${LIBGRP} -m 600 \
	    lib${LIB}_p.a ${DESTDIR}${LIBDIR}
	${RANLIB} -t ${DESTDIR}${LIBDIR}/lib${LIB}_p.a
	chmod ${LIBMODE} ${DESTDIR}${LIBDIR}/lib${LIB}_p.a
.endif
.if !defined(NOPIC)
#	ranlib lib${LIB}_pic.a
	install ${COPY} -o ${LIBOWN} -g ${LIBGRP} -m 600 \
	    lib${LIB}_pic.a ${DESTDIR}${LIBDIR}
	${RANLIB} -t ${DESTDIR}${LIBDIR}/lib${LIB}_pic.a
	chmod ${LIBMODE} ${DESTDIR}${LIBDIR}/lib${LIB}_pic.a
.endif
.if !defined(NOPIC) && defined(SHLIB_MAJOR) && defined(SHLIB_MINOR)
	install ${COPY} -o ${LIBOWN} -g ${LIBGRP} -m ${LIBMODE} \
	    lib${LIB}.so.${SHLIB_MAJOR}.${SHLIB_MINOR} ${DESTDIR}${LIBDIR}
.endif
.if !defined(NOLINT)
	install ${COPY} -o ${LIBOWN} -g ${LIBGRP} -m ${LIBMODE} \
	    llib-l${LIB}.ln ${DESTDIR}${LINTLIBDIR}
.endif
.if defined(LINKS) && !empty(LINKS)
	@set ${LINKS}; \
	while test $$# -ge 2; do \
		l=${DESTDIR}$$1; \
		shift; \
		t=${DESTDIR}$$1; \
		shift; \
		echo $$t -\> $$l; \
		rm -f $$t; \
		ln $$l $$t; \
	done; true
.endif

install: maninstall _SUBDIRUSE
maninstall: afterinstall
afterinstall: realinstall
realinstall: beforeinstall
.endif

.if !defined(NOMAN)
.include <bsd.man.mk>
.endif

.if !defined(NONLS)
.include <bsd.nls.mk>
.endif

.include <bsd.obj.mk>
.include <bsd.dep.mk>
.include <bsd.subdir.mk>
.include <bsd.sys.mk>
