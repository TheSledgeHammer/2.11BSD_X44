#	$NetBSD: bsd.own.mk,v 1.1247 2021/05/06 13:23:36 rin Exp $

.include <bsd.own.mk>

#
# CPU model, derived from MACHINE_ARCH
#
MACHINE?=
MACHINE_CPU?=
MACHINE_ARCH?=		

#
# Subdirectory or path component used for the following paths:
#   distrib/${RELEASEMACHINE}
#   distrib/notes/${RELEASEMACHINE}
#   etc/etc.${RELEASEMACHINE}
# Used when building a release.
#
RELEASEMACHINE?=	${MACHINE}

#
# NEED_OWN_INSTALL_TARGET is set to "no" by pkgsrc/mk/bsd.pkg.mk to
# ensure that things defined by <bsd.own.mk> (default targets,
# INSTALL_FILE, etc.) are not conflicting with bsd.pkg.mk.
#
NEED_OWN_INSTALL_TARGET?=	yes

#
# This lists the platforms which do not have working in-tree toolchains.  For
# the in-tree gcc toolchain, this list is empty.
#
# If some future port is not supported by the in-tree toolchain, this should
# be set to "yes" for that port only.
#
# .if ${MACHINE} == "playstation2"
# TOOLCHAIN_MISSING?=	yes
# .endif

TOOLCHAIN_MISSING?=	no

#
# GCC Using platforms.
#
.if ${MKGCC:Uyes} != "no"

#
# What GCC is used?
#
.if ${MACHINE} == "alpha" || \
    ${MACHINE} == "hppa" || \
    ${MACHINE} == "ia64" || \
    ${MACHINE} == "sparc" || \
    ${MACHINE} == "sparc64" || \
    ${MACHINE} == "vax" || \
    ${MACHINE_ARCH} == "x86_64" || \
    ${MACHINE_CPU} == "aarch64" || \
    ${MACHINE_CPU} == "powerpc" || \
    ${MACHINE_CPU} == "riscv"
HAVE_GCC?=	10
.else
HAVE_GCC?=	9
.endif

.if ${HAVE_GCC} == 			10
EXTERNAL_GCC_SUBDIR?=		gcc
.else
EXTERNAL_GCC_SUBDIR?=		/does/not/exist
.endif
.else
MKGCCCMDS?=	no
.endif

#
# What binutils is used?
#
HAVE_BINUTILS?=				234

.if ${HAVE_BINUTILS} == 	234
EXTERNAL_BINUTILS_SUBDIR=	binutils
.else
EXTERNAL_BINUTILS_SUBDIR=	/does/not/exist
.endif

#
# What GDB is used?
#
HAVE_GDB?=					1100

.if ${HAVE_GDB} == 			1100
EXTERNAL_GDB_SUBDIR=		gdb


#
# Determine if running in the NetBSD source tree by checking for the
# existence of build.sh and tools/ in the current or a parent directory,
# and setting _SRC_TOP_ to the result.
#
.if !defined(_SRC_TOP_)			# {
_SRC_TOP_!= cd "${.CURDIR}"; while :; do \
		here=`pwd`; \
		[ -f build.sh  ] && [ -d tools ] && { echo $$here; break; }; \
		case $$here in /) echo ""; break;; esac; \
		cd ..; done

.MAKEOVERRIDES+=	_SRC_TOP_

.endif					# }

#
# If _SRC_TOP_ != "", we're within the NetBSD source tree.
# * Set defaults for NETBSDSRCDIR and _SRC_TOP_OBJ_.
# * Define _NETBSD_VERSION_DEPENDS.  Targets that depend on the
#   NetBSD version, or on variables defined at build time, can
#   declare a dependency on ${_NETBSD_VERSION_DEPENDS}.
#
.if (${_SRC_TOP_} != "")		# {

NETBSDSRCDIR?=	${_SRC_TOP_}

.if !defined(_SRC_TOP_OBJ_)
_SRC_TOP_OBJ_!=		cd "${_SRC_TOP_}" && ${PRINTOBJDIR}
.MAKEOVERRIDES+=	_SRC_TOP_OBJ_
.endif

_NETBSD_VERSION_DEPENDS=	${_SRC_TOP_OBJ_}/params
_NETBSD_VERSION_DEPENDS+=	${NETBSDSRCDIR}/sys/sys/param.h
_NETBSD_VERSION_DEPENDS+=	${NETBSDSRCDIR}/sys/conf/newvers.sh
_NETBSD_VERSION_DEPENDS+=	${NETBSDSRCDIR}/sys/conf/osrelease.sh
${_SRC_TOP_OBJ_}/params: .NOTMAIN .OPTIONAL # created by top level "make build"

