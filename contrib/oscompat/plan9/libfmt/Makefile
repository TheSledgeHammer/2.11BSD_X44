include ../Make.inc
include ../unix/Make.$(SYSNAME)

PREFIX=/usr/local

NUKEFILES=

TGZFILES=

CLEANFILES=

LIB=libfmt.a
VERSION=2.0
PORTPLACE=devel/libfmt
NAME=libfmt

NUM=\
	charstod.$O\
	pow10.$O\

OFILES=\
	dofmt.$O\
	dorfmt.$O\
	errfmt.$O\
	fltfmt.$O\
	fmt.$O\
	fmtfd.$O\
	fmtfdflush.$O\
	fmtlocale.$O\
	fmtlock.$O\
	fmtnull.$O\
	fmtprint.$O\
	fmtquote.$O\
	fmtrune.$O\
	fmtstr.$O\
	fmtvprint.$O\
	fprint.$O\
	print.$O\
	runefmtstr.$O\
	runeseprint.$O\
	runesmprint.$O\
	runesnprint.$O\
	runesprint.$O\
	runevseprint.$O\
	runevsmprint.$O\
	runevsnprint.$O\
	seprint.$O\
	smprint.$O\
	snprint.$O\
	sprint.$O\
	strtod.$O\
	vfprint.$O\
	vseprint.$O\
	vsmprint.$O\
	vsnprint.$O\
	$(NUM)\
	$(NAN)\

HFILES=\
	fmtdef.h\
	fmt.h\

all: $(LIB)

install: $(LIB)
	test -d $(PREFIX)/share/man/man3 || mkdir -p $(PREFIX)/share/man/man3
	install -m 0644 print.3 $(PREFIX)/share/man/man3/print.3
	install -m 0644 fmtinstall.3 $(PREFIX)/share/man/man3/fmtinstall.3
	mkdir -p $(PREFIX)/include
	install -m 0644 fmt.h $(PREFIX)/include/fmt.h
	mkdir -p $(PREFIX)/lib
	install -m 0644 $(LIB) $(PREFIX)/lib/$(LIB)

$(NAN).$O: nan.h
strtod.$O: nan.h

test: $(LIB) test.$O
	$(CC) -o test test.$O $(LIB) -L$(PREFIX)/lib -lutf

include ../Make.clib
include ../unix/Make.package
