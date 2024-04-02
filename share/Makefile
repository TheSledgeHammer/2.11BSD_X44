#	from @(#)Makefile	8.1 (Berkeley) 6/5/93

.include <bsd.own.mk>

.if ${MKSHARE} != "no" || make(clean) || make(cleandir) || make(distclean) || make(obj)
SUBDIR= dict man me misc mk ms tabset \
        termcap tmac wscons zoneinfo
		
# 2.11BSD:
#SUBDIR+= adb font fontinfo learn lint pascal term

.if ${MKGROFF} != "no"
SUBDIR+=doc
.endif

.if ${MKNLS} != "no"
SUBDIR+=locale nls
.endif
.endif

SUBDIR.roff+= doc

# Speedup stubs for some subtrees that don't need to run these rules
includes-man:
	@true
	
.include <bsd.subdir.mk>
