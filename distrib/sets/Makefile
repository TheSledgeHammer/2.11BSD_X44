#	$NetBSD: Makefile,v 1.2.2.2 2000/06/20 06:00:28 thorpej Exp $

# The `all' target must appear before bsd.own.mk is pulled in.
all:
	@echo "Please understand what you are doing, first."
	@false

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

SETSENV=    DESTDIR=${DESTDIR:Q} \
            MACHINE=${MACHINE:Q} \
            MACHINE_ARCH=${MACHINE_ARCH:Q} \
		    AWK=${TOOL_AWK:Q} \
    		CKSUM=${TOOL_CKSUM:Q} \
		    HOST_SH=${HOST_SH:Q} \
    		MAKE=${MAKE:Q} \
    		MKTEMP=${TOOL_MKTEMP:Q} \
    		MTREE=${TOOL_MTREE:Q} \
            PAX=${TOOL_PAX:Q} \

SETSCMD=	cd ${.CURDIR} && \
    		${SETSENV} \
	    	${HOST_SH}

MAKEDLIST_FLAGS=
MAKEFLIST_FLAGS= -setd base
MAKESRCTARS_FLAGS=

.if !defined(MAKETARSETS)
MAKETARSETS!= ${SETSCMD} ./makeflist ${MAKEFLIST_FLAGS}
.endif

print_have_gcc: .PHONY
	@echo ${HAVE_GCC}

print_machine: .PHONY
	@echo ${MACHINE}

print_machine_arch: .PHONY
	@echo ${MACHINE_ARCH}

print_machine_cpu: .PHONY
	@echo ${MACHINE_CPU}

print_object_fmt: .PHONY
	@echo ${OBJECT_FMT}

print_toolchain_missing: .PHONY
	@echo "${TOOLCHAIN_MISSING}"

#
# METALOG MANIPULATION TARGETS
#
# METALOG is the name of a metadata log file, and is set in <bsd.own.mk>
# if MKUNPRIVED is not "no".
#
# METALOG.unpriv is a command line option passed to various scripts;
# it is either blank or "-M ${METALOG}.sanitised", depending on the
# MKUNPRIVED flag.
#
# The sanitise_METALOG target creates METALOG.sanitised from METALOG,
# without modifying METALOG itself.  This is used near the end of
# a build, after build products have been installed in DESTDIR
# and corresponding lines have been added to METALOG, but before
# METALOG.sanitised is used in the creation of sets in RELEASEDIR.
# For update builds, METALOG.sanitised has duplicates merged (keeping
# only the last of multiple entries for the same item), but for clean
# builds it's an error if a file or directory name appears twice.
# METALOG.sanitised is always sorted and has timestamps removed.
#
# The clean_METALOG target either deletes METALOG, or replaces METALOG with
# a sanitised version of itself, depending on the MKUPDATE flag, and
# deletes old METALOG.* files.  This is intended to be used at the start
# of a build, to ensure that repeated MKUPDATE builds do not cause
# unbounded growth of METALOG.
#

METALOG_REMOVE_DUPLICATES= \
	${TOOL_AWK} '{ a[$$1] = $$0; } END { for (f in a) print a[f]; }'

.if ${MKUNPRIVED} == "no"
METALOG.unpriv=
sanitise_METALOG: .PHONY
	@true
clean_METALOG: .PHONY
	@true
.else	# MKUNPRIVED
METALOG.unpriv= -M ${METALOG}.sanitised
sanitise_METALOG: .PHONY ${METALOG}.sanitised
${METALOG}.sanitised: ${METALOG}
	${_MKMSG_CREATE} METALOG.sanitised
	<${METALOG} \
	    ${${MKUPDATE} != "no" :? ${METALOG_REMOVE_DUPLICATES} | :} \
	    sort | ${TOOL_MTREE} -CSM -k all -R time -N ${NETBSDSRCDIR}/etc \
	    >${METALOG}.new
	mv ${METALOG}.new ${METALOG}.sanitised
.if defined(RCSMETALOG)
	. ./metalog.subr; \
	xrcs_descr="build distribution METALOG"; \
	xrcs_msg="$$(date)"; \
	xrcs_cur=${METALOG}.sanitised; \
	xrcs update
