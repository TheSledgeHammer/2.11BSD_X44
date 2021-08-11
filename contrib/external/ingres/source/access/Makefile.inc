
.include <ingres.mk>

.PATH: ${.CURDIR}/ingres/source/access

SRCS=	opencatalog.c closecatalog.c catalog_desc.c inpcloser.c closer.c 	\
		noclose.o openr.c accbuf.c acc_addbuf.c batch.c batchxtra.c 		\
		writebatch.c getequal.c get.c insert.c replace.c delete.c setkey.c 	\
		find.c rhash.c ndxsearch.c findbest.c scan_dups.c get_tuple.c 		\
		put_tuple.c del_tuple.c tup_len.c newlino.c add_ovflo.c formatpg.c 	\
		last_page.c page.c compare.c add_prim.c utility.c cleanrel.c 		\
		readadmin.c
