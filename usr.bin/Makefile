#	$NetBSD: Makefile,v 1.235 2021/05/07 21:51:20 nia Exp $
#	from: @(#)Makefile	8.3 (Berkeley) 1/7/94

.include <bsd.own.mk>

SUBDIR= at 															\
		bzip2 														\
		c11 c89 c99 cap_mkdb cksum crontab crunch ctags 			\
		db 															\
		elf2aout elf2ecoff 											\
		genassym gencat getaddrinfo getopt getconf gettext gprof	\
		hexdump 													\
		join 														\
		kdump ktrace												\
		ldd locate lorder 											\
		m4 make man menuc mkdep mkerrlst mkfifo mklocale mkstr      \
        mktemp msgc 	                                            \
		rpcgen rpcinfo												\
		sed sortinfo stat strfile 									\
		time touch tsort 										\
		unstr uudecode uuencode 									\
		vmstat vgrind 														\
		write 														\
		xinstall xlint
												
SUBDIR+= ../contrib/zlib/pigz/bin/pigz

.if (${MKBSDGREP} != "no")
SUBDIR+= grep
.endif

.if (${MKPIGZGZIP} != "no")
# nothing
.else
SUBDIR+= gzip 
.endif

.if (${MKHESIOD} != "no")
SUBDIR+= hesinfo
.endif

.include <bsd.subdir.mk>