.endif	# _SRC_TOP_ != ""		# }

.if (${_SRC_TOP_} != "") && \
    (${TOOLCHAIN_MISSING} == "no" || defined(EXTERNAL_TOOLCHAIN))
USETOOLS?=	yes
.endif
USETOOLS?=	no

.if ${MACHINE_ARCH} == "mips" || ${MACHINE_ARCH} == "mips64" || \
    ${MACHINE_ARCH} == "sh3"
.BEGIN:
	@echo "Must set MACHINE_ARCH to one of ${MACHINE_ARCH}eb or ${MACHINE_ARCH}el"
	@false
.elif defined(REQUIRETOOLS) && \
      (${TOOLCHAIN_MISSING} == "no" || defined(EXTERNAL_TOOLCHAIN)) && \
      ${USETOOLS} == "no"
.BEGIN:
	@echo "USETOOLS=no, but this component requires a version-specific host toolchain"
	@false
.endif

#
# Host platform information; may be overridden
#
.include <bsd.host.mk>

.if ${USETOOLS} == "yes"						# {

#
# Provide a default for TOOLDIR.
#
.if !defined(TOOLDIR)
TOOLDIR:=	${_SRC_TOP_OBJ_}/tooldir.${HOST_OSTYPE}
.MAKEOVERRIDES+= TOOLDIR
.endif

#
# This is the prefix used for the NetBSD-sourced tools.
#
_TOOL_PREFIX?=	nb

