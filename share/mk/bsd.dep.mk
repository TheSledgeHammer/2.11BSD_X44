#	$NetBSD: bsd.dep.mk,v 1.87 2020/07/01 07:38:29 lukem Exp $

##### Basic targets
realdepend:	beforedepend .depend afterdepend
.ORDER:		beforedepend .depend afterdepend

beforedepend .depend afterdepend: # ensure existence

##### Default values
MKDEP?=			mkdep
MKDEPCXX?=		mkdep
MKDEP_SUFFIXES?=	.o .d

##### Build rules
# some of the rules involve .h sources, so remove them from mkdep line

.if defined(SRCS) && !empty(SRCS)
__acpp_flags=	${_ASM_TRADITIONAL_CPP}

.if defined(NODPSRCS)
.for f in ${SRCS} ${DPSRCS}
.if "${NODPSRCS:M${f}}" == ""
__DPSRCS.all+=	${f:C/\.(c|m|s|S|C|cc|cpp|cxx)$/.d/}
.endif
.endfor
beforedepend: ${DPSRCS}
.else
__DPSRCS.all+=	${SRCS:C/\.(c|m|s|S|C|cc|cpp|cxx)$/.d/} \
		${DPSRCS:C/\.(c|m|s|S|C|cc|cpp|cxx)$/.d/}
.endif
__DPSRCS.d=	${__DPSRCS.all:O:u:M*.d}
__DPSRCS.notd=	${__DPSRCS.all:O:u:N*.d}

.NOPATH: .depend ${__DPSRCS.d}

.if !empty(__DPSRCS.d)							# {
${__DPSRCS.d}: ${__DPSRCS.notd} ${DPSRCS}
.endif									# }

MKDEPSUFFLAGS=-s ${MKDEP_SUFFIXES:Q}

.if defined(MKDEPINCLUDES) && ${MKDEPINCLUDES} != "no"
.STALE:
	@echo Rebuilding dependency file: ${.ALLSRC}
	@rm -f ${.ALLSRC}
	@(cd ${.CURDIR} && ${MAKE} depend)
_MKDEP_MERGEFLAGS=-i
_MKDEP_FILEFLAGS=${MKDEPSUFFLAGS}
.else
_MKDEP_MERGEFLAGS=${MKDEPSUFFLAGS}
_MKDEP_FILEFLAGS=
.endif

.depend: ${__DPSRCS.d}
	${_MKTARGET_CREATE}
	rm -f .depend
	${MKDEP} ${_MKDEP_MERGEFLAGS} -d -f ${.TARGET} ${__DPSRCS.d}

.SUFFIXES: .d .s .S .c .C .cc .cpp .cxx .m

.c.d:
	${_MKTARGET_CREATE}
	${MKDEP} -f ${.TARGET}.tmp ${_MKDEP_FILEFLAGS} -- ${MKDEPFLAGS} \
	    ${CFLAGS:M-std=*} ${CFLAGS:C/-([IDU])[  ]*/-\1/Wg:M-[IDU]*} \
	    ${CPPFLAGS:N-Wp,-iremap,*} ${COPTS.${.IMPSRC:T}} ${CPUFLAGS.${.IMPSRC:T}} \
	    ${CPPFLAGS.${.IMPSRC:T}} ${.IMPSRC} && \
	    ${MV} ${.TARGET}.tmp ${.TARGET}

.m.d:
	${_MKTARGET_CREATE}
	${MKDEP} -f ${.TARGET}.tmp ${_MKDEP_FILEFLAGS} -- ${MKDEPFLAGS} \
	    ${OBJCFLAGS:C/-([IDU])[  ]*/-\1/Wg:M-[IDU]*} \
	    ${CPPFLAGS:N-Wp,-iremap,*} ${COPTS.${.IMPSRC:T}} ${CPUFLAGS.${.IMPSRC:T}} \
	    ${CPPFLAGS.${.IMPSRC:T}} ${.IMPSRC} && \
	    ${MV} ${.TARGET}.tmp ${.TARGET}

.s.d .S.d:
	${_MKTARGET_CREATE}
	${MKDEP} -f ${.TARGET}.tmp ${_MKDEP_FILEFLAGS} -- ${MKDEPFLAGS} \
	    ${AFLAGS:C/-([IDU])[  ]*/-\1/Wg:M-[IDU]*} \
	    ${CPPFLAGS:N-Wp,-iremap,*} ${AFLAGS.${.IMPSRC:T}} ${CPPFLAGS.${.IMPSRC:T}} \
	    ${__acpp_flags} ${.IMPSRC} && \
	    ${MV} ${.TARGET}.tmp ${.TARGET}

.C.d .cc.d .cpp.d .cxx.d:
	${_MKTARGET_CREATE}
	${MKDEPCXX} -f ${.TARGET}.tmp ${_MKDEP_FILEFLAGS} -- ${MKDEPFLAGS} \
	    ${CXXFLAGS:M-std=*} ${CXXFLAGS:C/-([IDU])[  ]*/-\1/Wg:M-[IDU]*} \
	    ${CPPFLAGS:N-Wp,-iremap,*} ${COPTS.${.IMPSRC:T}} ${CPUFLAGS.${.IMPSRC:T}} \
	    ${CPPFLAGS.${.IMPSRC:T}} ${.IMPSRC} && \
	    ${MV} ${.TARGET}.tmp ${.TARGET}

.endif # defined(SRCS) && !empty(SRCS)					# }

##### Clean rules
.if defined(SRCS) && !empty(SRCS)
CLEANDIRFILES+= .depend ${__DPSRCS.d} ${__DPSRCS.d:.d=.d.tmp} tags ${CLEANDEPEND}
.endif

##### Custom rules
.if !commands(tags)
tags: ${SRCS}
.if defined(SRCS) && !empty(SRCS)
	${_MKTARGET_CREATE}
	-cd "${.CURDIR}"; ctags -f /dev/stdout ${.ALLSRC:M*.[cly]} | \
	    ${TOOL_SED} "s;\${.CURDIR}/;;" > ${.OBJDIR}/tags
.endif
.endif

##### Pull in related .mk logic
.include <bsd.clean.mk>