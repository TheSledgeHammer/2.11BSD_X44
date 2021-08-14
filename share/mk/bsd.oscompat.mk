#	$211BSD: Makefile,v 1.0 2021/05/25 23:59:27 Exp $
#  	OS Compatability (/contrib/external/compat)

.if !defined(_BSD_OSCOMPAT_MK_)
_BSD_OSCOMPAT_MK_=1

.include <bsd.own.mk>

MKPLAN9= yes

.include <bsd.sys.mk>

.endif	# !defined(_BSD_OSCOMPAT_MK_)