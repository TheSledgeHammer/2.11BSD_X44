INGRES=		..
BIN=		$(INGRES)/bin
DEMO=		$(INGRES)/demo
LIB=		$(INGRES)/lib
DESTDIR=	/usr
DESTBIN=	$(DESTDIR)/bin
DESTLIB=	$(DESTDIR)/lib
LDFLAGS = 	-i

YACC=		$(BIN)/yyyacc
YFLAGS=		-s

# Library
ACCESS=		$(LIB)/access
DBU=		$(LIB)/dbu
DECOMP=		$(LIB)/decomp
EQUEL=		$(LIB)/equel
GUTIL=		$(LIB)/gutil
IUTIL=		$(LIB)/iutil
MONITOR=	$(LIB)/monitor
OVQP=		$(LIB)/ovqp
PARSER=		$(LIB)/parser
QRYMOD=		$(LIB)/qrymod
SCANNER=	$(LIB)/scanner
SUPPORT=	$(LIB)/support