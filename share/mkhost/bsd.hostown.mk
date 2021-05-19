#	$NetBSD: bsd.own.mk,v 1.1247 2021/05/06 13:23:36 rin Exp $
#
# Determine if running in the NetBSD source tree by checking for the
# existence of build.sh and tools/ in the current or a parent directory,
# and setting _SRC_TOP_ to the result.
#
.if !defined(_SRC_TOP_)			# {
_SRC_TOP_!= cd ${.CURDIR}; while :; do \
		here=`pwd`; \
		[ -f build.sh  ] && [ -d tools ] && { echo $$here; break; }; \
		case $$here in /) echo ""; break;; esac; \
		cd ..; done

.MAKEOVERRIDES+=	_SRC_TOP_

.endif					# }

#
# If _SRC_TOP_ != "", we're within the NetBSD source tree, so set
# defaults for NETBSDSRCDIR and _SRC_TOP_OBJ_.
#
.if (${_SRC_TOP_} != "")		# {

NETBSDSRCDIR?=	${_SRC_TOP_}

.if !defined(_SRC_TOP_OBJ_)
_SRC_TOP_OBJ_!=		cd ${_SRC_TOP_} && ${PRINTOBJDIR}
.MAKEOVERRIDES+=	_SRC_TOP_OBJ_
.endif

.endif	# _SRC_TOP_ != ""		# }


.if (${_SRC_TOP_} != "") && \
    (${TOOLCHAIN_MISSING} == "no" || defined(EXTERNAL_TOOLCHAIN))
USETOOLS?=	yes
.endif
USETOOLS?=	no

#
# Host platform information; may be overridden
#
.if !defined(HOST_OSTYPE)
_HOST_OSNAME!=	uname -s
_HOST_OSREL!=	uname -r
_HOST_ARCH!=	uname -p 2>/dev/null || uname -m
HOST_OSTYPE:=	${_HOST_OSNAME}-${_HOST_OSREL:C/\([^\)]*\)//g:[*]:C/ /_/g}-${_HOST_ARCH:C/\([^\)]*\)//g:[*]:C/ /_/g}
.MAKEOVERRIDES+= HOST_OSTYPE
.endif
HOST_CYGWIN=	${HOST_OSTYPE:MCYGWIN*}

.if ${USETOOLS} == "yes"						# {

#
# Provide a default for TOOLDIR.
#
.if !defined(TOOLDIR)
TOOLDIR:=	${_SRC_TOP_OBJ_}/tooldir.${HOST_OSTYPE}
.MAKEOVERRIDES+= TOOLDIR
.endif

#
# This is the prefix used for the NetBSD-sourced tools.
#
_TOOL_PREFIX?=	nb

