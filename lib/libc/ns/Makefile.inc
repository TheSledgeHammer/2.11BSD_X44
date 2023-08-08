#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	6.1 (Berkeley) 1/29/86
#
.PATH: ${ARCHDIR}/ns ${.CURDIR}/ns

SRCS+=	nsdispatch.c nslexer.l nsparser.y ns_addr.c ns_ntoa.c nsap_addr.c

LPREFIX=_nsyy
YPREFIX=_nsyy
YHEADER=1

CLEANFILES+=nsparser.c nslexer.c nsparser.h

nslexer.c: nslexer.l nsparser.h
	${_MKTARGET_LEX}
	${LEX.l} -t ${.IMPSRC} | sed -e '/YY_BUF_SIZE/s/16384/1024/' >${.TARGET}

MAN+= ns3 nsdispatch.3
MLINKS+=ns.3 ns_addr.3 ns.3 ns_ntoa.3
