
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

TARG=mk
VERSION=2.0
PORTPLACE=devel/mk
NAME=mk
CLEANFILES=mk mk.1a

OFILES=\
	arc.$O\
	archive.$O\
	bufblock.$O\
	env.$O\
	file.$O\
	graph.$O\
	job.$O\
	lex.$O\
	main.$O\
	match.$O\
	mk.$O\
	parse.$O\
	recipe.$O\
	rc.$O\
	rule.$O\
	run.$O\
	sh.$O\
	shell.$O\
	shprint.$O\
	symtab.$O\
	var.$O\
	varsub.$O\
	word.$O\
	unix.$O\

HFILES=\
	mk.h\
	fns.h\

all: $(TARG)

TGZFILES+=mk.pdf

install: $(TARG)
	mkdir -p $(PREFIX)/bin
	install -m 0755 mk $(PREFIX)/bin/mk
	cat mk.1 | sed 's;DOCPREFIX;$(PREFIX);g' >mk.1a
	mkdir -p $(PREFIX)/man/man1
	install -m 0644 mk.1a $(PREFIX)/man/man1/mk.1
	mkdir -p $(PREFIX)/doc
	install -m 0644 mk.pdf $(PREFIX)/doc/mk.pdf

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

$(TARG): $(OFILES)
	$(CC) -o $(TARG) $(OFILES) -L$(PREFIX)/lib -L../libutf -L../libfmt -L../libbio -L../libregexp -L../lib9 -lregexp9 -lbio -lfmt -lutf -l9

