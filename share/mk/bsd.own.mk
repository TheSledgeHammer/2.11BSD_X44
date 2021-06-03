#	$NetBSD: bsd.own.mk,v 1.1247 2021/05/06 13:23:36 rin Exp $
#  
#	Notes: bsd.own.mk only defines parameters related to the toolchain,
#	machine archictechure and compiler.
#

# This needs to be before bsd.init.mk

.if !defined(_BSD_OWN_MK_)
_BSD_OWN_MK_=1

MAKECONF?=	/etc/mk.conf
.-include "${MAKECONF}"

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
    ${MACHINE_ARCH} == "i386" || \
    ${MACHINE_ARCH} == "x86_64" || \
    ${MACHINE_CPU} == "aarch64" || \
    ${MACHINE_CPU} == "mips" || \
    ${MACHINE_CPU} == "powerpc" || \
    ${MACHINE_CPU} == "riscv"
HAVE_GCC?=	10
.else
HAVE_GCC?=	9
.endif

#
# We import the old gcc as "gcc.old" when upgrading.  EXTERNAL_GCC_SUBDIR is
# set to the relevant subdirectory in src/external/gpl3 for his HAVE_GCC.
#
HAVE_GCC?=			10
EXTERNAL_GCC_SUBDIR?=		gcc
.else
EXTERNAL_GCC_SUBDIR?=		/does/not/exist
.endif

#
# What binutils is used?
#
HAVE_BINUTILS?=			234
.if ${HAVE_BINUTILS} == 	234
EXTERNAL_BINUTILS_SUBDIR=	binutils
.else
EXTERNAL_BINUTILS_SUBDIR=	/does/not/exist
.endif

#
# What GDB is used?
#
HAVE_GDB?=			1100
.if ${HAVE_GDB} == 		1100
EXTERNAL_GDB_SUBDIR=		gdb
.else
EXTERNAL_GDB_SUBDIR=		/does/not/exist
.endif

