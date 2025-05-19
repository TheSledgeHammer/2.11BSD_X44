#	$211BSD: Makefile,v 1.0 2025/05/19 19:27:27 Exp $

.include <bsd.own.mk>
.include "plan9.mk"

.if (${MKLIBUTF} != "no")
SUBDIR+= libutf
.endif

.if (${MKLIBFMT} != "no")
SUBDIR+= libutf libfmt
.endif

.if (${MKLIBBIO} != "no")
SUBDIR+= libutf libfmt libbio
.endif

.if (${MKLIBREGEXP} != "no")
SUBDIR+= libutf libfmt libregexp
.endif

.if (${MKMK} != "no")
SUBDIR+= libutf libfmt libbio libregexp
.endif

.include <bsd.subdir.mk>
