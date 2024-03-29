#	$NetBSD: Makefile.inc,v 1.34.2.2.2.1 2005/07/24 02:25:24 snj Exp $
#
# Makefile for sysinst

PROG=		sysinst
NOMAN=		# defined

WARNS=		3

SRCS+=	menu_defs.c msg_defs.c main.c install.c upgrade.c \
	txtwalk.c run.c factor.c net.c disks.c disks_lfs.c util.c geom.c \
	label.c target.c md.c sizemultname.c

SRCS+=	${MD_OPTIONS:MAOUT2ELF:S/AOUT2ELF/aout2elf.c/}
SRCS+=	${MENUS_MD:Mmenus.mbr:S/menus.mbr/mbr.c/}
SRCS+=	${CPPFLAGS:M-DWSKBD:S/-DWSKBD/wskbd.c/}
SRCS+=	${NODISKLABEL:D:Ubsddisklabel.c savenewlabel.c}

SYSINSTLANG?=	en
LANGUAGES?=	de en fr pl

MSG_MD?=	msg.md.${SYSINSTLANG}
MENUS_MD?=	menus.md.${SYSINSTLANG}

.include <bsd.sys.mk>		# for HOST_SH
.include <bsd.own.mk>		# for mk.conf
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

_MKSHTARGET_CREATE?=	${_MKSHMSG_CREATE} ${.CURDIR:T}/${.TARGET}

DPADD=		${LIBCURSES} ${LIBTERMCAP} ${LIBUTIL}
LDADD=		-lcurses -ltermcap -lutil

UNIF_AWK=	${.CURDIR}/../../unif.awk
MSG_XLAT_SH=	${.CURDIR}/../../msg_xlat.sh

CPPFLAGS+=	-I. -I${.CURDIR}/../.. -I${.CURDIR} \
		-DREL=\"${DISTRIBVER}\" -DMACH=\"${MACHINE}\"

.if defined(SMALLPROG_INET6) && (${USE_INET6} != "no")
CPPFLAGS+=-DINET6
.endif


# Host to ftp from.  Default:
#	"ftp.NetBSD.org"
#
.if defined(SYSINST_FTP_HOST)
CPPFLAGS+=	-DSYSINST_FTP_HOST=\"${SYSINST_FTP_HOST}\"
.endif

# Top-level ftp directory.  Default:
#	"pub/NetBSD/NetBSD-" + VER + "/" + MACH
#
.if defined(SYSINST_FTP_DIR)
CPPFLAGS+=	-DSYSINST_FTP_DIR=\"${SYSINST_FTP_DIR}\"
.endif

# Top-level CDROM directory.  Default:
#	"/" + MACH
#
.if defined(SYSINST_CDROM_DIR)
CPPFLAGS+=	-DSYSINST_CDROM_DIR=\"${SYSINST_CDROM_DIR}\"
.endif


DPSRCS+=	msg_defs.h menu_defs.h

CLEANFILES=	menu_defs.c menu_defs.h menus.def \
		msg_defs.c msg_defs.h msg.def msgtouch \
		sysinstmsgs.*

.PATH: ${.CURDIR}/../..

menu_defs.h: menu_defs.c
	[ -f ${.TARGET} ] || { \
		${_MKSHTARGET_CREATE}; \
		${TOOL_MENUC} menus.def; \
	}
menu_defs.c: menus.def
	${_MKTARGET_CREATE}
	[ ! -f menu_defs.h ] || mv -f menu_defs.h menu_defs.oh
	${TOOL_MENUC} menus.def
	! cmp -s menu_defs.oh menu_defs.h || mv -f menu_defs.oh menu_defs.h
	@rm -f menu_defs.oh

msg_defs.h: msg_defs.c
	[ -f ${.TARGET} ] || { \
		${_MKSHTARGET_CREATE}; \
		${TOOL_MSGC} msg.def; \
	}
msg_defs.c: msg.def
	${_MKTARGET_CREATE}
	[ ! -f msg_defs.h ] || mv -f msg_defs.h msg_defs.oh
	${TOOL_MSGC} msg.def
	! cmp -s msg_defs.oh msg_defs.h || mv -f msg_defs.oh msg_defs.h
	@rm -f msg_defs.oh

# Needed to get proper dependency checks on osrelease
msgtouch: ${DISTRIBVERDEP}
	touch ${.TARGET}

msg.def: msg.mi.${SYSINSTLANG} ${MSG_MD} msgtouch
	${_MKTARGET_CREATE}
	sed "s/@@VERSION@@/${DISTRIBVER}/" ${.ALLSRC} | \
	awk -f ${UNIF_AWK} -v defines="${MD_OPTIONS}" > ${.TARGET}

menus.def: menus.mi ${MENUS_MD} msgtouch
	${_MKTARGET_CREATE}
	sed "s/@@VERSION@@/${DISTRIBVER}/" ${.ALLSRC} | \
	awk -f ${UNIF_AWK} -v defines="${MD_OPTIONS}" > ${.TARGET}

.for LANG in ${LANGUAGES:N${SYSINSTLANG}}
sysinstmsgs.${LANG}: msg.mi.${LANG} ${MSG_MD:S/.${SYSINSTLANG}$/.${LANG}/} msgtouch msg_defs.h
	${_MKTARGET_CREATE}
	sed "s/@@VERSION@@/${DISTRIBVER}/" ${.ALLSRC:M*.${LANG}} | \
	awk -f ${UNIF_AWK} -v defines="${MD_OPTIONS}" | \
	${HOST_SH} ${MSG_XLAT_SH} > ${.TARGET}

DPSRCS+=	sysinstmsgs.${LANG}
.endfor

.include <bsd.prog.mk>
