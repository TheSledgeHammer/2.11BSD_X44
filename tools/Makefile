#	$NetBSD: Makefile,v 1.209 2021/05/06 13:23:36 rin Exp $

.include <bsd.own.mk>
.include <bsd.endian.mk>

# Make sure that the ordered build/install processing applies when using
# plain make.
.MAIN: build_install

# TOOLDIR must be valid, unless MKTOOLS=no
.if ${MKTOOLS:Uyes} != "no"
.if "${TOOLDIR}" == ""
.error "TOOLDIR is undefined or empty"
.elif "${TOOLDIR:tW:M/*}" == ""
.error "TOOLDIR is not an absolute path: ${TOOLDIR}"
#.elif !exists(TOOLDIR) # XXX .exists fails for directories
#.error "TOOLDIR does not exist: ${TOOLDIR}"
.endif
.endif # MKTOOLS != no

# TOOLS_BUILDRUMP == yes builds only the subset of the tools required
# for building rump kernels and the hypervisor.  It is typically used
# when building rump kernels targeted for non-NetBSD systems (via
# buildrump.sh), and should not be set for a regular "make build".
#TOOLS_BUILDRUMP?=no

.if ${TOOLCHAIN_MISSING} == "no"
. if (defined(HAVE_GCC) && ${HAVE_GCC} > 0) || \
     (defined(HAVE_GDB) && ${HAVE_GDB} > 0 && ${MKCROSSGDB:Uno} != "no")
TOOLCHAIN_BITS+= gmake .WAIT
TOOLCHAIN_BITS+= gmp .WAIT
TOOLCHAIN_BITS+= mpfr .WAIT
TOOLCHAIN_BITS+= mpc .WAIT
. endif

TOOLCHAIN_BITS+= binutils .WAIT

. if defined(HAVE_GCC) && ${HAVE_GCC} > 0
TOOLCHAIN_BITS+= gcc
. endif

. if defined(HAVE_GDB) && ${HAVE_GDB} > 0 && ${MKCROSSGDB:Uno} != "no"
TOOLCHAIN_BITS+= gdb
. endif

TOOLCHAIN_BITS+= .WAIT
.endif

.if defined(HAVE_PCC)
. if ${TOOLCHAIN_MISSING} == "no"
TOOLCHAIN_BITS+= pcc
. endif
.endif

XZ_BITS1=
XZ_BITS2=
.if ${MKXZ} != "no"
XZ_BITS1+= xz-include .WAIT
XZ_BITS1+= xz-lib
XZ_BITS2+= xz-bin
.endif

.if ${TOOLCHAIN_MISSING} == "no" || defined(EXTERNAL_TOOLCHAIN)
TOOLCHAIN_BITS+= dbsym mdsetimage
.endif

ELFTOOLCHAIN_BITS=
ELFTOOLCHAIN_BITS+= libelf .WAIT
ELFTOOLCHAIN_BITS+= libdwarf .WAIT

LINT_BITS=
.if ${MKLINT} != "no"
LINT_BITS= lint lint2
.endif

# All of host-mkdep, compat, and binstall are needed before anything
# else.  Within this group, they must be built in a specific order, and
# all of them must be built before any of them is installed.  They may
# be installed in any order.  This can't be expressed using the .WAIT
# notation inside the SUBDIR list.
#
# XXX .ORDER does not work when multiple targets are passed on the
# make command line without "-j", so use dependencies in addition to .ORDER.
#
.ORDER: dependall-host-mkdep dependall-compat dependall-binstall dependall-date
.if make(dependall-host-mkdep) && make(dependall-compat)
dependall-compat: dependall-host-mkdep
.endif
.if make(dependall-compat) && make(dependall-binstall)
dependall-binstall: dependall-compat
.endif
.if make(dependall-date)
dependall-date: dependall-host-mkdep dependall-compat
.endif
# Dependencies in SUBDIR below ordered to maximize parallel ability.
# See above for special treatment for host-mkdep, compat, and binstall.
#
SUBDIR=	host-mkdep compat binstall date .WAIT mktemp .WAIT sed .WAIT genassym 		\
		cap_mkdb crunchgen ctags gencat hexdump 								\
		${LINT_BITS} 															\
		makewhatis mtree nbperf .WAIT uudecode 										\
		cat rpcgen join lorder m4 mkdep tsort .WAIT yacc .WAIT awk .WAIT lex 			\
		${XZ_BITS1} .WAIT grep pax gethost									\
		.WAIT texinfo 															\
		.WAIT tic																	\
		.WAIT ${TOOLCHAIN_BITS} 												\
        	${ELFTOOLCHAIN_BITS}                                                    \
		cksum db																\
		file lint1																\
		makefs sortinfo 																\
		.WAIT menuc mklocale mknod msgc 										\
		.WAIT disklabel																	\
		.WAIT 																	\
		.WAIT fdisk																\
		.WAIT 																	\
		pwd_mkdb strfile vgrind zic												
														
