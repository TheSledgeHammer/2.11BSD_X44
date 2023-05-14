#	from @(#)Makefile	8.1 (Berkeley) 6/5/93

.include <bsd.own.mk>

.if ${MKSHARE} != "no" || make(clean) || make(cleandir) || make(distclean) || make(obj)
SUBDIR= 	adb dict font fontinfo learn man me misc mk ms \
			tabset term termcap tmac wscons lint zoneinfo

.if ${MKGROFF} != "no"
SUBDIR+=doc
.endif

.if ${MKNLS} != "no"
SUBDIR+=locale
.endif
.endif

SUBDIR.roff+= doc

# Speedup stubs for some subtrees that don't need to run these rules
includes-examples includes-man:
	@true
	
.include <bsd.subdir.mk>