#
# If an external toolchain base is specified, use it.
#
.if defined(EXTERNAL_TOOLCHAIN)						# {
AR=						${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-ar
AS=						${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-as
LD=						${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-ld
NM=						${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-nm
OBJCOPY=				${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-objcopy
OBJDUMP=				${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-objdump
RANLIB=					${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-ranlib
SIZE=					${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-size
STRIP=					${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-strip

CC=						${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-gcc
CPP=					${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-cpp
CXX=					${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-c++
FC=						${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-g77
OBJC=					${EXTERNAL_TOOLCHAIN}/bin/${MACHINE_GNU_PLATFORM}-gcc
.else									# } {
# Define default locations for common tools.
.if ${USETOOLS_BINUTILS:Uyes} == "yes"					#  {
AR=						${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-ar
AS=						${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-as
LD=						${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-ld
NM=						${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-nm
OBJCOPY=				${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-objcopy
OBJDUMP=				${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-objdump
RANLIB=					${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-ranlib
SIZE=					${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-size
STRIP=					${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-strip
.endif									#  }

.if ${USETOOLS_GCC:Uyes} == "yes"					#  {
CC=						${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-gcc
CPP=					${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-cpp
CXX=					${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-c++
FC=						${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-g77
OBJC=					${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-gcc
.endif									#  }
.endif	# EXTERNAL_TOOLCHAIN						# }

HOST_MKDEP=				${TOOLDIR}/bin/${_TOOL_PREFIX}host-mkdep

DBSYM=					${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-dbsym
ELF2ECOFF=				${TOOLDIR}/bin/${_TOOL_PREFIX}mips-elf2ecoff
INSTALL=				STRIP=${STRIP:Q} ${TOOLDIR}/bin/${_TOOL_PREFIX}install
LEX=					${TOOLDIR}/bin/${_TOOL_PREFIX}lex
LINT=					CC=${CC:Q} ${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-lint
LORDER=					NM=${NM:Q} MKTEMP=${TOOL_MKTEMP:Q} ${TOOLDIR}/bin/${_TOOL_PREFIX}lorder
MKDEP=					CC=${CC:Q} ${TOOLDIR}/bin/${_TOOL_PREFIX}mkdep
TSORT=					${TOOLDIR}/bin/${_TOOL_PREFIX}tsort -q
YACC=					${TOOLDIR}/bin/${_TOOL_PREFIX}yacc

TOOL_AMIGAAOUT2BB=		${TOOLDIR}/bin/${_TOOL_PREFIX}amiga-aout2bb
TOOL_AMIGAELF2BB=		${TOOLDIR}/bin/${_TOOL_PREFIX}amiga-elf2bb
TOOL_AMIGATXLT=			${TOOLDIR}/bin/${_TOOL_PREFIX}amiga-txlt
TOOL_ASN1_COMPILE=		${TOOLDIR}/bin/${_TOOL_PREFIX}asn1_compile
TOOL_BEBOXELF2PEF=		${TOOLDIR}/bin/${_TOOL_PREFIX}bebox-elf2pef
TOOL_BEBOXMKBOOTIMAGE=	${TOOLDIR}/bin/${_TOOL_PREFIX}bebox-mkbootimage
TOOL_CAP_MKDB=			${TOOLDIR}/bin/${_TOOL_PREFIX}cap_mkdb
TOOL_CAT=				${TOOLDIR}/bin/${_TOOL_PREFIX}cat
TOOL_CKSUM=				${TOOLDIR}/bin/${_TOOL_PREFIX}cksum
TOOL_COMPILE_ET=		${TOOLDIR}/bin/${_TOOL_PREFIX}compile_et
TOOL_CONFIG=			${TOOLDIR}/bin/${_TOOL_PREFIX}config
TOOL_CRUNCHGEN=			MAKE=${.MAKE:Q} ${TOOLDIR}/bin/${_TOOL_PREFIX}crunchgen
TOOL_CTAGS=				${TOOLDIR}/bin/${_TOOL_PREFIX}ctags
TOOL_DB=				${TOOLDIR}/bin/${_TOOL_PREFIX}db
TOOL_EQN=				${TOOLDIR}/bin/${_TOOL_PREFIX}eqn
TOOL_FGEN=				${TOOLDIR}/bin/${_TOOL_PREFIX}fgen
TOOL_GENCAT=			${TOOLDIR}/bin/${_TOOL_PREFIX}gencat
TOOL_GROFF=				PATH=${TOOLDIR}/lib/groff:$${PATH} ${TOOLDIR}/bin/${_TOOL_PREFIX}groff
TOOL_HEXDUMP=			${TOOLDIR}/bin/${_TOOL_PREFIX}hexdump
TOOL_HP300MKBOOT=		${TOOLDIR}/bin/${_TOOL_PREFIX}hp300-mkboot
TOOL_INDXBIB=			${TOOLDIR}/bin/${_TOOL_PREFIX}indxbib
TOOL_INSTALLBOOT=		${TOOLDIR}/bin/${_TOOL_PREFIX}installboot
TOOL_INSTALL_INFO=		${TOOLDIR}/bin/${_TOOL_PREFIX}install-info
TOOL_M4=				${TOOLDIR}/bin/${_TOOL_PREFIX}m4
TOOL_MACPPCFIXCOFF=		${TOOLDIR}/bin/${_TOOL_PREFIX}macppc-fixcoff
TOOL_MAKEFS=			${TOOLDIR}/bin/${_TOOL_PREFIX}makefs
TOOL_MAKEINFO=			${TOOLDIR}/bin/${_TOOL_PREFIX}makeinfo
TOOL_MAKEWHATIS=		${TOOLDIR}/bin/${_TOOL_PREFIX}makewhatis
TOOL_MDSETIMAGE=		${TOOLDIR}/bin/${MACHINE_GNU_PLATFORM}-mdsetimage
TOOL_MENUC=				MENUDEF=${TOOLDIR}/share/misc ${TOOLDIR}/bin/${_TOOL_PREFIX}menuc
TOOL_MIPSELF2ECOFF=		${TOOLDIR}/bin/${_TOOL_PREFIX}mips-elf2ecoff
TOOL_MKCSMAPPER=		${TOOLDIR}/bin/${_TOOL_PREFIX}mkcsmapper
TOOL_MKESDB=			${TOOLDIR}/bin/${_TOOL_PREFIX}mkesdb
TOOL_MKLOCALE=			${TOOLDIR}/bin/${_TOOL_PREFIX}mklocale
TOOL_MKMAGIC=			${TOOLDIR}/bin/${_TOOL_PREFIX}file
TOOL_MKTEMP=			${TOOLDIR}/bin/${_TOOL_PREFIX}mktemp
TOOL_MSGC=				MSGDEF=${TOOLDIR}/share/misc ${TOOLDIR}/bin/${_TOOL_PREFIX}msgc
TOOL_MTREE=				${TOOLDIR}/bin/${_TOOL_PREFIX}mtree
TOOL_PAX=				${TOOLDIR}/bin/${_TOOL_PREFIX}pax
TOOL_PIC=				${TOOLDIR}/bin/${_TOOL_PREFIX}pic
TOOL_PREPMKBOOTIMAGE=	${TOOLDIR}/bin/${_TOOL_PREFIX}prep-mkbootimage
TOOL_PWD_MKDB=			${TOOLDIR}/bin/${_TOOL_PREFIX}pwd_mkdb
TOOL_REFER=				${TOOLDIR}/bin/${_TOOL_PREFIX}refer
TOOL_ROFF_ASCII=		PATH=${TOOLDIR}/lib/groff:$${PATH} ${TOOLDIR}/bin/${_TOOL_PREFIX}nroff
TOOL_ROFF_DVI=			${TOOL_GROFF} -Tdvi
TOOL_ROFF_HTML=			${TOOL_GROFF} -Tlatin1 -mdoc2html
TOOL_ROFF_PS=			${TOOL_GROFF} -Tps
TOOL_ROFF_RAW=			${TOOL_GROFF} -Z
TOOL_RPCGEN=			CPP=${CPP:Q} ${TOOLDIR}/bin/${_TOOL_PREFIX}rpcgen
TOOL_SOELIM=			${TOOLDIR}/bin/${_TOOL_PREFIX}soelim
TOOL_STAT=				${TOOLDIR}/bin/${_TOOL_PREFIX}stat
TOOL_SPARKCRC=			${TOOLDIR}/bin/${_TOOL_PREFIX}sparkcrc
TOOL_SUNLABEL=			${TOOLDIR}/bin/${_TOOL_PREFIX}sunlabel
TOOL_TBL=				${TOOLDIR}/bin/${_TOOL_PREFIX}tbl
TOOL_UUDECODE=			${TOOLDIR}/bin/${_TOOL_PREFIX}uudecode
TOOL_VGRIND=			${TOOLDIR}/bin/${_TOOL_PREFIX}vgrind -f
TOOL_ZIC=				${TOOLDIR}/bin/${_TOOL_PREFIX}zic

.endif	# USETOOLS == yes						# }