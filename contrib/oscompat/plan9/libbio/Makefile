include ../Make.inc
include ../unix/Make.$(SYSNAME)

PREFIX=/usr/local

NUKEFILES=

TGZFILES=

CLEANFILES=

LIB=libbio.a
VERSION=2.0
PORTPLACE=devel/libbio
NAME=libbio

OFILES=\
	bbuffered.$O\
	bfildes.$O\
	bflush.$O\
	bgetc.$O\
	bgetd.$O\
	bgetrune.$O\
	binit.$O\
	boffset.$O\
	bprint.$O\
	bvprint.$O\
	bputc.$O\
	bputrune.$O\
	brdline.$O\
	brdstr.$O\
	bread.$O\
	bseek.$O\
	bwrite.$O\

HFILES=\
	bio.h\

all: $(LIB)

install: $(LIB)
	mkdir -p $(PREFIX)/share/man/man3
	install -m 0644 bio.3 $(PREFIX)/share/man/man3/bio.3
	mkdir -p $(PREFIX)/include
	install -m 0644 bio.h $(PREFIX)/include
	mkdir -p $(PREFIX)/lib
	install -m 0644 $(LIB) $(PREFIX)/lib/$(LIB)

bcat: bcat.$O $(LIB)
	$(CC) -o bcat bcat.$O $(LIB) -L/usr/local/lib -lfmt -lutf

include ../Make.clib
include ../unix/Make.package
