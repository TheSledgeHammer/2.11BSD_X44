
.PATH: ${.CURDIR}/ingres/source/ovqp

LIBS=	$(ACCESS) $(IUTIL) $(GUTIL)

SRCS+=	ovqp.c startovqp.c getqry.c rdsym.c strategy.c findsimps.c 		\
		key.c scan.c interp.c interp1.c equelpr.c string.c typecheck.c 	\
		support.c prsym.c copyreturn.c
