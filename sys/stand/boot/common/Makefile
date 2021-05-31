# $FreeBSD$

.include <loader.mk>

INCSDIR= 	/usr/include/stand/boot/common

SRCS+=		boot.c commands.c console.c devopen.c environment.c 					\
			fileload.c getopt.c interp_backslash.c interp_parse.c interp.c 			\
			load_exec.c load_ksyms.c ls.c misc.c pager.c							\
			panic.c readdir.c smbios.c strdup.c strspn.c strtol.c ufsread.c

INCS=		boot2.h bootstrap.h commands.h dev_net.h isapnp.h paths.h				\
			rbx.h smbios.h

.include <bsd.kinc.mk>