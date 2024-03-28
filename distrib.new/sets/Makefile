#	$NetBSD: Makefile,v 1.107 2019/03/10 11:04:08 mrg Exp $

# Experimental RCS METALOG versioning
# (Needs host's rcs(1) commands)
#
# TODO:
# - In 'build.sh distribution', print diff to previous
#RCSMETALOG=1

# The `all' target must appear before bsd.own.mk is pulled in.
all:
	@echo "Please understand what you are doing, first."
	@false

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

COMPRESS_PROGRAM=${"${USE_XZ_SETS:Uno}"!="no":?${TOOL_XZ}:${TOOL_GZIP}}
TAR_SUFF=${"${USE_XZ_SETS:Uno}"!="no":?tar.xz:tgz}
XZ_OPT=		-9
GZIP_FLAGS=	${GZIP_N_FLAG}

SETSENV=    DESTDIR=${DESTDIR:Q} \
            MACHINE=${MACHINE:Q} \
            MACHINE_ARCH=${MACHINE_ARCH:Q} \
		    AWK=${TOOL_AWK:Q} \
    		CKSUM=${TOOL_CKSUM:Q} \
	    	DB=${TOOL_DB:Q} \
	    	EGREP=${TOOL_GREP:Q}\ -E \
		    HOST_SH=${HOST_SH:Q} \
    		MAKE=${MAKE:Q} \
    		MKTEMP=${TOOL_MKTEMP:Q} \
    		MTREE=${TOOL_MTREE:Q} \
    		PAX=${TOOL_PAX:Q} \
    		COMPRESS_PROGRAM=${COMPRESS_PROGRAM:Q} \
    		GZIP=${GZIP_FLAGS:Q} \
    		XZ_OPT=${XZ_OPT:Q} \
    		TAR_SUFF=${TAR_SUFF:Q} \
    		PKG_CREATE=${TOOL_PKG_CREATE:Q} \
		    SED=${TOOL_SED:Q} \
		    TSORT=${TSORT:Q}
SETSCMD=	cd ${.CURDIR} && \
    		${SETSENV} \
	    	${HOST_SH}

MAKETARS_FLAGS=
.if ${MKREPRO_TIMESTAMP:Uno} != "no"
MAKETARS_FLAGS+= -T "${MKREPRO_TIMESTAMP}"
.endif
.if ${MKX11} != "no"
MAKESRCTARS_X11_FLAGS=	-x ${X11SRCDIR}
.endif
.if ${MKEXTSRC} != "no"
MAKESRCTARS_EXTSRC_FLAGS=	-y ${EXTSRCSRCDIR}
.endif
.if ${MKX11} != "no"
MAKEFLIST_X11_FLAGS=	,x
.endif
.if ${MKEXTSRC} != "no"
MAKEFLIST_EXTSRC_FLAGS=	,ext
.endif
MAKEFLIST_FLAGS=	-L base${MAKEFLIST_X11_FLAGS}${MAKEFLIST_EXTSRC_FLAGS}
MAKESRCTARS_FLAGS= 	-N ${NETBSDSRCDIR}/etc

.if ${MAKEVERBOSE} < 2
MAKETARS_FLAGS+=	-q
MAKESRCTARS_FLAGS+= 	-q
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

MAKETARS_FLAGS=
.if ${MKREPRO_TIMESTAMP:Uno} != "no"
MAKETARS_FLAGS+= -T "${MKREPRO_TIMESTAMP}"
.endif
MAKEFLIST_FLAGS=

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

makeflist: .PHONY .PRECIOUS check_DESTDIR
	${SETSCMD} ./makeflist ${MAKEFLIST_FLAGS}

checkflist: .PRECIOUS .PHONY check_DESTDIR sanitise_METALOG
	${SETSCMD} ${.CURDIR}/checkflist \
	    ${MAKEFLIST_FLAGS} ${CHECKFLIST_FLAGS} ${METALOG.unpriv}

