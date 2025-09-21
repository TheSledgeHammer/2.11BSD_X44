#	$NetBSD: Makefile.inc,v 1.2 2003/07/04 06:05:30 tshiozak Exp $

.PATH: ${ARCHDIR}/citrus ${.CURDIR}/citrus

# citrus base
SRCS+=	citrus_ctype.c citrus_stdenc.c
SRCS+=  citrus_frune.c citrus_hash.c citrus_memstream.c \
		citrus_mmap.c

# citrus db
SRCS+= citrus_db_factory.c citrus_db_hash.c citrus_db.c

# citrus lookup
SRCS+= citrus_lookup_factory.c citrus_lookup.c

# citrus mapper
SRCS+= citrus_mapper_std.c citrus_mapper.c citrus_csmapper.c

# citrus pivot
SRCS+= citrus_pivot_factory.c

# citrus iconv
SRCS+= citrus_iconv_std.c citrus_iconv.c

# citrus esdb
SRCS+= citrus_esdb.c
