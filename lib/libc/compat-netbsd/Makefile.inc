#	Makefile	1.6	1996/1/11
#

# compat-netbsd sources
.PATH: 	${ARCHDIR}/compat-netbsd ${.CURDIR}/compat-netbsd

SRCS+= 	atoll.c basename.c dprintf.c errc.c errx.c fparseln.c			\
		getopt_long.c mi_vector_hash.c mkdtemp.c mkstemp.c 				\
		reallocarr.c reallocarray.c										\
		strsuftoll.c strlcat.c strlcpy.c strncasecmp.c strnlen.c 		\
		strsuftoll.c strtoi.c strtoll.c strtou.c verr.c verrc.c verrx.c \
		vwarn.c vwarnc.c vwarnx.c warn.c warnc.c warnx.c

MAN+=	atoll.3 basename.3 fparseln.3 getopt_long.3 mi_vector_hash.3 	\
		mktemp.3 reallocarr.3 reallocarray.3 strsuftoll.3
