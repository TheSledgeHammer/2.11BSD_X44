.include <ingres.mk>

.PATH: 	${.CURDIR}/ingres/source/equel/runtime

SRCS+=	IIw_left.c IIw_right.c IIcvar.c IIgettup.c IIn_get.c IIn_ret.c 		\
		IIingres.c IIrdpipe.c IIretrieve.c IIsetup.c IIflushtup.c IIsync.c 	\
		IIwrite.c IIwrpipe.c IIexit.c IIbmove.c IIsyserr.c IIconvert.c 		\
		IIerror.c IIp_err.c IIgetc.c IIprpipe.c IIresync.c IIitos.c 		\
		IIconcatv.c IIbmove.c IIlength.c IIatoi.c IIsequal.c
