#	$NetBSD: Makefile,v 1.7 1999/02/13 23:53:57 lukem Exp $
#
# Copyright (c) 1987 Regents of the University of California.
# All rights reserved.
#
# Redistribution and use in source and binary forms are permitted
# provided that the above copyright notice and this paragraph are
# duplicated in all such forms and that any documentation,
# advertising materials, and other materials related to such
# distribution and use acknowledge that the software was developed
# by the University of California, Berkeley.  The name of the
# University may not be used to endorse or promote products derived
# from this software without specific prior written permission.
# THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
# WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
#
#	@(#)Makefile	5.3 (Berkeley) 7/9/88
#
# Note: Is compiled with /usr/share/

MAN=makewhatis.8
SCRIPTS=makewhatis.sh

.include <bsd.prog.mk>

SUBDIR+=man1 man3 man3f man4 man5 man6 man7 man8 \
		cat1 cat2 cat3 cat3f cat4 cat5 cat6 cat7 cat8		
	    	
makedb:
{
	makewhatis ${DESTDIR}/usr/man
}

.include <bsd.subdir.mk>
