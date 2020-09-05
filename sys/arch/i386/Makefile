#	@(#)Makefile	8.1 (Berkeley) 6/11/93

# Makefile for i386 links, tags file

SYSDIR?=	/sys

TI386=	${SYSDIR}/arch/i386/tags
SI386=	${SYSDIR}/arch/i386/eisa/*.[ch] \
		${SYSDIR}/arch/i386/i386/*.[ch] \
		${SYSDIR}/arch/i386/include/*.h \
		${SYSDIR}/arch/i386/isa/*.[ch] 	\
		${SYSDIR}/arch/i386/pci/*.[ch] \
AI386=	${SYSDIR}/arch/i386/i386/*.s 	\
		

# Directories in which to place tags links
DI386=	eisa isa include pci

.include "../../kern/Make.tags.inc"

tags:
	-rm -f ${TI386}
	-ctags -wdt ${COMM} ${I386}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${AI386} | \
		${TOOL_SED} -e \
	"s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3	\1	/^\2(\3\4$$/;" \
		>> ${TI386}
	sort -o ${TI386} ${TI386}

links:
	-for i in ${DI386}; do \
	    (cd $$i && rm -f tags; ln -s ../tags tags); done

.include <bsd.subdir.mk>
