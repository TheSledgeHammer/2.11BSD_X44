
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

LIB=lib9.a
VERSION=2.0
PORTPLACE=devel/lib9
NAME=lib9

OFILES=\
	_p9dir.$O\
	_exits.$O\
	argv0.$O\
	atoi.$O\
	cleanname.$O\
	create.$O\
	dirfstat.$O\
	dirfwstat.$O\
	dirstat.$O\
	dirwstat.$O\
	dup.$O\
	errstr.$O\
	exec.$O\
	execl.$O\
	exitcode.$O\
	exits.$O\
	getenv.$O\
	getfields.$O\
	getwd.$O\
	main.$O\
	nulldir.$O\
	open.$O\
	readn.$O\
	seek.$O\
	strecpy.$O\
	sysfatal.$O\
	time.$O\
	tokenize.$O\
	await.$O\
	getuser.$O\
	jmp.$O\
	notify.$O\
	rfork.$O\

HFILES=\
    libc.h\
    u.h\

all: $(LIB)

install: $(LIB)
	test -d $(PREFIX)/share/man/man3 || mkdir -p $(PREFIX)/share/man/man3
	install -m 0644 print.3 $(PREFIX)/share/man/man3/print.3
#	install -m 0644 fmtinstall.3 $(PREFIX)/share/man/man3/fmtinstall.3
	mkdir -p $(PREFIX)/include
#	install -m 0644 fmt.h $(PREFIX)/include/fmt.h
	mkdir -p $(PREFIX)/lib
	install -m 0644 $(LIB) $(PREFIX)/lib/$(LIB)

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
