.include <ingres.mk>

.PATH: ${.CURDIR}/ingres/source/qrymod

LIBS=	$(PARSER) $(ACCESS) $(IUTIL) $(GUTIL)

SRCS+=	main.c readtree.c qrymod.c issue.c define.c d_view.c d_integ.c \
		d_prot.c fillprotup.c view.c integrity.c protect.c writetree.c \
		range.c tree.c trscan.c treepr.c util.c catalog_desc.c
