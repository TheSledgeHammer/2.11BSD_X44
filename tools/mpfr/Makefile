#	$NetBSD: Makefile,v 1.2 2012/04/15 08:37:32 mrg Exp $
#

GNUHOSTDIST=	${.CURDIR}/../../contrib/gnu/mpfr/dist

CONFIGURE_ARGS+=	--with-gmp=${TOOLDIR:Q}

CONFIGURE_ARGS+=--disable-shared

.include "${.CURDIR}/../Makefile.gnuhost"

# Force avoiding possibly non-executable install-sh.
CONFIGURE_ENV+= ac_cv_path_mkdir="${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-install -d"
