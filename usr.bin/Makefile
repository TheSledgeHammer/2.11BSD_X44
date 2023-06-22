#	$NetBSD: Makefile,v 1.235 2021/05/07 21:51:20 nia Exp $
#	from: @(#)Makefile	8.3 (Berkeley) 1/7/94

.include <bsd.own.mk>

SUBDIR= at bzip2 c11 c89 c99 cap_mkdb cksum crunch ctags db elf2aout elf2ecoff genassym gencat gprof hexdump 	\
	join ldd lorder	m4 make man menuc mkcsmapper mkdep mkesdb mklocale mktemp msgc rpcgen sed sortinfo 	\
	stat tic time touch tsort uudecode uuencode vgrind write xinstall xlint
												
SUBDIR+= ../contrib/zlib/pigz/bin/pigz

.if (${MKBSDGREP} != "no")
SUBDIR+= grep
.endif

.if (${MKPIGZGZIP} != "no")
# nothing
.else
SUBDIR+= gzip 
.endif

.include <bsd.subdir.mk>
