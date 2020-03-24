#	@(#)Makefile	8.1 (Berkeley) 6/11/93

# Makefile for i386 links, tags file

TI386=	${SYSDIR}/arch/i386/tags
SI386=	${SYSDIR}/arch/i386/eisa/*.[ch] \
		${SYSDIR}/arch/i386/i386/*.[ch] \
		${SYSDIR}/arch/i386/include/*.h \
		${SYSDIR}/arch/i386/isa/*.[ch] 	\
AI386=	${SYSDIR}/arch/i386/i386/*.s 	\

# Directories in which to place tags links
DI386=	i386 eisa isa include scripts

#.include "../../kern/Make.tags.inc"

tags:
	-rm -f ${TI386}
	-echo ${SI386} | xargs $(TOOL_CTAGS) -wadtf ${TI386}
	-find -H ${SYSDIR}/external/bsd/acpica/dist/ -name '*.[ch]' | \
	    sort -t / -u | xargs $(TOOL_CTAGS) -wadtf ${TI386}
	-${FINDCOMM} | xargs $(TOOL_CTAGS) -wadtf ${TI386}
	#-ctags -wdt ${COMM} ${I386}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${AI386} | \
		${TOOL_SED} -e \
	"s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3	\1	/^\2(\3\4$$/;" \
		>> ${TI386}
	sort -o ${TI386} ${TI386}

links:
	-for i in ${DI386}; do \
	    (cd $$i && rm -f tags; ln -s ../tags tags); done

SUBDIR= include

#.include <bsd.subdir.mk>