#
# If an external toolchain base is specified, use it.
#
.if defined(EXTERNAL_TOOLCHAIN)						# {
AR=					${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-ar
AS=					${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-as
LD=					${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-ld
NM=					${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-nm
OBJCOPY=			${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-objcopy
OBJDUMP=			${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-objdump
RANLIB=				${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-ranlib
READELF=			${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-readelf
SIZE=				${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-size
STRINGS=			${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-strings
STRIP=				${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-strip

TOOL_CC.gcc=		${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-gcc
TOOL_CPP.gcc=		${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-cpp
TOOL_CXX.gcc=		${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-c++
TOOL_FC.gcc=		${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-gfortran
TOOL_OBJC.gcc=		${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-gcc

TOOL_CC.clang=		${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-clang
TOOL_CPP.clang=		${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-clang-cpp
TOOL_CXX.clang=		${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-clang++
TOOL_OBJC.clang=	${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-clang
else									# } {
# Define default locations for common tools.
.if ${USETOOLS_BINUTILS:Uyes} == "yes"					#  {
AR=					${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-ar
AS=					${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-as
LD=					${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-ld
NM=					${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-nm
OBJCOPY=			${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-objcopy
OBJDUMP=			${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-objdump
RANLIB=				${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-ranlib
READELF=			${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-readelf
SIZE=				${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-size
STRINGS=			${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-strings
STRIP=				${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-strip

# GCC supports C, C++, Fortran and Objective C
TOOL_CC.gcc=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-gcc
TOOL_CPP.gcc=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-cpp
TOOL_CXX.gcc=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-c++
TOOL_FC.gcc=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-gfortran
TOOL_OBJC.gcc=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-gcc
.endif									#  }

# Clang supports C, C++ and Objective C
TOOL_CC.clang=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-clang
TOOL_CPP.clang=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-clang-cpp
TOOL_CXX.clang=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-clang++
TOOL_OBJC.clang=	${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-clang

# PCC supports C and Fortran
TOOL_CC.pcc=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-pcc
TOOL_CPP.pcc=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-pcpp
TOOL_CXX.pcc=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-p++
.endif	# EXTERNAL_TOOLCHAIN						# }

#
# Make sure DESTDIR is set, so that builds with these tools always
# get appropriate -nostdinc, -nostdlib, etc. handling.  The default is
# <empty string>, meaning start from /, the root directory.
#
DESTDIR?=


# Don't append another copy of sysroot (coming from COMPATCPPFLAGS etc.)
# because it confuses Coverity. Still we need to cov-configure specially
# for each specific sysroot argument.
# Also don't add a sysroot at all if a rumpkernel build.
.if !defined(HOSTPROG) && !defined(HOSTLIB) && !defined(RUMPRUN)
.  if ${DESTDIR} != ""
.	if empty(CPPFLAGS:M*--sysroot=*)
CPPFLAGS+=	--sysroot=${DESTDIR}
.	endif
LDFLAGS+=	--sysroot=${DESTDIR}
.  else
.	if empty(CPPFLAGS:M*--sysroot=*)
CPPFLAGS+=	--sysroot=/
.	endif
LDFLAGS+=	--sysroot=/
.  endif
.endif

# Missing below TOOLDIR from NetBSD's bsd.own.mk

.else	# USETOOLS != yes						# } {

# Clang supports C, C++ and Objective C
TOOL_CC.clang=		clang
TOOL_CPP.clang=		clang-cpp
TOOL_CXX.clang=		clang++
TOOL_OBJC.clang=	clang

# GCC supports C, C++, Fortran and Objective C
TOOL_CC.gcc=		gcc
TOOL_CPP.gcc=		cpp
TOOL_CXX.gcc=		c++
TOOL_FC.gcc=		gfortran
TOOL_OBJC.gcc=		gcc

# PCC supports C and Fortran
TOOL_CC.pcc=		pcc
TOOL_CPP.pcc=		pcpp
TOOL_CXX.pcc=		p++

# Missing below TOOLDIR from NetBSD's bsd.own.mk

.endif	# USETOOLS != yes						# }

# Standalone code should not be compiled with PIE or CTF
# Should create a better test
.if defined(BINDIR) && ${BINDIR} == "/usr/mdec"
NOPIE=			# defined
NOCTF=			# defined
.elif ${MACHINE} == "sun2"
NOPIE=			# we don't have PIC, so no PIE
.endif

# Fallback to ensure that all variables are defined to something
TOOL_CC.false=		false
TOOL_CPP.false=		false
TOOL_CXX.false=		false
TOOL_FC.false=		false
TOOL_OBJC.false=	false

AVAILABLE_COMPILER?=	${HAVE_PCC:Dpcc} ${HAVE_LLVM:Dclang} ${HAVE_GCC:Dgcc} false

.for _t in CC CPP CXX FC OBJC
ACTIVE_${_t}=	${AVAILABLE_COMPILER:@.c.@ ${ !defined(UNSUPPORTED_COMPILER.${.c.}) && defined(TOOL_${_t}.${.c.}) :? ${.c.} : }@:[1]}
SUPPORTED_${_t}=${AVAILABLE_COMPILER:Nfalse:@.c.@ ${ !defined(UNSUPPORTED_COMPILER.${.c.}) && defined(TOOL_${_t}.${.c.}) :? ${.c.} : }@}
.endfor
# make bugs prevent moving this into the .for loop
CC=		${TOOL_CC.${ACTIVE_CC}}
CPP=	${TOOL_CPP.${ACTIVE_CPP}}
CXX=	${TOOL_CXX.${ACTIVE_CXX}}
FC=		${TOOL_FC.${ACTIVE_FC}}
OBJC=	${TOOL_OBJC.${ACTIVE_OBJC}}

# For each ${MACHINE_CPU}, list the ports that use it.
MACHINES.i386=		i386

#
# Targets to check if DESTDIR or RELEASEDIR is provided
#
.if !target(check_DESTDIR)
check_DESTDIR: .PHONY .NOTMAIN
.if !defined(DESTDIR)
	@echo "setenv DESTDIR before doing that!"
	@false
.else
	@true
.endif
.endif

.if !target(check_RELEASEDIR)
check_RELEASEDIR: .PHONY .NOTMAIN
.if !defined(RELEASEDIR)
	@echo "setenv RELEASEDIR before doing that!"
	@false
.else
	@true
.endif
.endif

#
# Location of the file that contains the major and minor numbers of the
# version of a shared library.  If this file exists a shared library
# will be built by <bsd.lib.mk>.
#
SHLIB_VERSION_FILE?= ${.CURDIR}/shlib_version

#
# GNU sources and packages sometimes see architecture names differently.
#
GNU_ARCH.i386=i486
GCC_CONFIG_ARCH.i386=i486
GCC_CONFIG_TUNE.i386=nocona

#
# In order to identify NetBSD to GNU packages, we sometimes need
# an "elf" tag for historically a.out platforms.
#
.if (!empty(MACHINE_ARCH:Mearm*))
MACHINE_GNU_PLATFORM?=${MACHINE_GNU_ARCH}--netbsdelf-${MACHINE_ARCH:C/eb//:C/v[4-7]//:S/earm/eabi/}
.elif (${MACHINE_GNU_ARCH} == "arm" || \
     ${MACHINE_GNU_ARCH} == "armeb" || \
     ${MACHINE_ARCH} == "i386" || \
     ${MACHINE_CPU} == "m68k" || \
     ${MACHINE_GNU_ARCH} == "sh" || \
     ${MACHINE_GNU_ARCH} == "shle" || \
     ${MACHINE_ARCH} == "sparc" || \
     ${MACHINE_ARCH} == "vax")
MACHINE_GNU_PLATFORM?=${MACHINE_GNU_ARCH}--netbsdelf
.else
MACHINE_GNU_PLATFORM?=${MACHINE_GNU_ARCH}--netbsd
.endif