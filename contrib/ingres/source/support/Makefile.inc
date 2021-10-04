.include <ingres.mk>

.PATH: ${.CURDIR}/ingres/source/support

LIBS=	$(DBU) $(ACCESS) $(IUTIL) $(GUTIL)

SRCS+=	error.c creatdb.c destroydb.c purge.c helpr.c ingres.c printr.c \
		restore.c sysmod.c usersetup.c
