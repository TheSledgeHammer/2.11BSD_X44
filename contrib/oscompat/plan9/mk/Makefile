#	$211BSD: Makefile,v 1.0 2025/05/19 19:27:27 Exp $

.include <bsd.own.mk>

TARG= mk
PROG= ${TARG}
MAN= mk.1
VERSION=2.0

.include "../Makefile.inc"

CPPFLAGS+= -I${PLAN9_INCLUDE}
CPPFLAGS+= -I${PLAN9_LIBREGEXP}
CPPFLAGS+= -I${PLAN9_LIBBIO}
CPPFLAGS+= -I${PLAN9_LIBUTF}
CPPFLAGS+= -I${PLAN9_LIBFMT}

.PATH: ${PLAN9_INCLUDE}
.PATH: ${PLAN9_LIBREGEXP}
.PATH: ${PLAN9_LIBBIO}
.PATH: ${PLAN9_LIBUTF}
.PATH: ${PLAN9_LIBFMT}

#LDADD+= -lregexp -lbio -lfmt -lutf
#DPADD+= ${LIBREGEXP} ${LIBBIO} ${LIBFMT} ${LIBUTF}
PROGDPLIBS+= ${DPLIBS}

CFILES=\
	arc.c\
	archive.c\
	bufblock.c\
	env.c\
	file.c\
	graph.c\
	job.c\
	lex.c\
	main.c\
	match.c\
	mk.c\
	parse.c\
	recipe.c\
	rc.c\
	rule.c\
	run.c\
	sh.c\
	shell.c\
	shprint.c\
	symtab.c\
	var.c\
	varsub.c\
	word.c\
	unix.c\

SRCS= ${CFILES}

COPTS.match.c = -Wno-stringop-overflow

.include <bsd.prog.mk>
