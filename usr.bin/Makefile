#	$NetBSD: Makefile,v 1.235 2021/05/07 21:51:20 nia Exp $
#	from: @(#)Makefile	8.3 (Berkeley) 1/7/94

.include <bsd.own.mk>

SUBDIR= apply ar asa at 											\
		banner basename bzip2 bzip2recover							\
		c11 c89 c99 cap_mkdb cksum cmp col colcrt colrm column comm \
		compress crontab crunch ctags cut 							\
		db deroff dirname du												\
		elf2aout elf2ecoff env expand								\
		false fdformat find fmt fold fpr fsplit                     \
		gcore genassym gencat getaddrinfo getopt getconf gettext 	\
		gprof														\
		head hexdump 												\
        id indent iostat                                                   \
		join 														\
		kdump ktrace												\
		ldd locate logname look lorder 								\
		m4 make man menuc mkdep mkerrlst mkfifo mklocale mkstr      \
        mktemp msgc msgs                                            \
        netgroup netstat                                            \
        paste patch pathchk pr printenv printf						\
		rpcgen rpcinfo												\
		script sed shar sort sortinfo spell split stat strfile su					\
		tail time tip touch true tsort 								\
		uname unexpand users unstr unzip uudecode uuencode 				\
		vgrind vis vmstat 											\
		w wall wc what whereis who whois write 						\
		xargs xinstall xlint xstr                                   \
        yes
												
SUBDIR+= ../contrib/zlib/pigz/bin/pigz

.if (${MKGROFF} == "no")
SUBDIR+= soelim
.endif

.if (${MKMAKEMANDB} == "no")
SUBDIR+= apropos whatis
.endif

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

.if (${MKSKEY} != "no")
SUBDIR+= skey skeyinfo skeyinit
.endif

.if (${MKYP} != "no")
SUBDIR+= ypcat ypmatch ypwhich
.endif

SUBDIR+= bdes

.include <bsd.subdir.mk>
