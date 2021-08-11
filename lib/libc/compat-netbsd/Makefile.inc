#	Makefile	1.6	1996/1/11
#

# compat-netbsd sources
.PATH: 	${ARCHDIR}/compat-netbsd ${.CURDIR}/compat-netbsd

SRCS+= 	atoll.c basename.c dirname.c dprintf.c efun.c errc.c errx.c fparseln.c	\
		getopt_long.c gettemp.c mkdtemp.c mkstemp.c pw_scan.c 					\
		raise_default_signal.c reallocarr.c reallocarray.c snprintb.c 			\
		stat_flags.c strsuftoll.c strlcat.c strlcpy.c strncasecmp.c strndup.c 	\
		strnlen.c strsuftoll.c strtoi.c strtoll.c strtou.c strtouq. verr.c 		\
		verrc.c verrx.c vwarn.c vwarnc.c vwarnx.c warn.c warnc.c warnx.c

MAN+=	atoll.3 basename.3 dirname.3 efun.3 fparseln.3 getopt_long.3 			\
		mktemp.3 raise_default_signal.3 reallocarr.3 reallocarray.3 snprintb.3 	\
		stat_flags.3 strsuftoll.3
