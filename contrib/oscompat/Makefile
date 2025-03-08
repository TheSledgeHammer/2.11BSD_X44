#	$211BSD: Makefile,v 1.0 2021/06/07 23:59:27 Exp $


.include <bsd.own.mk>
.include <bsd.oscompat.mk>

.if(${MK211BSD} != "no")
SUBDIR+= 211bsd
.endif

.if(${MKDRAGONFLYBSD} != "no")
SUBDIR+= dragonflybsd
.endif

.if(${MKFREEBSD} != "no")
SUBDIR+= freebsd
.endif

.if(${MKNETBSD} != "no")
SUBDIR+= netbsd
.endif

.if(${MKOPENBSD} != "no")
SUBDIR+= openbsd
.endif

.if(${MKPLAN9} != "no")
SUBDIR+= plan9
.endif

.if(${MKLINUX} != "no")
SUBDIR+= linux
.endif

.if(${MKSOLARIS} != "no")
SUBDIR+= solaris
.endif

.include <bsd.subdir.mk>
