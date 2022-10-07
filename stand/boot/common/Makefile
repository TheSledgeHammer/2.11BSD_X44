# $FreeBSD$

.include <bsd.loader.mk>

INCSDIR= 	/usr/include/stand/boot/common

SRCS+=		boot.c commands.c console.c devopen.c environment.c 	\
			fileload.c getopt.c install.c interp_backslash.c 		\
			interp_parse.c interp.c load_exec.c load_ksyms.c ls.c 	\
			misc.c pager.c panic.c readdir.c smbios.c ufsread.c
			
.if ${LOADER_INTERP} == "dloader"
SRCS+=	interp_dloader.c
.elif ${LOADER_INTERP} == "simp"
SRCS+=	interp_simple.c
.else
.error Unknown interpreter ${LOADER_INTERP}
.endif

INCS=		boot2.h bootstrap.h commands.h paths.h rbx.h smbios.h

.if ${LOADER_NET_SUPPORT} == "yes"
SRCS+= 	dev_net.c
INCS+=	dev_net.h
.endif

.if ${HAVE_BCACHE} == "yes"
SRCS+=  bcache.c
.endif

# Machine-independant ISA PnP
.if ${HAVE_ISABUS} == "yes"
SRCS+=	isapnp.c
INCS+= 	isapnp.h
.endif

.if ${HAVE_PNP} == "yes"
SRCS+=	pnp.c
.endif

.include <bsd.kinc.mk>
