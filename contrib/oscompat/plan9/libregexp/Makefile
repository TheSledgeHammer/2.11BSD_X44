
# this works in gnu make
SYSNAME:=${shell uname}

# this works in bsd make
SYSNAME!=uname

# the gnu rules will mess up bsd but not vice versa,
# hence the gnu rules come first.

RANLIB=true

include ../unix/Make.$(SYSNAME)

PREFIX=/usr/local

NUKEFILES=

TGZFILES=

CLEANFILES=

LIB=libregexp9.a
VERSION=2.0
PORTPLACE=devel/libregexp9
NAME=libregexp9

OFILES=\
	regcomp.$O\
	regerror.$O\
	regexec.$O\
	regsub.$O\
	regaux.$O\
	rregexec.$O\
	rregsub.$O\

HFILES=\
	regexp9.h\
	regcomp.h\

all: $(LIB)

install: $(LIB)
	mkdir -p $(PREFIX)/share/man/man3 $(PREFIX)/man/man7
	install -m 0644 regexp9.3 $(PREFIX)/share/man/man3/regexp9.3
	install -m 0644 regexp9.7 $(PREFIX)/man/man7/regexp9.7
	mkdir -p $(PREFIX)/lib
	install -m 0644 $(LIB) $(PREFIX)/lib/$(LIB)
	mkdir -p $(PREFIX)/include
	install -m 0644 regexp9.h $(PREFIX)/include/regexp9.h

test: test.$O $(LIB)
	$(CC) -o test test.$O $(LIB) -L/usr/local/lib -lfmt -lutf

test2: test2.$O $(LIB)
	$(CC) -o test2 test2.$O $(LIB) -L/usr/local/lib -lfmt -lutf

$(LIB): $(OFILES)
	$(AR) $(ARFLAGS) $(LIB) $(OFILES)
	$(RANLIB) $(LIB)

NUKEFILES+=$(LIB)
.c.$O:
	$(CC) $(CFLAGS) -I../libutf -I../libfmt -I../libbio -I../libregexp -I../lib9 -I$(PREFIX)/include $*.c

%.$O: %.c
	$(CC) $(CFLAGS) -I../libutf -I../libfmt -I../libbio -I../libregexp -I../lib9 -I$(PREFIX)/include $*.c


$(OFILES): $(HFILES)

include ../unix/Make.package
