#	$211BSD: Makefile,v 1.0 2021/05/25 23:59:27 Exp $
#  	OS Compatability (/contrib/compat)

.if !defined(_BSD_OSCOMPAT_MK_)
_BSD_OSCOMPAT_MK_=1

.include <bsd.own.mk>

# 2.11BSD Compatability
MK211BSD= yes

# DragonflyBSD Compatability
MKDRAGONFLYBSD=	no

# FreeBSD Compatability
MKFREEBSD= no

# NetBSD Compatability
MKNETBSD= yes

# OpenBSD Compatability
MKOPENBSD= no

# Plan9 Compatability
MKPLAN9= yes

# Linux Compatability
MKLINUX= no

# Solaris Compatability
MKSOLARIS= no

# OS Compatability options which default to "yes"

# 2.11BSD options which default to "yes"
_MK211BSD.yes= \
	MKLIBSTUBS

# DragonflyBSD options which default to "yes"
_MKDRAGONFLYBSD.yes=

# FreeBSD options which default to "yes"
_MKFREEBSD.yes=

# NetBSD options which default to "yes"
_MKNETBSD.yes=

# OpenBSD options which default to "yes"
_MKOPENBSD.yes=

# Plan9 options which default to "yes"
_MKPLAN9.yes= \
	MKLIBBIO \
	MKLIBFMT \
	MKLIBREGEXP \
	MKLIBUTF

# Linux options which default to "yes"
_MKLINUX.yes=

# Solaris options which default to "yes"
_MKSOLARIS.yes=

_MKVARS.yes= \
    ${_MK211BSD.yes} \
    ${_MKDRAGONFLYBSD.yes} \
    ${_MKFREEBSD.yes} \
    ${_MKNETBSD.yes} \
    ${_MKOPENBSD.yes} \
    ${_MKPLAN9.yes} \
    ${_MKLINUX.yes} \
    ${_MKSOLARIS.yes}

.for var in ${_MKVARS.yes}
${var}?=	${${var}.:Uyes}
.endfor

# OS Compatability options which default to "no"

# 2.11BSD options which default to "no"
_MK211BSD.no= \
	MKLIBNDBM \
	MKLIBFORTRAN \
	MKLIBMP \
	MKLIBOM \
	MKLIBTERMCAP \
	MKLIBVMF

# DragonflyBSD options which default to "no"
_MKDRAGONFLYBSD.no=

# FreeBSD options which default to "no"
_MKFREEBSD.no=

# NetBSD options which default to "no"
_MKNETBSD.no=

# OpenBSD options which default to "no"
_MKOPENBSD.no=

# Plan9 options which default to "no"
_MKPLAN9.no= \
	MKMK

# Linux options which default to "no"
_MKLINUX.no=

# Solaris options which default to "no"
_MKSOLARIS.no=

_MKVARS.no= \
    ${_MK211BSD.no} \
    ${_MKDRAGONFLYBSD.no} \
    ${_MKFREEBSD.no} \
    ${_MKNETBSD.no} \
    ${_MKOPENBSD.no} \
    ${_MKPLAN9.no} \
    ${_MKLINUX.no} \
    ${_MKSOLARIS.no}

.for var in ${_MKVARS.no}
${var}?=	${${var}.:Uno}
.endfor

.include <bsd.sys.mk>

.endif	# !defined(_BSD_OSCOMPAT_MK_)
