# $FreeBSD$

.include <bsd.loader.mk>

INCSDIR=    /usr/include

SRCS+=		boot.c commands.c console.c devopen.c environment.c 	\
			fileload.c getopt.c install.c interp_backslash.c 		\
			interp_parse.c interp.c load_exec.c load_ksyms.c ls.c 	\
			misc.c pager.c panic.c readdir.c smbios.c ufsread.c		\
			uuid_from_string.c uuid_to_string.c 
			
.if ${LOADER_INTERP} == "dloader"
SRCS+=	interp_dloader.c
.else
.error Unknown interpreter ${LOADER_INTERP}
.endif

.if ${LOADER_NET_SUPPORT} == "yes"
SRCS+= 	dev_net.c
#INCS+=	dev_net.h
.endif

.if ${HAVE_BCACHE} == "yes"
SRCS+=  bcache.c
.endif

# Machine-independant ISA PnP
.if ${HAVE_ISABUS} == "yes"
SRCS+=	isapnp.c
#INCS+= 	isapnp.h
.endif

.if ${HAVE_PNP} == "yes"
SRCS+=	pnp.c
.endif

#INCS=		boot2.h bootstrap.h commands.h paths.h rbx.h smbios.h

#.include <bsd.inc.mk>
