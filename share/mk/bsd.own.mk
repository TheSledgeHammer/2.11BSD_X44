#	$NetBSD: bsd.own.mk,v 1.1247 2021/05/06 13:23:36 rin Exp $
#  
#	Notes: bsd.own.mk only defines parameters related to the toolchain,
#	machine archictechure and compiler.
#

# This needs to be before bsd.init.mk
.if defined(BSD_MK_COMPAT_FILE)
.include <${BSD_MK_COMPAT_FILE}>
.endif

.if !defined(_BSD_OWN_MK_)
_BSD_OWN_MK_=1

MAKECONF?=	/etc/mk.conf
.-include "${MAKECONF}"

#
# CPU model, derived from MACHINE_ARCH
#
MACHINE_CPU= ${MACHINE_ARCH:C/e?arm.*/arm/:S/aarch64eb/aarch64/:C/riscv../riscv/}

.if (${MACHINE_ARCH} == "mips64el" || \
     ${MACHINE_ARCH} == "mips64eb" || \
     ${MACHINE_ARCH} == "mipsn64el" || \
     ${MACHINE_ARCH} == "mipsn64eb")
MACHINE_MIPS64= 	1
.else
MACHINE_MIPS64= 	0
.endif

#
# Subdirectory used below ${RELEASEDIR} when building a release
#
.if !empty(MACHINE:Mevbarm)
RELEASEMACHINEDIR?=	${MACHINE}-${MACHINE_ARCH}
.else
RELEASEMACHINEDIR?=	${MACHINE}
.endif

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
#
# We import the old gcc as "gcc.old" when upgrading.  EXTERNAL_GCC_SUBDIR is
# set to the relevant subdirectory in src/external/gpl3 for his HAVE_GCC.
#
HAVE_GCC?= 					10

.if ${HAVE_GCC} == 			10
EXTERNAL_GCC_SUBDIR=		gcc
.elif ${HAVE_GCC} == 		9
EXTERNAL_GCC_SUBDIR=		gcc.old
.else
EXTERNAL_GCC_SUBDIR=		/does/not/exist
.endif
.else
MKGCCCMDS?=			no
.endif

#
# What binutils is used?
#
HAVE_BINUTILS?=			234

.if ${HAVE_BINUTILS} == 	234
EXTERNAL_BINUTILS_SUBDIR=	binutils
.elif ${HAVE_BINUTILS} == 	231
EXTERNAL_BINUTILS_SUBDIR=	binutils.old
.else
EXTERNAL_BINUTILS_SUBDIR=	/does/not/exist
.endif

#
# What GDB is used?
#
HAVE_GDB?=			1100

.if ${HAVE_GDB} == 		1100
EXTERNAL_GDB_SUBDIR=		gdb
.elif ${HAVE_GDB} == 		830
EXTERNAL_GDB_SUBDIR=		gdb.old
.else
EXTERNAL_GDB_SUBDIR=		/does/not/exist
.endif

#
# What LibreSSL is used?
# 
HAVE_LIBRESSL?=	40

.if ${HAVE_LIBRESSL} == 40
EXTERNAL_LIBRESSL_SUBDIR=libressl
.elif ${HAVE_LIBRESSL} == 39
EXTERNAL_LIBRESSL_SUBDIR=libressl.old
.else
EXTERNAL_LIBRESSL_SUBDIR=/does/not/exist
.endif

#
# What OpenSSL is used?
# 
HAVE_OPENSSL?=	11

.if ${HAVE_OPENSSL} == 11
EXTERNAL_OPENSSL_SUBDIR=openssl
.elif ${HAVE_OPENSSL} == 10
EXTERNAL_OPENSSL_SUBDIR=openssl.old
.else
EXTERNAL_OPENSSL_SUBDIR=/does/not/exist
.endif

#
# Compile OpenSSL or LibreSSL.
#
#
# Change MKOPENSSL below for alternative Crypto libraries 
# For OpenSSL:  MKOPENSSL? = yes
# For LibreSSL: MKOPENSSL? = no
#
MKOPENSSL?= no

.if ${MKOPENSSL:Uno} != "no" 
MKLIBRESSL? = no
.else
MKLIBRESSL? = yes
EXTERNAL_OPENSSL_SUBDIR= ${EXTERNAL_LIBRESSL_SUBDIR}
.endif

#
# Does the platform support ACPI?
#

#
# Does the platform support UEFI?
#

#
# Does the platform support NVMM?
#

.if !empty(MACHINE_ARCH:Mearm*)
_LIBC_COMPILER_RT.${MACHINE_ARCH}=	yes
.endif

_LIBC_COMPILER_RT.aarch64=		yes
_LIBC_COMPILER_RT.aarch64eb=		yes
_LIBC_COMPILER_RT.i386=			yes
_LIBC_COMPILER_RT.x86_64=		yes

.if ${HAVE_LLVM:Uno} == "yes" && ${_LIBC_COMPILER_RT.${MACHINE_ARCH}:Uno} == "yes"
HAVE_LIBGCC?=	no
.else
HAVE_LIBGCC?=	yes
.endif

# Should libgcc have unwinding code?
.if ${HAVE_LLVM:Uno} == "yes" || !empty(MACHINE_ARCH:Mearm*)
HAVE_LIBGCC_EH?=	no
.else
HAVE_LIBGCC_EH?=	yes
.endif

