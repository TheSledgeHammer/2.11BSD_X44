#	$211BSD: Makefile.inc,v 1.0 2025/06/01 17:34:50 martin Exp $

# machine-independent nss sources
.PATH: ${ARCHDIR}/nss ${.CURDIR}/nss

SRCS+= getgrent_ns.c getpwent_ns.c
