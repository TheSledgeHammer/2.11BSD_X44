#
# Public domain - 1996/10/23 - sms
#
#	@(#)Makefile	1.1 (2.11BSD) 1997/9/26
#
#  Makefile for the lint libraries.
#

FILES= 	llib-lc llib-lcurses llib-ldbm llib-lm llib-lmp \
 	llib-lplot llib-ltermcap llib-port
FILESDIR= /usr/share/lint

all:

install:${FILES}
	-mkdir -p ${FILESDIR}
	-chmod a+r,a+x ${FILESDIR}
	install -c -m 444 ${FILES} ${FILESDIR}

libs:
	${TOOL_CC} -C -Dlint ${FILESDIR}/llib-port | ${LINT} -puv > ${FILESDIR}/llib-port.ln
	${TOOL_CC} -C -Dlint ${FILESDIR}/llib-ldbm | ${LINT} -uv > ${FILESDIR}/llib-ldbm.ln
	${TOOL_CC} -C -Dlint ${FILESDIR}/llib-lm | ${LINT} -uv > ${FILESDIR}/llib-lm.ln
	${TOOL_CC} -C -Dlint ${FILESDIR}/llib-lmp | ${LINT} -uv > ${FILESDIR}/llib-lmp.ln
	${TOOL_CC} -C -Dlint ${FILESDIR}/llib-lplot | ${LINT} -uv > ${FILESDIR}/llib-lplot.ln
	${TOOL_CC} -C -Dlint ${FILESDIR}/llib-ltermcap | ${LINT} -uv > ${FILESDIR}/llib-ltermcap.ln
	${TOOL_CC} -C -Dlint ${FILESDIR}/llib-lc | ${LINT} -v > ${FILESDIR}/llib-lc.ln
	${TOOL_CC} -C -Dlint ${FILESDIR}/llib-lcurses | ${LINT} -v > ${FILESDIR}/llib-lcurses.ln
	
depend:

.include <bsd.prog.mk>