.if defined(DESTDIR) && ${DESTDIR} != ""
checkflist_if_DESTDIR: checkflist
.else
checkflist_if_DESTDIR:
.endif

#
# SET BUILDING TARGETS
#

TARDIR=		${RELEASEDIR}/${RELEASEMACHINEDIR}/binary/sets

# If MAKETARS_SERIAL is set to "YES" or "yes", then the tar sets will be
# made in series, even if make's "-j" flag requests multiple parallel
# jobs.  This is useful on systems that exhibit poor performance when
# running multiple parallel disk-intensive operations.  The default is
# MAKETARS_SERIAL=NO, which will obey the "-j" flag passed to the make
# command.
#

MAKETARS_SERIAL?= NO
.if empty(MAKETARS_SERIAL:M[Yy][Ee][Ss])
_MAKETARS_WAIT= # empty
.else
_MAKETARS_WAIT= .WAIT
.endif

maketars: .PRECIOUS .PHONY check_DESTDIR check_RELEASEDIR \
		sanitise_METALOG checkflist_if_DESTDIR .WAIT \
		maketarsetup .WAIT \
		${MAKETARSETS:@.TARS.@${_MAKETARS_WAIT} do-${.TARS.}@}
	@true

maketarsetup: .EXEC
.if defined(DESTDIR) && ${DESTDIR} != ""
	${_MKMSG} "execute  checkflist"
	cd ${.CURDIR}; ${MAKE} checkflist
.endif
	mkdir -p ${TARDIR}
	for i in MD5 SHA512; do \
		rm -f ${TARDIR}/$$i ${TARDIR}/$$i.tmp; \
	done

makesetfiles: .PHONY sanitise_METALOG
	${_MKMSG_CREATE} "set lists"
	${SETSCMD} ./maketars -S -d ${DESTDIR:S,^$,/,} ${MAKETARS_FLAGS} \
	    ${METALOG.unpriv} \
	    -N ${NETBSDSRCDIR}/etc ${MAKEFLIST_FLAGS} -t ${TARDIR}

.for tar in ${MAKETARSETS}
do-${tar}: .PHONY sanitise_METALOG
	${_MKMSG_CREATE} "${tar}.${TAR_SUFF}"
	${SETSCMD} ./maketars -d ${DESTDIR:S,^$,/,} ${MAKETARS_FLAGS} \
	    ${METALOG.unpriv} \
	    -N ${NETBSDSRCDIR}/etc -t ${TARDIR} ${tar} \
	|| { rm -f ${TARDIR}/${tar}.${TAR_SUFF}; false; }
.endfor

makesums: .PRECIOUS .PHONY check_RELEASEDIR .WAIT \
		${MAKETARSETS:@.TARS.@do-sum-${.TARS.}@}
	for i in MD5 SHA512; do \
		mv ${TARDIR}/$$i.tmp ${TARDIR}/$$i; \
	done

.for tar in ${MAKETARSETS}
do-sum-${tar}: .PHONY do-${tar}
	${_MKMSG_CREATE} "${tar} checksums"
	${MAKESUMS} -t ${TARDIR} ${tar}.${TAR_SUFF}
	for i in MD5 SHA512; do \
		${TOOL_CAT} ${TARDIR}/$$i >> ${TARDIR}/$$i.tmp; \
	done
.endfor
.ORDER: ${MAKETARSETS:@.TARS.@do-sum-${.TARS.}@}

installsets: .PHONY check_DESTDIR sanitise_METALOG
.if !defined(INSTALLDIR)
	@echo "setenv INSTALLDIR before doing that!"
	@false
.endif
	${SETSCMD} ./maketars -d ${DESTDIR:S,^$,/,} ${MAKETARS_FLAGS} \
	    ${METALOG.unpriv} -N ${NETBSDSRCDIR}/etc \
	    ${MAKEFLIST_FLAGS} -i ${INSTALLDIR} ${INSTALLSETS}

#
# MAIN ENTRY POINTS
#

sets: .PHONY maketars .WAIT makesums
	@true

.include <bsd.files.mk>
