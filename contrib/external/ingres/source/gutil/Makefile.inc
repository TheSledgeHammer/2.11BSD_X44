.include <ingres.mk>

.PATH: ${.CURDIR}/ingres/source/gutil

SRCS+=	atof.c atoi.c atol.c bequal.c bitcnt.c bitpos.c bmove.c cat.c 			\
		concat.c dysize.c fgetline.c flush.c freebuf.c ftoa.c fullwait.c 		\
		getufield.c iocv.c itoa.c length.c locv.c longconst.c need.c oatoi.c 	\
		pad.c pmove.c prargs.c scompare.c sequal.c setflag.c smove.c syserr.c 	\
		tTf.c trace.c xputchar.c ztack.c
