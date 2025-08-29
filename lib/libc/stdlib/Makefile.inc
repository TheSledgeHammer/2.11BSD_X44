#
#	@(#)Makefile	1.0 (2.11BSD GTE) 1995/1/12
#
# This is the Makefile for 'stdlib'.  New routines ported from 4.4BSD's
# libc/stdlib directory go here but existing libc/gen files are being left
# where they are.
#

# stdlib sources
.PATH: ${ARCHDIR}/stdlib ${.CURDIR}/stdlib

SRCS+=	a64l.c abort.c abs.c aligned_alloc.c atexit.c atof.c atoi.c atol.c bsearch.c div.c drand48.c 			\
	    erand48.c exit.c freezero.c getenv.c getopt.c getopt_long.c getsubopt.c hcreate.c heapsort.c       		\
        jrand48.c l64a.c labs.c lcong48.c ldiv.c llabs.c lldiv.c lrand48.c merge.c  \
	    mi_vector_hash.c mrand48.c nrand48.c putenv.c qabs.c qdiv.c qsort.c radixsort.c rand48.c random.c	\
	    reallocarr.c reallocarray.c recallocarray.c seed48.c setenv.c srand48.c strsuftoll.c strtoi.c 			\
        strtoimax.c strtol.c strtoq.c strtoll.c strtou.c strtoul.c strtoull.c strtoumax.c strtouq.c strtonum.c \
        system.c tdelete.c tfind.c tsearch.c twalk.c

.if (${USE_JEMALLOC} == "no")
SRCS+= calloc.c malloc.c
.endif

.include "${ARCHDIR}/stdlib/Makefile.inc"

MAN+= 	a64l.3 abort.3 abs.3 atexit.3 atof.3 atoi.3 atol.3 bsearch.3 calloc.3 div.3 exit.3	\
	    getopt_long.3 getopt.3 getsubopt.3 hcreate.3 labs.3 ldiv.3 llabs.3 lldiv.3 malloc.3     \
        mi_vector_hash.3 posix_memalign.3 qabs.3 qdiv.3 qsort.3 radixsort.3 rand48.3 random.3 \
		reallocarr.3 reallocarray.3 strtol.3 strtoul.3 strsuftoll.3 strtod.3 strtonum.3 system.3 \
        tsearch.3

MLINKS+=a64l.3 l64a.3
MLINKS+=a64l.3 l64a_r.3
MLINKS+=hcreate.3 hdestroy.3 hcreate.3 hsearch.3
MLINKS+=malloc.3 freezero.3
MLINKS+=posix_memalign.3 aligned_alloc.3
MLINKS+=reallocarray.3 recallocarray.3
MLINKS+=rand.3 rand_r.3
MLINKS+=rand.3 srand.3
MLINKS+=rand48.3 drand48.3 rand48.3 erand48.3 rand48.3 lrand48.3
MLINKS+=rand48.3 mrand48.3 rand48.3 nrand48.3 rand48.3 jrand48.3
MLINKS+=rand48.3 srand48.3 rand48.3 seed48.3 rand48.3 lcong48.3
MLINKS+=random.3 initstate.3 random.3 setstate.3 random.3 srandom.3
MLINKS+=radixsort.3 sradixsort.3
MLINKS+=strsuftoll.3 strsuftollx.3
MLINKS+=strtod.3 strtof.3 strtod.3 strtold.3
MLINKS+=strtol.3 strtoimax.3
MLINKS+=strtol.3 strtoll.3
MLINKS+=strtol.3 strtoq.3
MLINKS+=strtoul.3 strtoull.3
MLINKS+=strtoul.3 strtoumax.3
MLINKS+=strtoul.3 strtouq.3
MLINKS+=tsearch.3 tfind.3 tsearch.3 twalk.3 tsearch.3 tdelete.3
