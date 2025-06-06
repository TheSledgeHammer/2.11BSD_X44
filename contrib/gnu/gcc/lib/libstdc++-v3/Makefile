#	$NetBSD: Makefile,v 1.50 2020/09/05 12:19:25 mrg Exp $

REQUIRETOOLS=	yes
NOLINT=		# defined
UNSUPPORTED_COMPILER.clang=	# defined

.include <bsd.init.mk>

.include "Makefile.inc"

LIB=		stdc++

LIBDPLIBS+=	m ${.CURDIR}/../../../../../usr.lib/libm

#COPTS+=		-pthread
#LDFLAGS+=	-pthread
.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
COPTS.random.cc+=	${${ACTIVE_CC} == "clang":? -mrdrnd :}
.endif

CSHLIBFLAGS+=	-D_GLIBCXX_COMPAT_

DIST=		${GCCDIST}
GNUVER=		${DIST}/libstdc++-v3/config/abi/pre/gnu.ver
SYMBOLS=	libstdc++-symbols.ver
.if ${MKPIC} != "no"
${SYMBOLS}: Makefile ${GNUVER}
	cat ${GNUVER} ${G_port_specific_symbol_files} | \
	${TOOL_GREP} -E -v '^[     ]*#(#| |$$)' | \
	${CXX} -I${DESTDIR}/usr/include/g++ -E -P -include ${.CURDIR}/arch/${LIBSTDCXX_MACHINE_ARCH}/symver-config.h - > \
	${.TARGET}.tmp && mv ${.TARGET}.tmp ${.TARGET} && rm -f ${.TARGET}.tmp

CLEANFILES+=	${SYMBOLS} ${SYMBOLS}.tmp

DPADD+=		${SYMBOLS}

.if ${MKPICLIB} != "no"
libstdc++_pic.a:: ${SYMBOLS}
.else
libstdc++.a:: ${SYMBOLS}
.endif

LDFLAGS+=	-Wl,-O1 \
		-Wl,--gc-sections \
		-Wl,--version-script=${SYMBOLS}
.endif

CXXFLAGS.clang+=	-stdlib=libstdc++ -std=c++11 -D_GLIBCXX_ABI_TAG_CXX11=
CXXFLAGS+=	${CXXFLAGS.${ACTIVE_CC}}
CWARNFLAGS.clang+=	-Wno-logical-op-parentheses \
			-Wno-deprecated-writable-strings -Wno-parentheses

.include "${.CURDIR}/../libsupc++/Makefile.common"

.if exists(${.CURDIR}/arch/${LIBSTDCXX_MACHINE_ARCH}/defs.mk) && ${MKGCC} != "no"
.include "${.CURDIR}/arch/${LIBSTDCXX_MACHINE_ARCH}/defs.mk"

SHLIB_MAJOR=	9
SHLIB_MINOR=	0

SUBDIR=		include

SRCS=		${LIBSUPCXXSRCS} ${LIBSTDCXXSRCS}

GCC_MACHINE_SUBDIR=${MACHINE_CPU:C/powerpc.*/rs6000/:C/x86_64/i386/}

CXXFLAGS+=	-fno-implicit-templates
CPPFLAGS+=	-I${DIST}/libstdc++-v3/include
CPPFLAGS+=	-I${DIST}/gcc/config/${GCC_MACHINE_SUBDIR}
CPPFLAGS+=	-I${.CURDIR}/arch/${LIBSTDCXX_MACHINE_ARCH}
CPPFLAGS+=	-D_GLIBCXX_SHARED -DGTHREAD_USE_WEAK -DSUPPORTS_WEAK
.if ${G_CLOCALE_CC:M*dragonfly*} != ""
CPPFLAGS+=	-I${CONF}/locale/dragonfly
LOCALETYPE=	dragonfly
.else
LOCALETYPE=	generic
.endif

# affects profiling; can't switch it off just for profiling easily.
#CXXFLAGS+=	${G_SECTION_FLAGS}
#CXXFLAGS+=	-ffunction-sections -fdata-sections 
COPTS.bitmap_allocator.cc+=	-ffunction-sections -fdata-sections 
COPTS.pool_allocator.cc+=	-ffunction-sections -fdata-sections 

CONF=		${DIST}/libstdc++-v3/config

GLIBCPP_INCLUDE_DIR=	${DESTDIR}/usr/include/g++

# Template instantiation bug workaround for -O<2/s
COPTS.cp-demangle.c += -Wno-stack-protector
COPTS.cp-demangle.c += -Wno-stack-protector
COPTS.fstream-inst.cc += -Wno-stack-protector
COPTS.locale-inst.cc += -Wno-stack-protector
COPTS.misc-inst.cc += -Wno-stack-protector
COPTS.ostream-inst.cc += -Wno-stack-protector
COPTS.functexcept.cc += -Wno-stack-protector
COPTS.valarray-inst.cc += -Wno-stack-protector
COPTS.wlocale-inst.cc += -Wno-stack-protector
COPTS.snprintf_lite.cc += -Wno-stack-protector
# From system headers.
COPTS.cxx11-wlocale-inst.cc +=-Wno-stack-protector
COPTS.cxx11-locale-inst.cc +=-Wno-stack-protector
COPTS.cxx11-wlocale-inst.cc +=-Wno-stack-protector
COPTS.codecvt_members.cc +=-Wno-stack-protector
COPTS.compatibility-thread-c++0x.cc += -Wno-deprecated-declarations
COPTS.concept-inst.cc += -fimplicit-templates
COPTS.collate_members_cow.cc += -fimplicit-templates
COPTS.messages_members_cow.cc += -fimplicit-templates
COPTS.monetary_members_cow.cc += -fimplicit-templates
COPTS.numeric_members_cow.cc += -fimplicit-templates
COPTS.hashtable_c++0x.cc += -fimplicit-templates
COPTS.c++locale.cc += -fimplicit-templates
COPTS.functexcept.cc += -Wno-deprecated-declarations
COPTS.future.cc += -Wno-deprecated-declarations
COPTS.shared_ptr.cc += -Wno-deprecated-declarations
COPTS.thread.cc += -Wno-deprecated-declarations
CPPFLAGS.strstream.cc += -I$(GLIBCPP_INCLUDE_DIR)/backward -Wno-deprecated

