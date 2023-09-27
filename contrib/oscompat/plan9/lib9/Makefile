include ../Make.inc
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
	install -m 0644 libc.h $(PREFIX)/include/libc.h
	mkdir -p $(PREFIX)/lib
	install -m 0644 $(LIB) $(PREFIX)/lib/$(LIB)

include ../Make.clib
include ../unix/Make.package