.endif
.if ${MKUPDATE} == "no" || !exists(${METALOG})
clean_METALOG: .PHONY
	rm -f ${METALOG} ${METALOG}.*
.else	# MKUPDATE
clean_METALOG: .PHONY ${METALOG}.sanitised
	mv ${METALOG}.sanitised ${METALOG}
.if defined(RCSMETALOG)
	[ -f ${METALOG}.sanitised,v ] && mv ${METALOG}.sanitised,v ${METALOG},v
.endif
	rm -f ${METALOG}.*
.if defined(RCSMETALOG)
	[ -f ${METALOG},v ] && mv ${METALOG},v ${METALOG}.sanitised,v
.endif
.endif	# MKUPDATE
.endif	# MKUNPRIVED

#
# FILE LIST TARGETS
#

makedlist: .PHONY .PRECIOUS check_DESTDIR
	${_MKMSG_EXECUTE} "makedlist"
	${SETSCMD} ./makedlist ${MAKEDLIST_FLAGS}

# This target has debugging value only, really.
makeflist: .PHONY .PRECIOUS check_DESTDIR
	${_MKMSG_EXECUTE} "makeflist"
	${SETSCMD} ./makeflist ${MAKEFLIST_FLAGS}

checkflist: .PRECIOUS .PHONY check_DESTDIR sanitise_METALOG
	${_MKMSG_EXECUTE} "checkflist"
	${SETSCMD} ${.CURDIR}/checkflist \
	    ${MAKEFLIST_FLAGS} ${CHECKFLIST_FLAGS} ${METALOG.unpriv}

#
# SET BUILDING TARGETS
#

TARDIR=		    ${RELEASEDIR}/${RELEASEMACHINEDIR}/binary/sets
SOURCETARDIR=	${RELEASEDIR}/source/sets

maketars: .PRECIOUS .PHONY check_DESTDIR check_RELEASEDIR .WAIT \
		maketarsetup .WAIT ${MAKETARSETS:@.TARS.@do-${.TARS.}@}
	@true

maketarsetup: .EXEC
.if defined(DESTDIR) && ${DESTDIR} != ""
	${_MKMSG} "execute  checkflist"
	${MAKE} checkflist
.endif
	mkdir -p ${TARDIR}
	for i in BSDSUM CKSUM MD5 SYSVSUM; do \
		rm -f ${TARDIR}/$$i ${TARDIR}/$$i.tmp; \
	done

makesetfiles: .PHONY
	${_MKMSG_CREATE} "set lists"
	${SETSCMD} ./maketars -dest ${DESTDIR:S,^$,/,} ${METALOG.unpriv} \
	    ${MAKEFLIST_FLAGS} -tars ${TARDIR}

.for tar in ${MAKETARSETS}
do-${tar}: .PHONY
	${SETSCMD} ./maketars -dest ${DESTDIR:S,^$,/,} ${METALOG.unpriv} \
	    -tars ${TARDIR} ${tar} \
	|| rm -f ${TARDIR}/${tar}.tgz
.endfor

makesums: .PRECIOUS .PHONY check_RELEASEDIR .WAIT \
		${MAKETARSETS:@.TARS.@do-sum-${.TARS.}@}
	for i in BSDSUM CKSUM MD5 SYSVSUM; do \
		mv ${TARDIR}/$$i.tmp ${TARDIR}/$$i; \
	done

.for tar in ${MAKETARSETS}
do-sum-${tar}: .PHONY do-${tar}
	${MAKESUMS} -t ${TARDIR} ${tar}.tgz
	for i in BSDSUM CKSUM MD5 SYSVSUM; do \
		${TOOL_CAT} ${TARDIR}/$$i >> ${TARDIR}/$$i.tmp; \
	done
.endfor
.ORDER: ${MAKETARSETS:@.TARS.@do-sum-${.TARS.}@}

#
# MAIN ENTRY POINTS
#

sets: .PHONY maketars .WAIT makesums
	@true

.include <bsd.files.mk>
