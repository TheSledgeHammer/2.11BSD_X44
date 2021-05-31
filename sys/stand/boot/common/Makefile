# $FreeBSD$

.include <bsd.loader.mk>

INCSDIR= 	/usr/include/stand/boot/common

SRCS+=		boot.c commands.c console.c devopen.c environment.c 					\
			fileload.c getopt.c interp_backslash.c interp_parse.c interp.c 			\
			load_exec.c load_ksyms.c ls.c misc.c pager.c							\
			panic.c readdir.c smbios.c strdup.c strspn.c strtol.c ufsread.c

INCS=		boot2.h bootstrap.h commands.h dev_net.h isapnp.h paths.h				\
			rbx.h smbios.h

.if defined(HAVE_BCACHE)
SRCS+=  bcache.c
.endif

# Machine-independant ISA PnP
.if defined(HAVE_ISABUS)
SRCS+=	isapnp.c
.endif
.if defined(HAVE_PNP)
SRCS+=	pnp.c
.endif

.include <bsd.kinc.mk>