SUBDIR+= stat .WAIT config ${XZ_BITS2}

.if ${MKLLVM} != "no" || ${MKLLVMRT} != "no"
SUBDIR+= 						\
	llvm .WAIT 					\
	llvm-lib/libLLVMDemangle llvm-lib/libLLVMSupport llvm-lib/libLLVMTableGen .WAIT \
	llvm-tblgen
.endif
.if ${MKLLVM} != "no"
SUBDIR+= 						\
	llvm-clang-tblgen
.endif
.if ${MKLLVM} != "no" && !defined(EXTERNAL_TOOLCHAIN)
SUBDIR+= 						\
	.WAIT llvm-include .WAIT 	\
	llvm-lib .WAIT 				\
	llvm-clang
.endif

.if ${MKMAN} != "no" || ${MKDOC} != "no" || ${MKHTML} != "no"
. if ${MKGROFF} != "no"
SUBDIR+=	groff
. endif
SUBDIR+=	mandoc
.endif

.if ${MKMAINTAINERTOOLS:Uno} != "no"
SUBDIR+=	autoconf .WAIT gettext
.endif

.if ${USE_PIGZGZIP} != "no"
SUBDIR+=	pigz
.endif

#.if ${MKX11} != "no"
#SUBDIR+=	makestrs
#SUBDIR+=	makekeys
#.endif

check_MKTOOLS: .PHONY .NOTMAIN
.if ${MKTOOLS:Uyes} == "no"
	@echo '*** WARNING: "MKTOOLS" is set to "no"; this will prevent building and'
	@echo '*** updating your host toolchain.  This should be used only as a'
	@echo '*** temporary workaround for toolchain problems, as it will result'
	@echo '*** in version skew and build errors over time!'
.endif

.if ${MKTOOLS:Uyes} == "no" || ${USETOOLS} != "yes"	# {
SUBDIR= # empty
realall realdepend install: check_MKTOOLS
.endif							# }

.include <bsd.subdir.mk>
.include <bsd.buildinstall.mk>
.include <bsd.obj.mk>

.if !defined(PREVIOUSTOOLDIR)
.  if exists(PREVIOUSTOOLDIR)
PREVIOUSTOOLDIR!=	cat PREVIOUSTOOLDIR
.  else
PREVIOUSTOOLDIR=
.  endif
.endif

CLEANFILES+=	PREVIOUSTOOLDIR

realall realdepend: .MAKE
.if !empty(PREVIOUSTOOLDIR) && "${PREVIOUSTOOLDIR}" != "${TOOLDIR}"
	@echo "*** WARNING: TOOLDIR has moved?"
	@echo "*** PREVIOUSTOOLDIR '${PREVIOUSTOOLDIR}'"
	@echo "***     !=  TOOLDIR '${TOOLDIR}'"
	@echo "*** Cleaning mis-matched tools"
	rm -f PREVIOUSTOOLDIR
	(cd ${.CURDIR} && ${MAKE} PREVIOUSTOOLDIR=${TOOLDIR} cleandir)
.endif
	echo ${TOOLDIR} >PREVIOUSTOOLDIR

cleandir:
	rm -f ${CLEANFILES}
