#	$NetBSD: Makefile.inc,v 1.24 2018/05/27 01:14:51 christos Exp $
#	S	must be set to the top of the 'sys' tree.

# Default values:
SADIR=	$S/lib/libsa
SA_AS?=		library
SADOTDIR?= ../../.

SADIR=		${S:S@^.@${SADOTDIR}@:q}/lib/libsa
.if (${SA_AS} == "obj")
SALIB=		${SADST}/libsa.o
SALIB_PROF=	${SADST}/libsa.po
.else
SALIB=		${SADST}/libsa.a
SALIB_PROF=	${SADST}/libsa_p.a
.endif

CWARNFLAGS.clang+=	-Wno-format-extra-args

SAMAKE= \
		cd ${SADST} && ${MAKE} -f ${SADIR:q}/Makefile \
	    	SADIR=${SADIR:q} \
			CC=${CC:q} CFLAGS=${CFLAGS:q} CPUFLAGS= \
			AS=${AS:q} AFLAGS=${AFLAGS:q} \
			LORDER=${LORDER:q} \
			TSORT=${TSORT:q} \
			LD=${LD:q} STRIP=${STRIP:q} \
			AR=${AR:q} NM=${NM:q} \
			RANLIB=${RANLIB:q} SIZE=${SIZE:q} \
			MACHINE=${MACHINE} MACHINE_ARCH=${MACHINE_ARCH:q} \

${SALIB}:				.NOTMAIN .MAKE __always_make_salib
		@echo making sure the sa library is up to date...
.if (${SA_AS} == "library")
		@${SAMAKE} libsa.a
.else
		@${SAMAKE} libsa.o
.endif

${SALIB_PROF}:			.NOTMAIN .MAKE __always_make_salib
		@echo making sure the profiled sa library is up to date...
.if (${SA_AS} == "library")
	@${SAMAKE} libsa_p.a
.else
	@${SAMAKE} libsa.po
.endif

clean:					.NOTMAIN cleansalib
cleansalib:				.NOTMAIN
		@echo cleaning the sa library objects
		@if [ -d "${SADST}" ]; then ${SAMAKE} clean; fi

cleandir distclean:		.NOTMAIN cleandirsalib
cleandirsalib:			.NOTMAIN
		@echo cleandiring the sa library objects
		@if [ -d "${SADST}" ]; then ${SAMAKE} cleandir; fi

dependall depend:		.NOTMAIN dependsalib
dependsalib:			.NOTMAIN .MAKE __always_make_salib
		@echo depending the sa library objects
		@${SAMAKE} depend

__always_make_salib:	.NOTMAIN
	@mkdir -p ${SADST}

.PHONY: __always_make_salib
.PHONY: cleansalib cleandirsalib dependsalib
