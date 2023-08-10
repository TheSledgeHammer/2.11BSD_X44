#	$NetBSD: Makefile.inc,v 1.1 2009/01/16 19:39:52 pooka Exp $
#
#	$FreeBSD$
#

.PATH:	${.CURDIR}/ffs ${NETBSDSRCDIR}/sys/ufs/ffs

SRCS+=	ffs_alloc.c ffs_balloc.c ffs_bswap.c ffs_subr.c ufs_bmap.c
SRCS+=	buf.c mkfs.c

# Reach-over source from sys/ufs/ffs
SRCS+=	ffs_tables.c
