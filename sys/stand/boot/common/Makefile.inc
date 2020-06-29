#	$NetBSD: Makefile.inc,v 1.3 2016/08/04 16:22:40 scole Exp $

SRCS+=	bcache.c commands.c console.c devopen.c interp.c boot.c dev_net.c
SRCS+=	interp_backslash.c interp_parse.c ls.c misc.c
SRCS+=	panic.c readdir.c pager.c environment.c fileload.c load_exec.c ksyms.c
SRCS+=  getopt.c gets.c strdup.c strtol.c strspn.c

# XXX might need this for different loaders later
#.if defined(LOADER_NET_SUPPORT)
#SRCS+=	dev_net.c
#.endif
