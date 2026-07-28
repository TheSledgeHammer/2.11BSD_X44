#	$211BSD: Makefile,v 1.0 2021/05/25 23:59:27 Exp $
#  	OS Compatability (/contrib/compat)

.if !defined(_BSD_OSCOMPAT_MK_)
_BSD_OSCOMPAT_MK_=1

.include <bsd.own.mk>

# 2.11BSD Compatability
MK211BSD= yes

.if(${MK211BSD} != "no")
SUBDIR+= 211bsd

# 2.11BSD options which default to "yes"
_MK211BSD.yes= \
	MKLIBSTUBS

.for var in ${_MK211BSD.yes}
${var}?=	${${var}:Uyes}
.endfor

# 2.11BSD options which default to "no"
_MK211BSD.no= \
	MKLIBNDBM \
	MKLIBMP \
	MKLIBOM \
	MKLIBTERMCAP \
	MKLIBVMF
	
.for var in ${_MK211BSD.no}
${var}?=	${${var}:Uno}
.endfor

.endif

# DragonflyBSD Compatability
MKDRAGONFLYBSD=	no

.if(${MKDRAGONFLYBSD} != "no")
SUBDIR+= dragonflybsd

# DragonflyBSD options which default to "yes"
_MKDRAGONFLYBSD.yes=

# DragonflyBSD options which default to "no"
_MKDRAGONFLYBSD.no=

.endif

# FreeBSD Compatability
MKFREEBSD= no

.if(${MKFREEBSD} != "no")
SUBDIR+= freebsd

# FreeBSD options which default to "yes"
_MKFREEBSD.yes=

# FreeBSD options which default to "no"
_MKFREEBSD.no=

.endif

# NetBSD Compatability
MKNETBSD= yes

.if(${MKNETBSD} != "no")
SUBDIR+= netbsd

# NetBSD options which default to "yes"
_MKNETBSD.yes=

# NetBSD options which default to "no"
_MKNETBSD.no=

.endif

# OpenBSD Compatability
MKOPENBSD= no

.if(${MKOPENBSD} != "no")
SUBDIR+= openbsd

# OpenBSD options which default to "yes"
_MKOPENBSD.yes=

# OpenBSD options which default to "no"
_MKOPENBSD.no=

.endif

# Plan9 Compatability
MKPLAN9= yes

.if(${MKPLAN9} != "no")
SUBDIR+= plan9

# Plan9 options which default to "yes"
_MKPLAN9.yes= \
	MKLIBBIO \
	MKLIBFMT \
	MKLIBREGEXP \
	MKLIBUTF \
	MKMK \

.for var in ${_MKPLAN9.yes}
${var}?=	${${var}:Uyes}
.endfor

# Plan9 options which default to "no"
_MKPLAN9.no=

.for var in ${_MKPLAN9.no}
${var}?=	${${var}:Uno}
.endfor

.endif

# Linux Compatability
MKLINUX= no

.if(${MKLINUX} != "no")
SUBDIR+= linux

# Linux options which default to "yes"
_MKLINUX.yes=

# Linux options which default to "no"
_MKLINUX.no=

.endif

# Solaris Compatability
MKSOLARIS= no

.if(${MKSOLARIS} != "no")
SUBDIR+= solaris

# Solaris options which default to "yes"
_MKSOLARIS.yes=

# Solaris options which default to "no"
_MKSOLARIS.no=

.endif

.include <bsd.sys.mk>

.endif	# !defined(_BSD_OSCOMPAT_MK_)
