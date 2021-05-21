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
# set Compiler, gcc or clang. GCC is default
#
USE_GCC_COMPILER=	 yes
USE_CLANG_COMPILER=  no

#
# CPU model, derived from MACHINE_ARCH
#
MACHINE_CPU=		${MACHINE_ARCH:C/}

# For each ${MACHINE_CPU}, list the ports that use it.
MACHINES.i386=		i386
MACHINES.x86_64=	amd64

#
# GNU sources and packages sometimes see architecture names differently.
#
GNU_ARCH.i386=i486
GCC_CONFIG_ARCH.i386=i486
GCC_CONFIG_TUNE.i386=nocona
GCC_CONFIG_TUNE.x86_64=nocona
MACHINE_GNU_ARCH=${GNU_ARCH.${MACHINE_ARCH}:U${MACHINE_ARCH}}
#
# Subdirectory used below ${RELEASEDIR} when building a release
#
.if !empty(MACHINE:Mevbarm) || !empty(MACHINE:Mevbmips) \
	|| !empty(MACHINE:Mevbsh3)
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
.if ${MACHINE_ARCH} == "x86_64"
HAVE_GCC?=	10
.else
HAVE_GCC?=	9
.endif

#
# What binutils is used?
#
HAVE_BINUTILS?=	234
.if ${HAVE_BINUTILS} == 234
EXTERNAL_BINUTILS_SUBDIR=	binutils
.else
EXTERNAL_BINUTILS_SUBDIR=	/does/not/exist
.endif

#
# What GDB is used?
#
HAVE_GDB?=	1100
.if ${HAVE_GDB} == 1100
EXTERNAL_GDB_SUBDIR=		gdb
.else
EXTERNAL_GDB_SUBDIR=		/does/not/exist
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

.include <bsd.tools.mk>

# where the system object and source trees are kept; can be configurable
# by the user in case they want them in ~/foosrc and ~/fooobj, for example
BSDSRCDIR?=			/usr/src
BSDOBJDIR?=			/usr/obj
NETBSDSRCDIR?=		${BSDSRCDIR}

BINGRP?=			bin
BINOWN?=			root
BINMODE?=			555
NONBINMODE?=		444
DIRMODE?=			755

SHAREDIR?=			/usr/share
SHAREGRP?=			bin
SHAREOWN?=			root
SHAREMODE?=			${NONBINMODE}

MANDIR?=			/usr/share/man
MANGRP?=			bin
MANOWN?=			root
MANMODE?=			${NONBINMODE}
MANINSTALL?=		maninstall catinstall

LIBDIR?=			/usr/lib
LINTLIBDIR?=		/usr/libdata/lint
LIBGRP?=			${BINGRP}
LIBOWN?=			${BINOWN}
LIBMODE?=			${NONBINMODE}

DOCDIR?=    		/usr/share/doc
DOCGRP?=			bin
DOCOWN?=			root
DOCMODE?=   		${NONBINMODE}

NLSDIR?=			/usr/share/nls
NLSGRP?=			bin
NLSOWN?=			root
NLSMODE?=			${NONBINMODE}

LOCALEDIR?=			/usr/share/locale
LOCALEGRP?=			wheel
LOCALEOWN?=			root
LOCALEMODE?=		${NONBINMODE}

# Data-driven table using make variables to control how 
# toolchain-dependent targets and shared libraries are built
# for different platforms and object formats.
# OBJECT_FMT:		currently either "ELF" or "a.out".
#
		
OBJECT_FMT_ELF?=	ELF
OBJECT_FMT_AOUT?=	a.out
OBJECT_FMT=			OBJECT_FMT_ELF

#
# If this platform's toolchain is missing, we obviously cannot build it.
#
.if ${TOOLCHAIN_MISSING} != "no"
MKBFD:= no
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
# In order to identify NetBSD to GNU packages, we sometimes need
# an "elf" tag for historically a.out platforms.
#
.if ${OBJECT_FMT} == ${ELF} && (${MACHINE_ARCH} == "i386")
MACHINE_GNU_PLATFORM?=${MACHINE_GNU_ARCH}--netbsdelf
.endif

.if ${OBJECT_FMT} == "a.out" && (${MACHINE_ARCH} == "i386")
MACHINE_GNU_PLATFORM?=${MACHINE_GNU_ARCH}--netbsd
.endif

TARGETS+=	all clean cleandir depend dependall includes \
			install lint obj regress tags
.PHONY:		all clean cleandir depend dependall distclean includes \
			install lint obj regress tags beforedepend afterdepend \
			beforeinstall afterinstall realinstall realdepend realall \
			subdir-all subdir-install subdir-depend
			
.if ${NEED_OWN_INSTALL_TARGET} != "no"
.if !target(install)
install:		.NOTMAIN beforeinstall subdir-install realinstall afterinstall
beforeinstall:	.NOTMAIN
subdir-install:	.NOTMAIN beforeinstall
realinstall:	.NOTMAIN beforeinstall
afterinstall:	.NOTMAIN subdir-install realinstall
.endif
all:			.NOTMAIN realall subdir-all
subdir-all:		.NOTMAIN
realall:		.NOTMAIN
depend:			.NOTMAIN realdepend subdir-depend
subdir-depend:	.NOTMAIN
realdepend:		.NOTMAIN
distclean:		.NOTMAIN cleandir
cleandir:		.NOTMAIN clean

dependall:		.NOTMAIN realdepend .MAKE
	@cd ${.CURDIR}; ${MAKE} realall
.endif

# Define MKxxx variables (which are either yes or no) for users
# to set in /etc/mk.conf and override on the make commandline.
# These should be tested with `== "no"' or `!= "no"'.
# The NOxxx variables should only be used by Makefiles.
#

MKCATPAGES?=yes

#
# Make the bootloader on supported arches
#
.if ${MACHINE_ARCH} == "i386"
MKBOOT= 	yes
.endif

.if defined(NODOC)
MKDOC=		no
#.elif !defined(MKDOC)
#MKDOC=yes
.else
MKDOC?=		yes
.endif

MKINFO?=	yes

.if defined(NOLINKLIB)
MKLINKLIB=	no
.else
MKLINKLIB?=	yes
.endif
.if ${MKLINKLIB} == "no"
MKPICINSTALL=	no
MKPROFILE=	no
.endif

.if defined(NOLINT)
MKLINT=		no
.else
MKLINT?=	yes
.endif

.if defined(NOMAN)
MKMAN=		no
.else
MKMAN?=		yes
.endif
.if ${MKMAN} == "no"
MKCATPAGES=	no
.endif

.if defined(NONLS)
MKNLS=		no
.else
MKNLS?=		yes
.endif

.if defined(NOOBJ)
MKOBJ=		no
.else
MKOBJ?=		yes
.endif

.if defined(NOPIC)
MKPIC=		no
.else
MKPIC?=		yes
.endif

.if defined(NOPICINSTALL)
MKPICINSTALL=	no
.else
MKPICINSTALL?=	yes
.endif

.if defined(NOPROFILE)
MKPROFILE=	no
.else
MKPROFILE?=	yes
.endif

.if defined(NOSHARE)
MKSHARE=	no
.else
MKSHARE?=	yes
.endif
.if ${MKSHARE} == "no"
MKCATPAGES=	no
MKDOC=		no
MKINFO=		no
MKMAN=		no
MKNLS=		no
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
SYSPKGDOCTAG?=	${SYSPKG:D-T ${SYSPKG}-doc_pkg}
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
.else	# MAKEVERBOSE >= 2
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