# $FreeBSD$

SRCS+=	start.S \
		exec.c

.PATH:	${BOOTSRC}/i386/libi386
SRCS+=	nullconsole.c \
		comconsole.c \
		spinconsole.c

CFLAGS+=	-fPIC
LDFLAGS+=	-Wl,-znocombreloc
