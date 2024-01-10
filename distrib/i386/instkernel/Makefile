#	$NetBSD: Makefile,v 1.11 2008/05/02 23:13:06 ad Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

#	create ${RAMDISK_*} variables
#
RAMDISKS=	RAMDISK_B	ramdisk-big
# RAMDISKS+=	RAMDISK_S	ramdisk-small
# RAMDISKS+=	RAMDISK_T	ramdisk-tiny
# RAMDISKS+=	RESCUE_T	ramdisk-rescuetiny

.for V F in ${RAMDISKS}
${V}DIR!=	cd ${.CURDIR}/../ramdisks/${F} && ${PRINTOBJDIR}
${V}=		${${V}DIR}/${F}.fs
.endfor

# If you change the list of distributed install kernels, don't forget
# to update the release documentation in distrib/notes/common/contents
MDSETTARGETS+=	GENERIC			    ${RAMDISK_B}	-
MDSETTARGETS+=	INSTALL			    ${RAMDISK_B}	-
# MDSETTARGETS+=	INSTALL_FLOPPY		${RAMDISK_B}	-
# MDSETTARGETS+=	INSTALL_SMALL		${RAMDISK_S}	-
# MDSETTARGETS+=	INSTALL_TINY		${RAMDISK_T}	-
# MDSETTARGETS+=	INSTALL_PS2		    ${RAMDISK_S}	-
# MDSETTARGETS+=	INSTALL_TINY		${RESCUE_T}	twobsd-RESCUE_TINY

MDSET_RELEASEDIR=	binary/kernel
# need symbols to load modules. don't actually want image inserted.
MDSET_NOSTRIP.twobsd-GENERIC=
MDSET_NOIMAGE.twobsd-GENERIC=

# do not strip any other kernels, it is done by Makefile.tarfloppy before
# they are written to a floppy.
MDSET_NOSTRIP=

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