CPPFLAGS.cp-demangle.cc += -DIN_GLIBCPP_V3
CPPFLAGS.concept-inst.cc += -D_GLIBCXX_CONCEPT_CHECKS
CPPFLAGS.parallel_list.cc += -D_GLIBCXX_PARALLEL
CPPFLAGS.parallel_settings.cc += -D_GLIBCXX_PARALLEL
CPPFLAGS.compatibility-parallel_list.cc += -D_GLIBCXX_PARALLEL
CPPFLAGS.compatibility.cc+=	-I${DIST}/libstdc++-v3/config

cp-demangle.d cp-demangle.o: cp-demangle.h

.if ${MACHINE_ARCH} == "i386"
COPTS.random.cc += -Wno-error=psabi
COPTS.cow-string-inst.cc += -Wno-error=psabi
.endif

# XXX
MKDEPFLAGS+=	-I$(GLIBCPP_INCLUDE_DIR)/backward

GLIBCXX_SRCDIR=	${DIST}/libstdc++-v3

BUILDSYMLINKS+=	\
	${GLIBCXX_SRCDIR}/${G_CCODECVT_CC} codecvt_members.cc \
	${GLIBCXX_SRCDIR}/${G_CCOLLATE_CC} collate_members.cc \
	${GLIBCXX_SRCDIR}/${G_CCTYPE_CC} ctype_members.cc \
	${GLIBCXX_SRCDIR}/${G_CMESSAGES_CC} messages_members.cc \
	${GLIBCXX_SRCDIR}/${G_CMONEY_CC} monetary_members.cc \
	${GLIBCXX_SRCDIR}/${G_CNUMERIC_CC} numeric_members.cc \
	${GLIBCXX_SRCDIR}/${G_CTIME_CC} time_members.cc \
	${GLIBCXX_SRCDIR}/${G_ATOMICITY_SRCDIR}/atomicity.h atomicity.cc \
	${GLIBCXX_SRCDIR}/${G_CLOCALE_CC} c++locale.cc \
	${GLIBCXX_SRCDIR}/${G_BASIC_FILE_CC} basic_file.cc \
	${DIST}/libiberty/cp-demangle.c cp-demangle.c \
	${DIST}/libiberty/cp-demangle.h cp-demangle.h \
	${GLIBCXX_SRCDIR}/${G_CCOLLATE_CC} collate_members_cow.cc \
	${GLIBCXX_SRCDIR}/${G_CMESSAGES_CC} messages_members_cow.cc \
	${GLIBCXX_SRCDIR}/${G_CMONEY_CC} monetary_members_cow.cc \
	${GLIBCXX_SRCDIR}/${G_CNUMERIC_CC} numeric_members_cow.cc

# both c++98 and c++11 have codecvt.cc files.
BUILDSYMLINKS+=	\
	${GLIBCXX_SRCDIR}/src/c++98/codecvt.cc c98-codecvt.cc
BUILDSYMLINKS+=	\
	${GLIBCXX_SRCDIR}/src/c++11/codecvt.cc c11-codecvt.cc
SRCS+=	c98-codecvt.cc c11-codecvt.cc

.include "../Makefile.gthr"

# XXX Special rules in c++98/Makefile; may move into c++11 in future GCC.
CXX11_ALWAYS=	localename.cc \
		locale_init.cc

FCHAR8_SRCS+=	localename.cc \
		locale_init.cc \
		c11-codecvt.cc \
		limits.cc

.for _s in ${G_cxx11_sources} ${CXX11_ALWAYS}
COPTS.${_s}+= -std=gnu++11
.endfor

.for _s in ${G_cxx98_sources}
COPTS.${_s}+= -std=gnu++98
.endfor

.for _s in ${FCHAR8_SRCS}
COPTS.${_s}+= -fchar8_t
.endfor

COPTS.cp-demangle.c += -Wno-unused-function

COPTS.ext-inst.cc+= -Wno-error

.for f in collate_members_cow \
	  messages_members_cow \
          monetary_members_cow \
	  numeric_members_cow \
	  c++locale
COPTS.${f}.cc+= -fimplicit-templates ${G_GLIBCXX_ABI_FLAGS}
.endfor

.include <bsd.lib.mk>
.include <bsd.subdir.mk>
.include <bsd.info.mk>

.PATH:	${DIST}/libstdc++-v3/src \
	${DIST}/libstdc++-v3/src/c++98 \
	${DIST}/libstdc++-v3/src/c++11 \
	${DIST}/libstdc++-v3/src/c++17 \
	${DIST}/libstdc++-v3/src/filesystem \
	${DIST}/libstdc++-v3/libsupc++ \
	${DIST}/libstdc++-v3/config/os/bsd/netbsd \
	${CONF}/locale/${LOCALETYPE} \
	${.CURDIR}/arch/${LIBSTDCXX_MACHINE_ARCH}

${OBJS}: ${.CURDIR}/arch/${LIBSTDCXX_MACHINE_ARCH}/defs.mk

.else
.include <bsd.prog.mk> # do nothing
.endif
