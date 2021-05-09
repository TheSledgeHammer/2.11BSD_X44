#	$NetBSD: bsd.own.mk,v 1.1247 2021/05/06 13:23:36 rin Exp $

# This needs to be before bsd.init.mk
.if defined(BSD_MK_COMPAT_FILE)
.include <${BSD_MK_COMPAT_FILE}>
.endif

.if !defined(_BSD_OWN_MK_)
_BSD_OWN_MK_=1

MAKECONF?=	/etc/mk.conf
.-include "${MAKECONF}"

# Set `WARNINGS' to `yes' to add appropriate warnings to each compilation
WARNINGS?=		no
# Defining `SKEY' causes support for S/key authentication to be compiled in.
SKEY=			yes
# Defining `KERBEROS' causes support for Kerberos authentication to be
# compiled in.
#KERBEROS=		yes
# Defining 'KERBEROS5' causes support for Kerberos5 authentication to be
# compiled in.
#KERBEROS5=		yes

# where the system object and source trees are kept; can be configurable
# by the user in case they want them in ~/foosrc and ~/fooobj, for example
BSDSRCDIR?=			/usr/src
BSDOBJDIR?=			/usr/obj

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

COPY?=		-c
.if defined(UPDATE)
PRESERVE?=	-p
.else
PRESERVE?=
.endif
RENAME?=
STRIPFLAG?=	-s

# Define SYS_INCLUDE to indicate whether you want symbolic links to the system
# source (``symlinks''), or a separate copy (``copies''); (latter useful
# in environments where it's not possible to keep /sys publicly readable)
#SYS_INCLUDE= 	symlinks

# Data-driven table using make variables to control how 
# toolchain-dependent targets and shared libraries are built
# for different platforms and object formats.
# OBJECT_FMT:		currently either "ELF" or "a.out".
# SHLIB_TYPE:		"ELF" or "a.out" or "" to force static libraries.
#

OBJECT_FMT?=ELF

OBJECT_FMT?=a.out

# GNU sources and packages sometimes see architecture names differently.
# This table maps an architecture name to its GNU counterpart.
# Use as so:  ${GNU_ARCH.${TARGET_ARCH}} or ${MACHINE_GNU_ARCH}

GNU_ARCH.i386=i386

TARGETS+=	all clean cleandir depend distclean includes install lint obj \
			regress tags
.PHONY:		all clean cleandir depend distclean includes install lint obj \
			regress tags beforedepend afterdepend beforeinstall \
			afterinstall realinstall

# set NEED_OWN_INSTALL_TARGET, if it's not already set, to yes
# this is used by bsd.pkg.mk to stop "install" being defined
NEED_OWN_INSTALL_TARGET?=	yes

.if ${NEED_OWN_INSTALL_TARGET} == "yes"
.if !target(install)
install:		.NOTMAIN beforeinstall subdir-install realinstall afterinstall
beforeinstall:	.NOTMAIN
subdir-install:	.NOTMAIN beforeinstall
realinstall:	.NOTMAIN beforeinstall
afterinstall:	.NOTMAIN subdir-install realinstall
.endif
.endif

# Define MKxxx variables (which are either yes or no) for users
# to set in /etc/mk.conf and override on the make commandline.
# These should be tested with `== "no"' or `!= "no"'.
# The NOxxx variables should only be used by Makefiles.
#

MKCATPAGES?=yes

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

.endif		# _BSD_OWN_MK_