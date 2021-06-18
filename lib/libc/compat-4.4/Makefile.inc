#	Makefile	1.6	1996/1/11
#

# compat-4.4 sources
.PATH: 	${ARCHDIR}/compat-4.4 ${.CURDIR}/compat-4.4
SRCS+= 	fgetln.c fpurge.c fts.c glob.c heapsort.c pwcache.c snprintf.c \
		strmode.c unvis.c vis.c vsnprintf.c

MAN+=	fgetln.3 fts.3 glob.3 pwcache.3 strmode.3 unvis.3 vis.3
