#	Makefile	1.6	1996/1/11
#

# compat-netbsd sources
.PATH: 	${ARCHDIR}/compat-netbsd ${.CURDIR}/compat-netbsd

SRCS+= 	asprintf.c basename.c dirname.c efun.c fparseln.c getcwd.c getopt_long.c 			\
		gettemp.c pw_scan.c pwcache.c reallocarr.c reallocarray.c 				\
		stat_flags.c strlcat.c strlcpy.c strndup.c strnlen.c strtoimax.c 		\
		strtoll.c strtoumax.c strtouq.c vasprintf.c

MAN+=	basename.3 dirname.3 efun.3 fparseln.3 getcwd.3 getopt_long.3 reallocarr.3 		\
		reallocarray.3 stat_flags.3