# Coverity does not like SSP
.if defined(COVERITY_TOP_CONFIG) || \
    ${MACHINE} == "alpha" || \
    ${MACHINE} == "hppa" || \
    ${MACHINE} == "ia64"
HAVE_SSP?=	no
.else
HAVE_SSP?=	yes
.if !defined(NOFORT) && ${USE_FORT:Uno} != "no"
USE_SSP?=	yes
.endif
.endif

#
# What version of jemalloc we use (100 is the one
# built-in to libc from 2005 (pre version 3).
#
.if ${MACHINE_ARCH} == "vax" || ${MACHINE} == "sun2"
HAVE_JEMALLOC?=		100
.else
HAVE_JEMALLOC?=		510
.endif

.if empty(.MAKEFLAGS:tW:M*-V .OBJDIR*)
.if defined(MAKEOBJDIRPREFIX) || defined(MAKEOBJDIR)
PRINTOBJDIR=	${MAKE} -r -V .OBJDIR -f /dev/null xxx
.else
PRINTOBJDIR=	${MAKE} -V .OBJDIR
.endif
.else
PRINTOBJDIR=	echo /error/bsd.own.mk/PRINTOBJDIR # avoid infinite recursion
.endif

#
# Determine if running in the NetBSD source tree by checking for the
# existence of build.sh and tools/ in the current or a parent directory,
# and setting _SRC_TOP_ to the result.
#
.if !defined(_SRC_TOP_)			# {
_SRC_TOP_!= cd ${.CURDIR}; while :; do \
		here=`pwd`; \
		[ -f build.sh  ] && [ -d tools ] && { echo $$here; break; }; \
		case $$here in /) echo ""; break;; esac; \
		cd ..; done

.MAKEOVERRIDES+=	_SRC_TOP_

.endif					# }

#
# If _SRC_TOP_ != "", we're within the NetBSD source tree, so set
# defaults for NETBSDSRCDIR and _SRC_TOP_OBJ_.
#
.if (${_SRC_TOP_} != "")		# {

NETBSDSRCDIR?=	${_SRC_TOP_}

.if !defined(_SRC_TOP_OBJ_)
_SRC_TOP_OBJ_!=		cd ${_SRC_TOP_} && ${PRINTOBJDIR}
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

.else									# } {
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
.endif	# EXTERNAL_TOOLCHAIN

#
# Make sure DESTDIR is set, so that builds with these tools always
# get appropriate -nostdinc, -nostdlib, etc. handling.  The default is
# <empty string>, meaning start from /, the root directory.
#
DESTDIR?=

# Don't append another copy of sysroot (coming from COMPATCPPFLAGS etc.)
# because it confuses Coverity. Still we need to cov-configure specially
# for each specific sysroot argument.
.if !defined(HOSTPROG) && !defined(HOSTLIB)
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

