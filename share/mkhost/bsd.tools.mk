#	$NetBSD: bsd.own.mk,v 1.1247 2021/05/06 13:23:36 rin Exp $

.include <bsd.own.mk>

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

INSTALL=			${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-install
LEX=				${TOOLDIR}/bin/${_TOOL_PREFIX}lex
LINT=				CC=${CC:Q} ${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-lint
LORDER=				NM=${NM:Q} MKTEMP=${TOOL_MKTEMP:Q} ${TOOLDIR}/bin/${_TOOL_PREFIX}lorder
MKDEP=				CC=${CC:Q} ${TOOLDIR}/bin/${_TOOL_PREFIX}mkdep
MKDEPCXX=			CC=${CXX:Q} ${TOOLDIR}/bin/${_TOOL_PREFIX}mkdep
#PAXCTL=				${TOOLDIR}/bin/${_TOOL_PREFIX}paxctl
TSORT=				${TOOLDIR}/bin/${_TOOL_PREFIX}tsort -q
YACC=				${TOOLDIR}/bin/${_TOOL_PREFIX}yacc

TOOL_AWK=			${TOOLDIR}/bin/${_TOOL_PREFIX}awk
TOOL_CAP_MKDB=		${TOOLDIR}/bin/${_TOOL_PREFIX}cap_mkdb
TOOL_CAT=			${TOOLDIR}/bin/${_TOOL_PREFIX}cat
TOOL_CKSUM=			${TOOLDIR}/bin/${_TOOL_PREFIX}cksum
TOOL_CLANG_TBLGEN=	${TOOLDIR}/bin/${_TOOL_PREFIX}clang-tblgen
TOOL_COMPILE_ET=	${TOOLDIR}/bin/${_TOOL_PREFIX}compile_et
TOOL_CONFIG=		${TOOLDIR}/bin/${_TOOL_PREFIX}config
TOOL_CRUNCHGEN=		MAKE=${.MAKE:Q} ${TOOLDIR}/bin/${_TOOL_PREFIX}crunchgen
TOOL_CTAGS=			${TOOLDIR}/bin/${_TOOL_PREFIX}ctags
TOOL_CTFCONVERT=	${TOOLDIR}/bin/${_TOOL_PREFIX}ctfconvert
TOOL_CTFMERGE=		${TOOLDIR}/bin/${_TOOL_PREFIX}ctfmerge
TOOL_CVSLATEST=		${TOOLDIR}/bin/${_TOOL_PREFIX}cvslatest
TOOL_DB=			${TOOLDIR}/bin/${_TOOL_PREFIX}db
TOOL_DISKLABEL=		${TOOLDIR}/bin/${_TOOL_PREFIX}disklabel
TOOL_DTC=			${TOOLDIR}/bin/${_TOOL_PREFIX}dtc
TOOL_EQN=			${TOOLDIR}/bin/${_TOOL_PREFIX}eqn
TOOL_FDISK=			${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-fdisk
TOOL_FGEN=			${TOOLDIR}/bin/${_TOOL_PREFIX}fgen
TOOL_GENASSYM=		${TOOLDIR}/bin/${_TOOL_PREFIX}genassym
TOOL_GENCAT=		${TOOLDIR}/bin/${_TOOL_PREFIX}gencat
TOOL_GMAKE=			${TOOLDIR}/bin/${_TOOL_PREFIX}gmake
TOOL_GPT=			${TOOLDIR}/bin/${_TOOL_PREFIX}gpt
TOOL_GREP=			${TOOLDIR}/bin/${_TOOL_PREFIX}grep
GROFF_SHARE_PATH=	${TOOLDIR}/share/groff
TOOL_GROFF_ENV= \
    GROFF_ENCODING= \
    GROFF_BIN_PATH=${TOOLDIR}/lib/groff \
    GROFF_FONT_PATH=${GROFF_SHARE_PATH}/site-font:${GROFF_SHARE_PATH}/font \
    GROFF_TMAC_PATH=${GROFF_SHARE_PATH}/site-tmac:${GROFF_SHARE_PATH}/tmac
TOOL_GROFF=			${TOOL_GROFF_ENV} ${TOOLDIR}/bin/${_TOOL_PREFIX}groff ${GROFF_FLAGS}

TOOL_HEXDUMP=		${TOOLDIR}/bin/${_TOOL_PREFIX}hexdump
TOOL_HP300MKBOOT=	${TOOLDIR}/bin/${_TOOL_PREFIX}hp300-mkboot
TOOL_HPPAMKBOOT=	${TOOLDIR}/bin/${_TOOL_PREFIX}hppa-mkboot
TOOL_INDXBIB=		${TOOLDIR}/bin/${_TOOL_PREFIX}indxbib
TOOL_INSTALLBOOT=	${TOOLDIR}/bin/${_TOOL_PREFIX}installboot
TOOL_INSTALL_INFO=	${TOOLDIR}/bin/${_TOOL_PREFIX}install-info
TOOL_JOIN=			${TOOLDIR}/bin/${_TOOL_PREFIX}join
TOOL_LLVM_TBLGEN=	${TOOLDIR}/bin/${_TOOL_PREFIX}llvm-tblgen
TOOL_M4=			${TOOLDIR}/bin/${_TOOL_PREFIX}m4
TOOL_MACPPCFIXCOFF=	${TOOLDIR}/bin/${_TOOL_PREFIX}macppc-fixcoff
TOOL_MAKEFS=		${TOOLDIR}/bin/${_TOOL_PREFIX}makefs
TOOL_MAKEINFO=		${TOOLDIR}/bin/${_TOOL_PREFIX}makeinfo
TOOL_MAKEKEYS=		${TOOLDIR}/bin/${_TOOL_PREFIX}makekeys
TOOL_MAKESTRS=		${TOOLDIR}/bin/${_TOOL_PREFIX}makestrs
TOOL_MAKEWHATIS=	${TOOLDIR}/bin/${_TOOL_PREFIX}makewhatis
TOOL_MANDOC_ASCII=	${TOOLDIR}/bin/${_TOOL_PREFIX}mandoc -Tascii
TOOL_MANDOC_HTML=	${TOOLDIR}/bin/${_TOOL_PREFIX}mandoc -Thtml
TOOL_MANDOC_LINT=	${TOOLDIR}/bin/${_TOOL_PREFIX}mandoc -Tlint
TOOL_MDSETIMAGE=	${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-mdsetimage
TOOL_MENUC=			MENUDEF=${TOOLDIR}/share/misc ${TOOLDIR}/bin/${_TOOL_PREFIX}menuc
TOOL_ARMELF2AOUT=	${TOOLDIR}/bin/${_TOOL_PREFIX}arm-elf2aout
TOOL_M68KELF2AOUT=	${TOOLDIR}/bin/${_TOOL_PREFIX}m68k-elf2aout
TOOL_MIPSELF2ECOFF=	${TOOLDIR}/bin/${_TOOL_PREFIX}mips-elf2ecoff
TOOL_MKCSMAPPER=	${TOOLDIR}/bin/${_TOOL_PREFIX}mkcsmapper
TOOL_MKESDB=		${TOOLDIR}/bin/${_TOOL_PREFIX}mkesdb
TOOL_MKLOCALE=		${TOOLDIR}/bin/${_TOOL_PREFIX}mklocale
TOOL_MKMAGIC=		${TOOLDIR}/bin/${_TOOL_PREFIX}file
TOOL_MKNOD=			${TOOLDIR}/bin/${_TOOL_PREFIX}mknod
TOOL_MKTEMP=		${TOOLDIR}/bin/${_TOOL_PREFIX}mktemp
TOOL_MKUBOOTIMAGE=	${TOOLDIR}/bin/${_TOOL_PREFIX}mkubootimage
TOOL_ELFTOSB=		${TOOLDIR}/bin/${_TOOL_PREFIX}elftosb
TOOL_MSGC=			MSGDEF=${TOOLDIR}/share/misc ${TOOLDIR}/bin/${_TOOL_PREFIX}msgc
TOOL_MTREE=			${TOOLDIR}/bin/${_TOOL_PREFIX}mtree
TOOL_MVME68KWRTVID=	${TOOLDIR}/bin/${_TOOL_PREFIX}mvme68k-wrtvid
TOOL_NBPERF=		${TOOLDIR}/bin/${_TOOL_PREFIX}perf
TOOL_NCDCS=			${TOOLDIR}/bin/${_TOOL_PREFIX}ibmnws-ncdcs
TOOL_PAX=			${TOOLDIR}/bin/${_TOOL_PREFIX}pax
TOOL_PIC=			${TOOLDIR}/bin/${_TOOL_PREFIX}pic
TOOL_PIGZ=			${TOOLDIR}/bin/${_TOOL_PREFIX}pigz
TOOL_XZ=			${TOOLDIR}/bin/${_TOOL_PREFIX}xz
TOOL_PKG_CREATE=	${TOOLDIR}/bin/${_TOOL_PREFIX}pkg_create
TOOL_POWERPCMKBOOTIMAGE=${TOOLDIR}/bin/${_TOOL_PREFIX}powerpc-mkbootimage
TOOL_PWD_MKDB=		${TOOLDIR}/bin/${_TOOL_PREFIX}pwd_mkdb
TOOL_REFER=			${TOOLDIR}/bin/${_TOOL_PREFIX}refer
TOOL_ROFF_ASCII=	${TOOL_GROFF_ENV} ${TOOLDIR}/bin/${_TOOL_PREFIX}nroff
TOOL_ROFF_DOCASCII=	${TOOL_GROFF} -Tascii
TOOL_ROFF_DOCHTML=	${TOOL_GROFF} -Thtml
TOOL_ROFF_DVI=		${TOOL_GROFF} -Tdvi ${ROFF_PAGESIZE}
TOOL_ROFF_HTML=		${TOOL_GROFF} -Tlatin1 -mdoc2html
TOOL_ROFF_PS=		${TOOL_GROFF} -Tps ${ROFF_PAGESIZE}
TOOL_ROFF_RAW=		${TOOL_GROFF} -Z
TOOL_RPCGEN=		RPCGEN_CPP=${CPP:Q} ${TOOLDIR}/bin/${_TOOL_PREFIX}rpcgen
TOOL_SED=			${TOOLDIR}/bin/${_TOOL_PREFIX}sed
TOOL_SLC=			${TOOLDIR}/bin/${_TOOL_PREFIX}slc
TOOL_SOELIM=		${TOOLDIR}/bin/${_TOOL_PREFIX}soelim
TOOL_SORTINFO=		${TOOLDIR}/bin/${_TOOL_PREFIX}sortinfo
TOOL_SPARKCRC=		${TOOLDIR}/bin/${_TOOL_PREFIX}sparkcrc
TOOL_STAT=			${TOOLDIR}/bin/${_TOOL_PREFIX}stat
TOOL_STRFILE=		${TOOLDIR}/bin/${_TOOL_PREFIX}strfile
TOOL_SUNLABEL=		${TOOLDIR}/bin/${_TOOL_PREFIX}sunlabel
TOOL_TBL=			${TOOLDIR}/bin/${_TOOL_PREFIX}tbl
TOOL_TIC=			${TOOLDIR}/bin/${_TOOL_PREFIX}tic
TOOL_UUDECODE=		${TOOLDIR}/bin/${_TOOL_PREFIX}uudecode
TOOL_VGRIND=		${TOOLDIR}/bin/${_TOOL_PREFIX}vgrind -f
TOOL_VFONTEDPR=		${TOOLDIR}/libexec/${_TOOL_PREFIX}vfontedpr
TOOL_ZIC=			${TOOLDIR}/bin/${_TOOL_PREFIX}zic

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