.if empty(.MAKEFLAGS:M-V*)
PRINTOBJDIR=	${MAKE} -V .OBJDIR
.else
PRINTOBJDIR=	echo # prevent infinite recursion
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
AR=			${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-ar
AS=			${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-as
LD=			${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-ld
NM=			${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-nm
OBJCOPY=		${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-objcopy
OBJDUMP=		${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-objdump
RANLIB=			${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-ranlib
READELF=		${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-readelf
SIZE=			${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-size
STRINGS=		${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-strings
STRIP=			${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-strip

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
AR=			${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-ar
AS=			${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-as
LD=			${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-ld
NM=			${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-nm
OBJCOPY=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-objcopy
OBJDUMP=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-objdump
RANLIB=			${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-ranlib
READELF=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-readelf
SIZE=			${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-size
STRINGS=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-strings
STRIP=			${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-strip

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

DBSYM=			${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-dbsym
INSTALL=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-install
LEX=			${TOOLDIR}/bin/${_TOOL_PREFIX}lex
LINT=			CC=${CC:Q} ${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-lint
LORDER=			NM=${NM:Q} MKTEMP=${TOOL_MKTEMP:Q} ${TOOLDIR}/bin/${_TOOL_PREFIX}lorder
MKDEP=			CC=${CC:Q} ${TOOLDIR}/bin/${_TOOL_PREFIX}mkdep
MKDEPCXX=		CC=${CXX:Q} ${TOOLDIR}/bin/${_TOOL_PREFIX}mkdep
PAXCTL=			${TOOLDIR}/bin/${_TOOL_PREFIX}paxctl
TSORT=			${TOOLDIR}/bin/${_TOOL_PREFIX}tsort -q
YACC=			${TOOLDIR}/bin/${_TOOL_PREFIX}yacc

TOOL_AWK=		${TOOLDIR}/bin/${_TOOL_PREFIX}awk
TOOL_CAP_MKDB=		${TOOLDIR}/bin/${_TOOL_PREFIX}cap_mkdb
TOOL_CAT=		${TOOLDIR}/bin/${_TOOL_PREFIX}cat
TOOL_CKSUM=		${TOOLDIR}/bin/${_TOOL_PREFIX}cksum

TOOL_COMPILE_ET=	${TOOLDIR}/bin/${_TOOL_PREFIX}compile_et
TOOL_CONFIG=		${TOOLDIR}/bin/${_TOOL_PREFIX}config
TOOL_CRUNCHGEN=		MAKE=${.MAKE:Q} ${TOOLDIR}/bin/${_TOOL_PREFIX}crunchgen
TOOL_CTAGS=		${TOOLDIR}/bin/${_TOOL_PREFIX}ctags
TOOL_CTFCONVERT=	${TOOLDIR}/bin/${_TOOL_PREFIX}ctfconvert
TOOL_CTFMERGE=		${TOOLDIR}/bin/${_TOOL_PREFIX}ctfmerge
TOOL_CVSLATEST=		${TOOLDIR}/bin/${_TOOL_PREFIX}cvslatest
TOOL_DB=		${TOOLDIR}/bin/${_TOOL_PREFIX}db
TOOL_DISKLABEL=		${TOOLDIR}/bin/${_TOOL_PREFIX}disklabel
TOOL_DTC=		${TOOLDIR}/bin/${_TOOL_PREFIX}dtc
TOOL_EQN=		${TOOLDIR}/bin/${_TOOL_PREFIX}eqn
TOOL_FDISK=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-fdisk
TOOL_FGEN=		${TOOLDIR}/bin/${_TOOL_PREFIX}fgen
TOOL_GENASSYM=		${TOOLDIR}/bin/${_TOOL_PREFIX}genassym
TOOL_GENCAT=		${TOOLDIR}/bin/${_TOOL_PREFIX}gencat
TOOL_GMAKE=		${TOOLDIR}/bin/${_TOOL_PREFIX}gmake
TOOL_GPT=		${TOOLDIR}/bin/${_TOOL_PREFIX}gpt
TOOL_GREP=		${TOOLDIR}/bin/${_TOOL_PREFIX}grep
GROFF_SHARE_PATH=	${TOOLDIR}/share/groff
TOOL_GROFF_ENV= 															\
    GROFF_ENCODING= 														\
    GROFF_BIN_PATH=${TOOLDIR}/lib/groff 									\
    GROFF_FONT_PATH=${GROFF_SHARE_PATH}/site-font:${GROFF_SHARE_PATH}/font 	\
    GROFF_TMAC_PATH=${GROFF_SHARE_PATH}/site-tmac:${GROFF_SHARE_PATH}/tmac
TOOL_GROFF=		${TOOL_GROFF_ENV} ${TOOLDIR}/bin/${_TOOL_PREFIX}groff ${GROFF_FLAGS}

TOOL_HEXDUMP=		${TOOLDIR}/bin/${_TOOL_PREFIX}hexdump
TOOL_INDXBIB=		${TOOLDIR}/bin/${_TOOL_PREFIX}indxbib
TOOL_INSTALLBOOT=	${TOOLDIR}/bin/${_TOOL_PREFIX}installboot
TOOL_INSTALL_INFO=	${TOOLDIR}/bin/${_TOOL_PREFIX}install-info
TOOL_JOIN=		${TOOLDIR}/bin/${_TOOL_PREFIX}join
TOOL_LLVM_TBLGEN=	${TOOLDIR}/bin/${_TOOL_PREFIX}llvm-tblgen
TOOL_M4=		${TOOLDIR}/bin/${_TOOL_PREFIX}m4
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
TOOL_MENUC=		MENUDEF=${TOOLDIR}/share/misc ${TOOLDIR}/bin/${_TOOL_PREFIX}menuc
TOOL_MKCSMAPPER=	${TOOLDIR}/bin/${_TOOL_PREFIX}mkcsmapper
TOOL_MKESDB=		${TOOLDIR}/bin/${_TOOL_PREFIX}mkesdb
TOOL_MKLOCALE=		${TOOLDIR}/bin/${_TOOL_PREFIX}mklocale
TOOL_MKMAGIC=		${TOOLDIR}/bin/${_TOOL_PREFIX}file
TOOL_MKNOD=		${TOOLDIR}/bin/${_TOOL_PREFIX}mknod
TOOL_MKTEMP=		${TOOLDIR}/bin/${_TOOL_PREFIX}mktemp
TOOL_MKUBOOTIMAGE=	${TOOLDIR}/bin/${_TOOL_PREFIX}mkubootimage
TOOL_ELFTOSB=		${TOOLDIR}/bin/${_TOOL_PREFIX}elftosb
TOOL_MSGC=		MSGDEF=${TOOLDIR}/share/misc ${TOOLDIR}/bin/${_TOOL_PREFIX}msgc
TOOL_MTREE=		${TOOLDIR}/bin/${_TOOL_PREFIX}mtree
TOOL_NBPERF=		${TOOLDIR}/bin/${_TOOL_PREFIX}perf
TOOL_NCDCS=		${TOOLDIR}/bin/${_TOOL_PREFIX}ibmnws-ncdcs
TOOL_PAX=		${TOOLDIR}/bin/${_TOOL_PREFIX}pax
TOOL_PIC=		${TOOLDIR}/bin/${_TOOL_PREFIX}pic
TOOL_PIGZ=		${TOOLDIR}/bin/${_TOOL_PREFIX}pigz
TOOL_XZ=		${TOOLDIR}/bin/${_TOOL_PREFIX}xz
TOOL_PKG_CREATE=	${TOOLDIR}/bin/${_TOOL_PREFIX}pkg_create
TOOL_PWD_MKDB=		${TOOLDIR}/bin/${_TOOL_PREFIX}pwd_mkdb
TOOL_REFER=		${TOOLDIR}/bin/${_TOOL_PREFIX}refer
TOOL_ROFF_ASCII=	${TOOL_GROFF_ENV} ${TOOLDIR}/bin/${_TOOL_PREFIX}nroff
TOOL_ROFF_DOCASCII=	${TOOL_GROFF} -Tascii
TOOL_ROFF_DOCHTML=	${TOOL_GROFF} -Thtml
TOOL_ROFF_DVI=		${TOOL_GROFF} -Tdvi ${ROFF_PAGESIZE}
TOOL_ROFF_HTML=		${TOOL_GROFF} -Tlatin1 -mdoc2html
TOOL_ROFF_PS=		${TOOL_GROFF} -Tps ${ROFF_PAGESIZE}
TOOL_ROFF_RAW=		${TOOL_GROFF} -Z
TOOL_RPCGEN=		RPCGEN_CPP=${CPP:Q} ${TOOLDIR}/bin/${_TOOL_PREFIX}rpcgen
TOOL_SED=		${TOOLDIR}/bin/${_TOOL_PREFIX}sed
TOOL_SOELIM=		${TOOLDIR}/bin/${_TOOL_PREFIX}soelim
TOOL_SORTINFO=		${TOOLDIR}/bin/${_TOOL_PREFIX}sortinfo
TOOL_STAT=		${TOOLDIR}/bin/${_TOOL_PREFIX}stat
TOOL_TIC=		${TOOLDIR}/bin/${_TOOL_PREFIX}tic
TOOL_UUDECODE=		${TOOLDIR}/bin/${_TOOL_PREFIX}uudecode
TOOL_VGRIND=		${TOOLDIR}/bin/${_TOOL_PREFIX}vgrind -f
TOOL_VFONTEDPR=		${TOOLDIR}/libexec/${_TOOL_PREFIX}vfontedpr
TOOL_ZIC=		${TOOLDIR}/bin/${_TOOL_PREFIX}zic

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

TOOL_ASN1_COMPILE=	asn1_compile
TOOL_AWK=		awk
TOOL_CAP_MKDB=		cap_mkdb
TOOL_CAT=		cat
TOOL_CKSUM=		cksum
TOOL_CLANG_TBLGEN=	clang-tblgen
TOOL_COMPILE_ET=	compile_et
TOOL_CONFIG=		config
TOOL_CRUNCHGEN=		crunchgen
TOOL_CTAGS=		ctags
TOOL_CTFCONVERT=	ctfconvert
TOOL_CTFMERGE=		ctfmerge
TOOL_CVSLATEST=		cvslatest
TOOL_DATE=		date
TOOL_DB=		db
TOOL_DISKLABEL=		disklabel
TOOL_DTC=		dtc
TOOL_EQN=		eqn
TOOL_FDISK=		fdisk
TOOL_FGEN=		fgen
TOOL_GENASSYM=		genassym
TOOL_GENCAT=		gencat
TOOL_GMAKE=		gmake
TOOL_GPT=		gpt
TOOL_GREP=		grep
TOOL_GROFF=		groff
TOOL_HEXDUMP=		hexdump
TOOL_HP300MKBOOT=	hp300-mkboot
TOOL_HPPAMKBOOT=	hppa-mkboot
TOOL_INDXBIB=		indxbib
TOOL_INSTALLBOOT=	installboot
TOOL_INSTALL_INFO=	install-info
TOOL_JOIN=		join
TOOL_LLVM_TBLGEN=	llvm-tblgen
TOOL_M4=		m4
TOOL_MACPPCFIXCOFF=	macppc-fixcoff
TOOL_MAKEFS=		makefs
TOOL_MAKEINFO=		makeinfo
TOOL_MAKEKEYS=		makekeys
TOOL_MAKESTRS=		makestrs
TOOL_MAKEWHATIS=	/usr/libexec/makewhatis
TOOL_MANDOC_ASCII=	mandoc -Tascii
TOOL_MANDOC_HTML=	mandoc -Thtml
TOOL_MANDOC_LINT=	mandoc -Tlint
TOOL_MDSETIMAGE=	mdsetimage
TOOL_MENUC=		menuc
TOOL_ARMELF2AOUT=	arm-elf2aout
TOOL_M68KELF2AOUT=	m68k-elf2aout
TOOL_MIPSELF2ECOFF=	mips-elf2ecoff
TOOL_MKCSMAPPER=	mkcsmapper
TOOL_MKESDB=		mkesdb
TOOL_MKLOCALE=		mklocale
TOOL_MKMAGIC=		file
TOOL_MKNOD=		mknod
TOOL_MKTEMP=		mktemp
TOOL_MKUBOOTIMAGE=	mkubootimage
TOOL_ELFTOSB=		elftosb
TOOL_MSGC=		msgc
TOOL_MTREE=		mtree
TOOL_MVME68KWRTVID=	wrtvid
TOOL_NBPERF=		nbperf
TOOL_NCDCS=		ncdcs
TOOL_PAX=		pax
TOOL_PIC=		pic
TOOL_PIGZ=		pigz
TOOL_XZ=		xz
TOOL_PKG_CREATE=	pkg_create
TOOL_POWERPCMKBOOTIMAGE=powerpc-mkbootimage
TOOL_PWD_MKDB=		pwd_mkdb
TOOL_REFER=		refer
TOOL_ROFF_ASCII=	nroff
TOOL_ROFF_DOCASCII=	${TOOL_GROFF} -Tascii
TOOL_ROFF_DOCHTML=	${TOOL_GROFF} -Thtml
TOOL_ROFF_DVI=		${TOOL_GROFF} -Tdvi ${ROFF_PAGESIZE}
TOOL_ROFF_HTML=		${TOOL_GROFF} -Tlatin1 -mdoc2html
TOOL_ROFF_PS=		${TOOL_GROFF} -Tps ${ROFF_PAGESIZE}
TOOL_ROFF_RAW=		${TOOL_GROFF} -Z
TOOL_RPCGEN=		rpcgen
TOOL_SED=		sed
TOOL_SOELIM=		soelim
TOOL_SORTINFO=		sortinfo
TOOL_SPARKCRC=		sparkcrc
TOOL_STAT=		stat
TOOL_STRFILE=		strfile
TOOL_SUNLABEL=		sunlabel
TOOL_TBL=		tbl
TOOL_TIC=		tic
TOOL_UUDECODE=		uudecode
TOOL_VGRIND=		vgrind -f
TOOL_VFONTEDPR=		/usr/libexec/vfontedpr
TOOL_ZIC=		zic

.endif	# USETOOLS != yes						# }

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


.if ${USETOOLS} == "yes"						# {
#
# Make sure DESTDIR is set, so that builds with these tools always
# get appropriate -nostdinc, -nostdlib, etc. handling.  The default is
# <empty string>, meaning start from /, the root directory.
#
DESTDIR?=
.endif									# }

#
# Build a dynamically linked /bin and /sbin, with the necessary shared
# libraries moved from /usr/lib to /lib and the shared linker moved
# from /usr/libexec to /lib
#
# Note that if the BINDIR is not /bin or /sbin, then we always use the
# non-DYNAMICROOT behavior (i.e. it is only enabled for programs in /bin
# and /sbin).  See <bsd.shlib.mk>.
#
MKDYNAMICROOT?=	yes

# where the system object and source trees are kept; can be configurable
# by the user in case they want them in ~/foosrc and ~/fooobj, for example
BSDSRCDIR?=		/usr/src
BSDOBJDIR?=		/usr/obj
NETBSDSRCDIR?=		${BSDSRCDIR}

BINGRP?=		wheel
BINOWN?=		root
BINMODE?=		555
NONBINMODE?=		444
DIRMODE?=		755

SHAREDIR?=		/usr/share
SHAREGRP?=		wheel
SHAREOWN?=		root
SHAREMODE?=		${NONBINMODE}

MANDIR?=		/usr/share/man
MANGRP?=		wheel
MANOWN?=		root
MANMODE?=		${NONBINMODE}
MANINSTALL?=		maninstall catinstall

LIBDIR?=		/usr/lib
LINTLIBDIR?=		/usr/libdata/lint
LIBGRP?=		${BINGRP}
LIBOWN?=		${BINOWN}
LIBMODE?=		${NONBINMODE}

DOCDIR?=    		/usr/share/doc
DOCGRP?=		wheel
DOCOWN?=		root
DOCMODE?=   		${NONBINMODE}

NLSDIR?=		/usr/share/nls
NLSGRP?=		wheel
NLSOWN?=		root
NLSMODE?=		${NONBINMODE}

LOCALEDIR?=		/usr/share/locale
LOCALEGRP?=		wheel
LOCALEOWN?=		root
LOCALEMODE?=		${NONBINMODE}

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
GCC_CONFIG_TUNE.x86_64=nocona
MACHINE_GNU_ARCH=${GNU_ARCH.${MACHINE_ARCH}:U${MACHINE_ARCH}}

#
# In order to identify NetBSD to GNU packages, we sometimes need
# an "elf" tag for historically a.out platforms.
#
.if ${OBJECT_FMT} == "ELF" && (${MACHINE_ARCH} == "i386")
MACHINE_GNU_PLATFORM?=${MACHINE_GNU_ARCH}--netbsdelf
.else
MACHINE_GNU_PLATFORM?=${MACHINE_GNU_ARCH}--netbsd
.endif

TARGETS+=	all clean cleandir depend dependall includes 			\
		install lint obj regress tags html analyze describe 		\
		rumpdescribe
PHONY_NOTMAIN =	all clean cleandir depend dependall distclean includes 		\
		install lint obj regress beforedepend afterdepend 		\
		beforeinstall afterinstall realinstall realdepend realall 	\
		html subdir-all subdir-install subdir-depend analyze describe 	\
		rumpdescribe
.PHONY:		${PHONY_NOTMAIN}
.NOTMAIN:	${PHONY_NOTMAIN}

.if ${NEED_OWN_INSTALL_TARGET} != "no"
.if !target(install)
install:	beforeinstall .WAIT subdir-install realinstall .WAIT afterinstall
beforeinstall:
subdir-install:
realinstall:
afterinstall:
.endif
all:		realall subdir-all
subdir-all:
realall:
depend:		realdepend subdir-depend
subdir-depend:
realdepend:
distclean:	cleandir
cleandir:	clean

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
.for var in 										\
	CRYPTO DOC HTML LIBCSANITIZER LINKLIB LINT MAN NLS OBJ PIC PICINSTALL PROFILE 	\
	SHARE STATICLIB DEBUGLIB SANITIZER RELRO
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
# MK* options which default to "yes".
#
.for var in 					\
	BINUTILS BSDTAR				\
	CATPAGES CXX 				\
	DOC 					\
	GCC GDB GROFF 				\
	HTML 					\
	IEEEFP INET6 INFO 			\
	KERBEROS KERBEROS4 			\
	LIBSTDCXX LINKLIB LINT 			\
	MAN MANDOC 				\
	NLS NPF 				\
	OBJ 					\
	PIC PICINSTALL PICLIB POSTFIX PROFILE 	\
	SENDMAIL SHARE STATICLIB 		\
	UNBOUND  				\
	YP
MK${var}?=	yes
.endfor

#
# MK* options which default to "no".
#
.for var in 					\
	ARZERO					\
	BSDGREP					\
	CATPAGES				\
	DEBUG DEBUGLIB 				\
	KYUA					\
	LIBCXX LLD LLDB LLVM LLVMRT LINT 	\
	MANZ 					\
	OBJDIRS 				\
	PCC PICINSTALL				\
	REPRO					\
	SOFTFLOAT STRIPIDENT 			\
	UNPRIVED UPDATE
MK${var}?=	no
.endfor

#
# Force some options off if their dependencies are off.
#

.if ${MKCXX} == "no"
MKATF:=		no
MKGCCCMDS:=	no
MKGDB:=		no
MKGROFF:=	no
MKKYUA:=	no
.endif

.if ${MKMAN} == "no"
MKCATPAGES:=	no
MKHTML:=	no
.endif

.if ${MKLINKLIB} == "no"
MKLINT:=	no
MKPICINSTALL:=	no
MKPROFILE:=	no
.endif

.if ${MKPIC} == "no"
MKPICLIB:=	no
.endif

.if ${MKOBJ} == "no"
MKOBJDIRS:=	no
.endif

.if ${MKSHARE} == "no"
MKCATPAGES:=	no
MKDOC:=		no
MKINFO:=	no
MKHTML:=	no
MKMAN:=		no
MKNLS:=		no
.endif

_NEEDS_LIBCXX.i386=		yes

.if ${MKLLVM} == "yes" && ${_NEEDS_LIBCXX.${MACHINE_ARCH}:Uno} == "yes"
MKLIBCXX:=	yes
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
INSTPRIV.unpriv=-U -M ${METALOG} -D ${DESTDIR} -h sha1
.else
INSTPRIV.unpriv=
.endif
INSTPRIV?=		${INSTPRIV.unpriv} -N ${NETBSDSRCDIR}/etc
.endif
SYSPKGTAG?=		${SYSPKG:D-T ${SYSPKG}_pkg}
SYSPKGDOCTAG?=		${SYSPKG:D-T ${SYSPKG}-doc_pkg}
STRIPFLAG?=	

.if ${NEED_OWN_INSTALL_TARGET} != "no"
INSTALL_DIR?=		${INSTALL} ${INSTPRIV} -d
INSTALL_FILE?=		${INSTALL} ${INSTPRIV} ${COPY} ${PRESERVE} ${RENAME}
INSTALL_LINK?=		${INSTALL} ${INSTPRIV} ${HRDLINK} ${RENAME}
INSTALL_SYMLINK?=	${INSTALL} ${INSTPRIV} ${SYMLINK} ${RENAME}
HOST_INSTALL_FILE?=	${INSTALL} ${COPY} ${PRESERVE} ${RENAME}
.endif

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
_MKMSG_FORMAT?=		${_MKMSG} " format "
_MKMSG_INSTALL?=	${_MKMSG} "install "
_MKMSG_LINK?=		${_MKMSG} "   link "
_MKMSG_LEX?=		${_MKMSG} "    lex "
_MKMSG_REMOVE?=		${_MKMSG} " remove "
_MKMSG_YACC?=		${_MKMSG} "   yacc "

_MKSHMSG_CREATE?=	${_MKSHMSG} " create "
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

.endif	# !defined(_BSD_OWN_MK_)