DBSYM=				${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-dbsym
INSTALL=			${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-install
LEX=				${TOOLDIR}/bin/${_TOOL_PREFIX}lex
LINT=				CC=${CC:Q} ${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-lint
LORDER=				NM=${NM:Q} MKTEMP=${TOOL_MKTEMP:Q} ${TOOLDIR}/bin/${_TOOL_PREFIX}lorder
MKDEP=				CC=${CC:Q} ${TOOLDIR}/bin/${_TOOL_PREFIX}mkdep
MKDEPCXX=			CC=${CXX:Q} ${TOOLDIR}/bin/${_TOOL_PREFIX}mkdep
PAXCTL=				${TOOLDIR}/bin/${_TOOL_PREFIX}paxctl
TSORT=				${TOOLDIR}/bin/${_TOOL_PREFIX}tsort -q
YACC=				${TOOLDIR}/bin/${_TOOL_PREFIX}yacc

TOOL_AR=            ${TOOLDIR}/bin/${_TOOL_PREFIX}ar
TOOL_ASN1_COMPILE=	${TOOLDIR}/bin/${_TOOL_PREFIX}asn1_compile
TOOL_AWK=			${TOOLDIR}/bin/${_TOOL_PREFIX}awk
TOOL_CAP_MKDB=		${TOOLDIR}/bin/${_TOOL_PREFIX}cap_mkdb
TOOL_CAT=			${TOOLDIR}/bin/${_TOOL_PREFIX}cat
TOOL_CKSUM=			${TOOLDIR}/bin/${_TOOL_PREFIX}cksum
TOOL_CLANG_TBLGEN=	${TOOLDIR}/bin/${_TOOL_PREFIX}clang-tblgen
TOOL_COMPILE_ET=	${TOOLDIR}/bin/${_TOOL_PREFIX}compile_et
TOOL_CONFIG=		${TOOLDIR}/bin/${_TOOL_PREFIX}config
TOOL_CRUNCHGEN=		MAKE=${.MAKE:Q} ${TOOLDIR}/bin/${_TOOL_PREFIX}crunchgen
TOOL_CTAGS=			${TOOLDIR}/bin/${_TOOL_PREFIX}ctags
TOOL_DATE=			${TOOLDIR}/bin/${_TOOL_PREFIX}date
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
TOOL_GROFF_ENV= 															\
    GROFF_ENCODING= 														\
    GROFF_BIN_PATH=	${TOOLDIR}/lib/groff 									\
    GROFF_FONT_PATH=${GROFF_SHARE_PATH}/site-font:${GROFF_SHARE_PATH}/font 	\
    GROFF_TMAC_PATH=${GROFF_SHARE_PATH}/site-tmac:${GROFF_SHARE_PATH}/tmac
TOOL_GROFF=			${TOOL_GROFF_ENV} ${TOOLDIR}/bin/${_TOOL_PREFIX}groff ${GROFF_FLAGS}

TOOL_HEXDUMP=		${TOOLDIR}/bin/${_TOOL_PREFIX}hexdump
TOOL_INDXBIB=		${TOOLDIR}/bin/${_TOOL_PREFIX}indxbib
TOOL_INSTALLBOOT=	${TOOLDIR}/bin/${_TOOL_PREFIX}installboot
TOOL_INSTALL_INFO=	${TOOLDIR}/bin/${_TOOL_PREFIX}install-info
TOOL_JOIN=			${TOOLDIR}/bin/${_TOOL_PREFIX}join
TOOL_LLVM_TBLGEN=	${TOOLDIR}/bin/${_TOOL_PREFIX}llvm-tblgen
TOOL_M4=			${TOOLDIR}/bin/${_TOOL_PREFIX}m4
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
TOOL_NBPERF=		${TOOLDIR}/bin/${_TOOL_PREFIX}perf
TOOL_NCDCS=			${TOOLDIR}/bin/${_TOOL_PREFIX}ibmnws-ncdcs
TOOL_PAX=			${TOOLDIR}/bin/${_TOOL_PREFIX}pax
TOOL_PIC=			${TOOLDIR}/bin/${_TOOL_PREFIX}pic
TOOL_PIGZ=			${TOOLDIR}/bin/${_TOOL_PREFIX}pigz
TOOL_XZ=			${TOOLDIR}/bin/${_TOOL_PREFIX}xz
TOOL_PKG_CREATE=	${TOOLDIR}/bin/${_TOOL_PREFIX}pkg_create
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
TOOL_STAT=			${TOOLDIR}/bin/${_TOOL_PREFIX}stat
TOOL_STRFILE=		${TOOLDIR}/bin/${_TOOL_PREFIX}strfile
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

TOOL_AR=			ar
TOOL_ASN1_COMPILE=	asn1_compile
TOOL_AWK=			awk
TOOL_CAP_MKDB=		cap_mkdb
TOOL_CAT=			cat
TOOL_CKSUM=			cksum
TOOL_CLANG_TBLGEN=	clang-tblgen
TOOL_COMPILE_ET=	compile_et
TOOL_CONFIG=		config
TOOL_CRUNCHGEN=		crunchgen
TOOL_CTAGS=			ctags
TOOL_DATE=			date
TOOL_DB=			db
TOOL_DISKLABEL=		disklabel
TOOL_DTC=			dtc
TOOL_EQN=			eqn
TOOL_FDISK=			fdisk
TOOL_FGEN=			fgen
TOOL_GENASSYM=		genassym
TOOL_GENCAT=		gencat
TOOL_GMAKE=			gmake
TOOL_GPT=			gpt
TOOL_GREP=			grep
TOOL_GROFF=			groff
TOOL_HEXDUMP=		hexdump
TOOL_INDXBIB=		indxbib
TOOL_INSTALLBOOT=	installboot
TOOL_INSTALL_INFO=	install-info
TOOL_JOIN=			join
TOOL_LLVM_TBLGEN=	llvm-tblgen
TOOL_M4=			m4
TOOL_MAKEFS=		makefs
TOOL_MAKEINFO=		makeinfo
TOOL_MAKEKEYS=		makekeys
TOOL_MAKESTRS=		makestrs
TOOL_MAKEWHATIS=	/usr/libexec/makewhatis
TOOL_MANDOC_ASCII=	mandoc -Tascii
TOOL_MANDOC_HTML=	mandoc -Thtml
TOOL_MANDOC_LINT=	mandoc -Tlint
TOOL_MDSETIMAGE=	mdsetimage
TOOL_MENUC=			menuc
TOOL_MKCSMAPPER=	mkcsmapper
TOOL_MKESDB=		mkesdb
TOOL_MKLOCALE=		mklocale
TOOL_MKMAGIC=		file
TOOL_MKNOD=			mknod
TOOL_MKTEMP=		mktemp
TOOL_MKUBOOTIMAGE=	mkubootimage
TOOL_ELFTOSB=		elftosb
TOOL_MSGC=			msgc
TOOL_MTREE=			mtree
TOOL_MVME68KWRTVID=	wrtvid
TOOL_NBPERF=		nbperf
TOOL_NCDCS=			ncdcs
TOOL_PAX=			pax
TOOL_PIC=			pic
TOOL_PIGZ=			pigz
TOOL_XZ=			xz
TOOL_PKG_CREATE=	pkg_create
TOOL_PWD_MKDB=		pwd_mkdb
TOOL_REFER=			refer
TOOL_ROFF_ASCII=	nroff
TOOL_ROFF_DOCASCII=	${TOOL_GROFF} -Tascii
TOOL_ROFF_DOCHTML=	${TOOL_GROFF} -Thtml
TOOL_ROFF_DVI=		${TOOL_GROFF} -Tdvi ${ROFF_PAGESIZE}
TOOL_ROFF_HTML=		${TOOL_GROFF} -Tlatin1 -mdoc2html
TOOL_ROFF_PS=		${TOOL_GROFF} -Tps ${ROFF_PAGESIZE}
TOOL_ROFF_RAW=		${TOOL_GROFF} -Z
TOOL_RPCGEN=		rpcgen
TOOL_SED=			sed
TOOL_SOELIM=		soelim
TOOL_SORTINFO=		sortinfo
TOOL_SPARKCRC=		sparkcrc
TOOL_STAT=			stat
TOOL_STRFILE=		strfile
TOOL_SUNLABEL=		sunlabel
TOOL_TBL=			tbl
TOOL_TIC=			tic
TOOL_UUDECODE=		uudecode
TOOL_VGRIND=		vgrind -f
TOOL_VFONTEDPR=		/usr/libexec/vfontedpr
TOOL_ZIC=			zic

.endif	# USETOOLS != yes						# }

# Standalone code should not be compiled with PIE or CTF
# Should create a better test

#|| ${BINDIR} == "/boot"
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
MACHINES.aarch64=	evbarm
MACHINES.arm=		acorn32 cats epoc32 evbarm hpcarm \
					iyonix netwinder shark zaurus
MACHINES.i386=		i386
MACHINES.riscv=		riscv
MACHINES.x86_64=	amd64

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

# where the system object and source trees are kept; can be configurable
# by the user in case they want them in ~/foosrc and ~/fooobj, for example
BSDSRCDIR?=		/usr/src
BSDOBJDIR?=		/usr/obj
NETBSDSRCDIR?=	${BSDSRCDIR}

BINGRP?=		wheel
BINOWN?=		root
BINMODE?=		555
NONBINMODE?=	444

SHAREDIR?=		/usr/share
SHAREGRP?=		wheel
SHAREOWN?=		root
SHAREMODE?=		${NONBINMODE}

MANDIR?=		/usr/share/man
MANGRP?=		wheel
MANOWN?=		root
MANMODE?=		${NONBINMODE}
MANINSTALL?=	${_MANINSTALL} ${_CATINSTALL}

INFODIR?=		/usr/share/info
INFOGRP?=		wheel
INFOOWN?=		root
INFOMODE?=		${NONBINMODE}

LIBDIR?=		/usr/lib

LINTLIBDIR?=	/usr/libdata/lint
LIBGRP?=		${BINGRP}
LIBOWN?=		${BINOWN}
LIBMODE?=		${NONBINMODE}

DOCDIR?=    	/usr/share/doc
HTMLDOCDIR?=	/usr/share/doc/html
DOCGRP?=		wheel
DOCOWN?=		root
DOCMODE?=   	${NONBINMODE}

NLSDIR?=		/usr/share/nls
NLSGRP?=		wheel
NLSOWN?=		root
NLSMODE?=		${NONBINMODE}

LOCALEDIR?=		/usr/share/locale
LOCALEGRP?=		wheel
LOCALEOWN?=		root
LOCALEMODE?=	${NONBINMODE}

DEBUGDIR?=		/usr/libdata/debug
DEBUGGRP?=		wheel
DEBUGOWN?=		root
DEBUGMODE?=		${NONBINMODE}

MKDIRMODE?=		0755
MKDIRPERM?=		-m ${MKDIRMODE}

# Data-driven table using make variables to control how 
# toolchain-dependent targets and shared libraries are built
# for different platforms and object formats.
# OBJECT_FMT:		currently either "ELF" or "a.out".
#

OBJECT_FMT=	ELF

#
# If this platform's toolchain is missing, we obviously cannot build it.
#
.if ${TOOLCHAIN_MISSING} != "no"
MKBINUTILS:= no
MKGDB:= no
MKGCC:= no
.endif

#
# If we are using an external toolchain, we can still build the target's
# BFD stuff, but we cannot build GCC's support libraries, since those are
# tightly-coupled to the version of GCC being used.
#
.if defined(EXTERNAL_TOOLCHAIN)
MKGCC:= no
.endif

# No GDB support for aarch64eb
MKGDB.riscv32=	no
MKGDB.riscv64=	no

# No kernel modules for or1k, riscv or mips (yet)
MKKMOD.riscv32=	no
MKKMOD.riscv64=	no

# No profiling for or1k (yet)
MKPROFILE.riscv32=no
MKPROFILE.riscv64=no

#
# GCC warnings with simple disables.  Use these with eg
# COPTS.foo.c+= ${GCC_NO_STRINGOP_TRUNCATION}.
#
GCC_NO_FORMAT_TRUNCATION=	${${ACTIVE_CC} == "gcc" && ${HAVE_GCC:U0} >= 7:? -Wno-format-truncation :}
GCC_NO_FORMAT_OVERFLOW=		${${ACTIVE_CC} == "gcc" && ${HAVE_GCC:U0} >= 7:? -Wno-format-overflow :}
GCC_NO_STRINGOP_OVERFLOW=	${${ACTIVE_CC} == "gcc" && ${HAVE_GCC:U0} >= 7:? -Wno-stringop-overflow :}
GCC_NO_IMPLICIT_FALLTHRU=	${${ACTIVE_CC} == "gcc" && ${HAVE_GCC:U0} >= 7:? -Wno-implicit-fallthrough :}         
GCC_NO_STRINGOP_TRUNCATION=	${${ACTIVE_CC} == "gcc" && ${HAVE_GCC:U0} >= 8:? -Wno-stringop-truncation :}
GCC_NO_CAST_FUNCTION_TYPE=	${${ACTIVE_CC} == "gcc" && ${HAVE_GCC:U0} >= 8:? -Wno-cast-function-type :}
GCC_NO_ADDR_OF_PACKED_MEMBER=	${${ACTIVE_CC} == "gcc" && ${HAVE_GCC:U0} >= 9:? -Wno-address-of-packed-member :}
GCC_NO_MAYBE_UNINITIALIZED=	${${ACTIVE_CC} == "gcc" && ${HAVE_GCC:U0} >= 10:? -Wno-maybe-uninitialized :}
GCC_NO_RETURN_LOCAL_ADDR=	${${ACTIVE_CC} == "gcc" && ${HAVE_GCC:U0} >= 10:? -Wno-return-local-addr :}

#
# Clang warnings
#
CLANG_NO_ADDR_OF_PACKED_MEMBER=	${${ACTIVE_CC} == "clang" :? -Wno-error=address-of-packed-member :}

NO_ADDR_OF_PACKED_MEMBER=	${CLANG_NO_ADDR_OF_PACKED_MEMBER} ${GCC_NO_ADDR_OF_PACKED_MEMBER}

#
# Location of the file that contains the major and minor numbers of the
# version of a shared library.  If this file exists a shared library
# will be built by <bsd.lib.mk>.
#
SHLIB_VERSION_FILE?= ${.CURDIR}/shlib_version

#
# GNU sources and packages sometimes see architecture names differently.
#
GNU_ARCH.aarch64eb=aarch64_be
GNU_ARCH.earm=arm
GNU_ARCH.earmhf=arm
GNU_ARCH.earmeb=armeb
GNU_ARCH.earmhfeb=armeb
GNU_ARCH.earmv4=armv4
GNU_ARCH.earmv4eb=armv4eb
GNU_ARCH.earmv5=arm
GNU_ARCH.earmv5hf=arm
GNU_ARCH.earmv5eb=armeb
GNU_ARCH.earmv5hfeb=armeb
GNU_ARCH.earmv6=armv6
GNU_ARCH.earmv6hf=armv6
GNU_ARCH.earmv6eb=armv6eb
GNU_ARCH.earmv6hfeb=armv6eb
GNU_ARCH.earmv7=armv7
GNU_ARCH.earmv7hf=armv7
GNU_ARCH.earmv7eb=armv7eb
GNU_ARCH.earmv7hfeb=armv7eb
GNU_ARCH.i386=i486
GCC_CONFIG_ARCH.i386=i486
GCC_CONFIG_TUNE.i386=nocona
GCC_CONFIG_TUNE.x86_64=nocona
MACHINE_GNU_ARCH=${GNU_ARCH.${MACHINE_ARCH}:U${MACHINE_ARCH}}

#
# In order to identify NetBSD to GNU packages, we sometimes need
# an "elf" tag for historically a.out platforms.
#
.if (!empty(MACHINE_ARCH:Mearm*))
MACHINE_GNU_PLATFORM?=${MACHINE_GNU_ARCH}--netbsdelf-${MACHINE_ARCH:C/eb//:C/v[4-7]//:S/earm/eabi/}
.elif (${MACHINE_GNU_ARCH} == "arm" || 	\
	${MACHINE_GNU_ARCH} == "armeb" || 	\
	${MACHINE_ARCH} == "i386")
MACHINE_GNU_PLATFORM?=${MACHINE_GNU_ARCH}--netbsdelf
.else
MACHINE_GNU_PLATFORM?=${MACHINE_GNU_ARCH}--netbsd
.endif

.if !empty(MACHINE_ARCH:M*arm*)
# Flags to pass to CC for using the old APCS ABI on ARM for compat or stand.
ARM_APCS_FLAGS=	-mabi=apcs-gnu -mfloat-abi=soft -marm
ARM_APCS_FLAGS+= ${${ACTIVE_CC} == "gcc" && ${HAVE_GCC:U0} >= 8:? -mno-thumb-interwork :}
ARM_APCS_FLAGS+=${${ACTIVE_CC} == "clang":? -target ${MACHINE_GNU_ARCH}--netbsdelf -B ${TOOLDIR}/${MACHINE_GNU_PLATFORM}/bin :}
.endif

GENASSYM_CPPFLAGS+=	${${ACTIVE_CC} == "clang":? -no-integrated-as :}

TARGETS+=		all clean cleandir depend dependall includes 					\
				install lint obj regress tags html analyze describe 		
PHONY_NOTMAIN:		all clean cleandir depend dependall distclean includes 			\
			install lint obj regress beforedepend afterdepend 				\
			beforeinstall afterinstall realinstall realdepend realall 		\
			html subdir-all subdir-install subdir-depend analyze describe 	
.PHONY:		${PHONY_NOTMAIN}
.NOTMAIN:	${PHONY_NOTMAIN}

.if ${NEED_OWN_INSTALL_TARGET} != "no"
.if !target(install)
install:		beforeinstall .WAIT subdir-install realinstall .WAIT afterinstall
beforeinstall:
subdir-install:
realinstall:
afterinstall:
.endif
all:			realall subdir-all
subdir-all:		
realall:		
depend:			realdepend subdir-depend
subdir-depend:	
realdepend:		
distclean:		cleandir
cleandir:		clean

dependall:	.NOTMAIN realdepend .MAKE
	@cd "${.CURDIR}"; ${MAKE} realall
.endif

# Define MKxxx variables (which are either yes or no) for users
# to set in /etc/mk.conf and override on the make commandline.
# These should be tested with `== "no"' or `!= "no"'.
# The NOxxx variables should only be used by Makefiles.
#

#
# Supported NO* options (if defined, MK* will be forced to "no",
# regardless of user's mk.conf setting).
#
.for var in \
	COMPAT CRYPTO DEBUGLIB DOC HTML INFO LIBCSANITIZER LINKLIB \
	LINT MAN NLS OBJ PIC PICINSTALL PROFILE RELRO SANITIZER \
    SHARE STATICLIB 
.if defined(NO${var})
MK${var}:=	no
.endif
.endfor

#
# Older-style variables that enabled behaviour when set.
#
.for var in MANZ UNPRIVED UPDATE
.if defined(${var})
MK${var}:=	yes
.endif
.endfor


#
# MK* options which have variable defaults.
#
# aarch64eb is not yet supported.
#
.if ${MACHINE_ARCH} == "x86_64" || \
	(${MACHINE_ARCH} == "aarch64" && ${HAVE_GCC:U0} == 0) || \
	${MACHINE_ARCH} == "riscv64" || \
    !empty(MACHINE_ARCH:Mearm*)
MKCOMPAT?=	yes
.else
# Don't let this build where it really isn't supported.
MKCOMPAT:=	no
.endif

.if ${MKCOMPAT} == "no"
MKCOMPATTESTS:=	no
MKCOMPATX11:=	no
.endif

#
# These platforms always use softfloat.
#
#.if (${MACHINE_CPU} == "arm" && ${MACHINE_ARCH:M*hf*} == "")
#MKSOFTFLOAT=	yes
#.endif

#
# PIE is enabled on many platforms by default.
#
# Coverity does not like PIE
.if !defined(COVERITY_TOP_CONFIG) && \
    (${MACHINE_ARCH} == "i386" || \
    ${MACHINE_ARCH} == "x86_64" || \
    ${MACHINE_CPU} == "arm")
MKPIE?=		yes
.else
MKPIE?=		no
.endif

#
# RELRO is enabled on i386 and amd64 by default
#
.if ${MACHINE_ARCH} == "i386" || \
    ${MACHINE_ARCH} == "x86_64"
MKRELRO?=	partial
.else
MKRELRO?=	no
.endif

.if ${MACHINE_ARCH} == "x86_64" || ${MACHINE_ARCH} == "i386"
MKSTATICPIE?=	yes
.else
MKSTATICPIE?=	no
.endif

#
# Notes: MKCXX and MKLIBSTDCXX
# Would normally be placed in "_MKVARS.yes" options. 
# However, due to compiler issues have been placed
# in "_MKVARS.no".
# Please read TODO.md for issue.
#

#
# MK* options which default to "yes".
#
_MKVARS.yes= \
	MKBINUTILS \
	MKBSDTAR \
	MKCOMPLEX \
	MKCVS \
	MKDOC \
	MKDYNAMICROOT \
	MKGCC \
    	MKGDB \
    	MKGROFF \
    	MKHESIOD \
	MKHTML \
    	MKIEEEFP \
    	MKINET6 \
	MKINFO \
    	MKLINKLIB \
	MKMAN \
    	MKMANDOC \
    	MKMAKEMANDB \
	MKNLS \
	MKOBJ \
    	MKPF \
	MKPIC \
    	MKPICLIB \
    	MKPROFILE \
	MKSHARE \
	MKSKEY \
    	MKSTATICLIB \
    	MKYP
	
.for var in ${_MKVARS.yes}
${var}?=	${${var}.${MACHINE_ARCH}:Uyes}
.endfor

#
# MKGCCCMDS is only valid if we are building GCC so make it dependent on that.
#
_MKVARS.yes += MKGCCCMDS
MKGCCCMDS?=	${MKGCC}

#
# Sanitizers, only "address" and "undefined" are supported by gcc
#
MKSANITIZER?=		no
USE_SANITIZER?=		address

#
# Sanitizers implemented in libc, only "undefined" is supported
#
MKLIBCSANITIZER?=	no
USE_LIBCSANITIZER?=	undefined

.if defined(MKREPRO)
MKARZERO ?= ${MKREPRO}
GROFF_FLAGS ?= -dpaper=letter
ROFF_PAGESIZE ?= -P-pletter
.endif

#
# Install the kernel as /netbsd/kernel and the modules in /netbsd/modules
#
KERNEL_DIR?=	no

#
# MK* options which default to "no".
#
_MKVARS.no= \
    	MKATF   \
	MKARGON2 \
	MKARZERO \
	MKBSDGREP \
	MKCATPAGES \
    	MKCOMPATTESTS \
    	MKCOMPATX11 \
    	MKCTF \
	MKDEBUG \
    	MKDEBUGLIB \
    	MKDTC \
    	MKDTB \
    	MKDTRACE \
	MKEXTSRC \
	MKFIRMWARE \
	MKGROFFHTMLDOC \
    	MKISCSI \
	MKKYUA \
	MKLIBCXX \
    	MKLLD \
    	MKLLDB \
    	MKLLVM \
    	MKLLVMRT \
    	MKLINT \
    	MKLVM \
	MKMANZ \
    	MKMCLINKER \
    	MKMDNS \
    	MKKMOD \
	MKNOUVEAUFIRMWARE \
	MKNPF \
    	MKNSD \
	MKOBJDIRS \
	MKPCC \
    	MKPERFUSE \
    	MKPICINSTALL \
    	MKPIGZGZIP \
	MKRADEONFIRMWARE \
    	MKREPRO \
    	MKRUMP \
	MKSLJIT \
    	MKSOFTFLOAT \
    	MKSTRIPIDENT \
	MKTEGRAFIRMWARE \
    	MKTPM \
    	MKUNPRIVED \
    	MKUPDATE \
	MKX11 \
    	MKX11FONTS \
    	MKX11MOTIF \
    	MKXORG_SERVER \
	MKZFS \
	MKLDAP \
	MKKERBEROS \
	MKPAM \
	MKPOSTFIX \
	MKUNBOUND \
	MKCXX \
	MKLIBSTDCXX
	
.for var in ${_MKVARS.no}
${var}?=	${${var}.${MACHINE_ARCH}:U${${var}.${MACHINE}:Uno}}
.endfor

#
# MKXZ is optional.
# Note: Compile time linker issues with memmove and libpthreads.
#
MKXZ:= no

#
# Force some options off if their dependencies are off.
#

.if ${MKCXX} == "no"
MKATF:=			no
MKGCCCMDS:=		no
MKGDB:=			no
MKGROFF:=		no
MKKYUA:=		no
.endif

.if ${MKMAN} == "no"
MKCATPAGES:=	no
MKHTML:=		no
.endif

_MANINSTALL=	maninstall
.if ${MKCATPAGES} != "no"
_MANINSTALL+=	catinstall
.endif
.if ${MKHTML} != "no"
_MANINSTALL+=	htmlinstall
.endif

.if ${MKLINKLIB} == "no"
MKLINT:=		no
MKPICINSTALL:=	no
MKPROFILE:=		no
.endif

.if ${MKPIC} == "no"
MKPICLIB:=		no
.endif

.if ${MKOBJ} == "no"
MKOBJDIRS:=		no
.endif

.if ${MKSHARE} == "no"
MKCATPAGES:=		no
MKDOC:=			no
MKINFO:=		no
MKHTML:=		no
MKMAN:=			no
MKNLS:=			no
.endif

.if !empty(MACHINE_ARCH:Mearm*)
_NEEDS_LIBCXX.${MACHINE_ARCH}=	yes
.endif
_NEEDS_LIBCXX.aarch64=		yes
_NEEDS_LIBCXX.aarch64eb=	yes
_NEEDS_LIBCXX.i386=			yes
_NEEDS_LIBCXX.x86_64=		yes

.if ${MKLLVM} == "yes" && ${_NEEDS_LIBCXX.${MACHINE_ARCH}:Uno} == "yes"
MKLIBCXX:=		yes
.endif

#
# Disable MKSTRIPSYM if MKDEBUG is enabled.
#
.if ${MKDEBUG} != "no"
MKSTRIPSYM:=	no
.endif

#
# install(1) parameters.
#
COPY?=		-c
.if ${MKUPDATE} == "no"
PRESERVE?=	
.else
PRESERVE?=	-p
.endif
RENAME?=	-r
HRDLINK?=	-l h
SYMLINK?=	-l s

METALOG?=	${DESTDIR}/METALOG
METALOG.add?=	${TOOL_CAT} -l >> ${METALOG}
.if (${_SRC_TOP_} != "")	# only set INSTPRIV if inside ${NETBSDSRCDIR}
.if ${MKUNPRIVED} != "no"
INSTPRIV.unpriv=-U -M ${METALOG} -D ${DESTDIR} -h sha256
.else
INSTPRIV.unpriv=
.endif
INSTPRIV?=		${INSTPRIV.unpriv} -N ${NETBSDSRCDIR}/etc
.endif
STRIPFLAG?=	

.if ${NEED_OWN_INSTALL_TARGET} != "no"
INSTALL_DIR?=		${INSTALL} ${INSTPRIV} -d
INSTALL_FILE?=		${INSTALL} ${INSTPRIV} ${COPY} ${PRESERVE} ${RENAME}
INSTALL_LINK?=		${INSTALL} ${INSTPRIV} ${HRDLINK} ${RENAME}
INSTALL_SYMLINK?=	${INSTALL} ${INSTPRIV} ${SYMLINK} ${RENAME}
.endif

# for crunchide & ldd, define the OBJECT_FMTS used by a MACHINE_ARCH
#
OBJECT_FMTS=
.if	${MACHINE_ARCH} != "alpha" && ${MACHINE_ARCH} != "ia64"
OBJECT_FMTS+=	elf32
.endif
.if	${MACHINE_ARCH} == "alpha" || ${MACHINE_ARCH:M*64*} != ""
. if !(${MKCOMPAT:Uyes} == "no" && ${MACHINE_CPU} == "mips")
OBJECT_FMTS+=	elf64
. endif
.endif

#
# Set defaults for the USE_xxx variables.
#

#
# USE_* options which default to "no" and will be forced to "no" if their
# corresponding MK* variable is set to "no".
#
.for var in USE_SKEY
.if (${${var:S/USE_/MK/}} == "no")
${var}:= no
.else
${var}?= no
.endif
.endfor

#
# USE_* options which default to "yes" unless their corresponding MK*
# variable is set to "no".
#
.for var in USE_HESIOD USE_INET6 USE_KERBEROS USE_LDAP USE_PAM USE_YP
.if (${${var:S/USE_/MK/}} == "no")
${var}:= no
.else
${var}?= yes
.endif
.endfor

#
# USE_* options which default to "yes".
#
.for var in LIBSTDCXX
USE_${var}?= yes
.endfor

#
# libc USE_* options which default to "yes".
#
.for var in JEMALLOC
USE_${var}?= no
.endfor

#
# libc USE_* options which default to "no".
#
.for var in ARC4 SGTTY NDBM
USE_${var}?= no
.endfor

#
# USE_* options which default to "no".
#
# For now, disable pigz as compressor by default
.for var in PIGZGZIP
USE_${var}?= no
.endfor

# Default to USE_XZ_SETS on some 64bit architectures where decompressor
# memory will likely not be in short supply.
# Since pigz can not create .xz format files currently, disable .xz
# format if USE_PIGZGZIP is enabled.
.if ${USE_PIGZGZIP} == "no" && (${MACHINE} == "amd64")
USE_XZ_SETS?= yes
.else
USE_XZ_SETS?= no
.endif 

#
# TOOL_GZIP and friends.  These might refer to TOOL_PIGZ or to the host gzip.
#
.if ${USE_PIGZGZIP} != "no"
TOOL_GZIP=		${TOOL_PIGZ}
GZIP_N_FLAG?=	-nT
.else
TOOL_GZIP=		gzip
GZIP_N_FLAG?=	-n
.endif
TOOL_GZIP_N=	${TOOL_GZIP} ${GZIP_N_FLAG}

#
# MAKEDIRTARGET dir target [extra make(1) params]
#	run "cd $${dir} && ${MAKEDIRTARGETENV} ${MAKE} [params] $${target}", with a pretty message
#
MAKEDIRTARGETENV?=
MAKEDIRTARGET=\
	@_makedirtarget() { \
		dir="$$1"; shift; \
		target="$$1"; shift; \
		case "$${dir}" in \
		/*)	this="$${dir}/"; \
			real="$${dir}" ;; \
		.)	this="${_THISDIR_}"; \
			real="${.CURDIR}" ;; \
		*)	this="${_THISDIR_}$${dir}/"; \
			real="${.CURDIR}/$${dir}" ;; \
		esac; \
		show=$${this:-.}; \
		echo "$${target} ===> $${show%/}$${1:+	(with: $$@)}"; \
		cd "$${real}" \
		&& ${MAKEDIRTARGETENV} ${MAKE} _THISDIR_="$${this}" "$$@" $${target}; \
	}; \
	_makedirtarget
	
#
# MAKEVERBOSE support.  Levels are:
#	0	Minimal output ("quiet")
#	1	Describe what is occurring
#	2	Describe what is occurring and echo the actual command
#	3	Ignore the effect of the "@" prefix in make commands
#	4	Trace shell commands using the shell's -x flag
#		
MAKEVERBOSE?=		2

.if ${MAKEVERBOSE} == 0
_MKMSG?=	@\#
_MKSHMSG?=	: echo
_MKSHECHO?=	: echo
.SILENT:
.elif ${MAKEVERBOSE} == 1
_MKMSG?=	@echo '   '
_MKSHMSG?=	echo '   '
_MKSHECHO?=	: echo
.SILENT:
.else	# MAKEVERBOSE == 2 ?
_MKMSG?=	@echo '\#  '
_MKSHMSG?=	echo '\#  '
_MKSHECHO?=	echo
.SILENT: __makeverbose_dummy_target__
.endif	# MAKEVERBOSE >= 2
.if ${MAKEVERBOSE} >= 3
.MAKEFLAGS:	-dl
.endif	# ${MAKEVERBOSE} >= 3
.if ${MAKEVERBOSE} >= 4
.MAKEFLAGS:	-dx
.endif	# ${MAKEVERBOSE} >= 4

_MKMSG_BUILD?=		${_MKMSG} "  build "
_MKMSG_CREATE?=		${_MKMSG} " create "
_MKMSG_COMPILE?=	${_MKMSG} "compile "
_MKMSG_EXECUTE?=	${_MKMSG} "execute "
_MKMSG_FORMAT?=		${_MKMSG} " format "
_MKMSG_INSTALL?=	${_MKMSG} "install "
_MKMSG_LINK?=		${_MKMSG} "   link "
_MKMSG_LEX?=		${_MKMSG} "    lex "
_MKMSG_REMOVE?=		${_MKMSG} " remove "
_MKMSG_REGEN?=		${_MKMSG} "  regen "
_MKMSG_YACC?=		${_MKMSG} "   yacc "

_MKSHMSG_CREATE?=	${_MKSHMSG} " create "
_MKSHMSG_FORMAT?=	${_MKSHMSG} " format "
_MKSHMSG_INSTALL?=	${_MKSHMSG} "install "

_MKTARGET_BUILD?=	${_MKMSG_BUILD} ${.CURDIR:T}/${.TARGET}
_MKTARGET_CREATE?=	${_MKMSG_CREATE} ${.CURDIR:T}/${.TARGET}
_MKTARGET_COMPILE?=	${_MKMSG_COMPILE} ${.CURDIR:T}/${.TARGET}
_MKTARGET_FORMAT?=	${_MKMSG_FORMAT} ${.CURDIR:T}/${.TARGET}
_MKTARGET_INSTALL?=	${_MKMSG_INSTALL} ${.TARGET}
_MKTARGET_LINK?=	${_MKMSG_LINK} ${.CURDIR:T}/${.TARGET}
_MKTARGET_LEX?=		${_MKMSG_LEX} ${.CURDIR:T}/${.TARGET}
_MKTARGET_REMOVE?=	${_MKMSG_REMOVE} ${.TARGET}
_MKTARGET_YACC?=	${_MKMSG_YACC} ${.CURDIR:T}/${.TARGET}

.if ${MKMANDOC} == "yes"
TARGETS+=	lintmanpages
.endif

.endif	# !defined(_BSD_OWN_MK_)
