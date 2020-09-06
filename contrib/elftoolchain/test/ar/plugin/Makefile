# $Id: Makefile 3393 2016-02-07 11:31:18Z jkoshy $

PLUGINS=	ardiff teraser

all:	${PLUGINS}

${PLUGINS}:
.for plugin in ${.TARGET}
	${MAKE} -f Makefile.${plugin}
.endfor

test:	.PHONY

clean cleandepend clobber depend:	.PHONY
.for plugin in ${PLUGINS}
	${MAKE} -f Makefile.${plugin} ${.TARGET}
.endfor
