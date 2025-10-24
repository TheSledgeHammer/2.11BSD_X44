#	$NetBSD: Makefile,v 1.9 2013/04/27 17:12:36 christos Exp $

USE_FORT?=yes	# setgid

PROG=	ccdconfig
MAN=	ccdconfig.8 ccd.conf.5

BINGRP=	kmem
BINMODE=2555

DPADD=	${LIBKVM} ${LIBUTIL}
LDADD=	-lkvm -lutil

.include <bsd.prog.mk>
