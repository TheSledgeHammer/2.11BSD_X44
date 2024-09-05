#	$NetBSD: Makefile,v 1.63 2003/01/03 15:34:33 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

CRUNCHBIN=	instbin
ARCHDIR=	${.CURDIR}/../${MACHINE}/miniroot

VND?=		vnd0c
VND_DEV=	/dev/${VND}
VND_RDEV=	/dev/r${VND}
VND_GEOM?=
IMAGE=		miniroot.fs
IMAGESIZE?=	12288 # 6MB in 512 byte blocks
NEWFSOPTS?=
MOUNT_POINT?= /mnt
PRELABEL?=
BOOTINSTALL?= true

.ifdef DISKTYPE
DISKTYPEARG=	-t ${DISKTYPE}
.endif

LISTS=		${.CURDIR}/list ${ARCHDIR}/list
MTREECONF=	${DISTRIBDIR}/common/mtree.common

PARSELISTENV=	ARCHDIR=${ARCHDIR:Q} \
				DISTRIBREV=${DISTRIBREV:Q} \
				DISTRIBVER=${DISTRIBVER:Q} \
				KERNOBJDIR=${KERNOBJDIR:Q} \
		
IMAGE_RELEASEDIR=	installation/miniroot

.include 	"${ARCHDIR}/Makefile.inc"

all: unconfig ${CRUNCHBIN}
	dd if=/dev/zero of=${IMAGE} count=${IMAGESIZE}
	vnconfig ${DISKTYPEARG} -v -c ${VND_DEV} ${IMAGE} ${VND_GEOM}
	${PRELABEL}
	-newfs -m 0 -o space ${NEWFSOPTS} ${VND_RDEV} ${DISKTYPE}
	${BOOTINSTALL}
	mount ${VND_DEV} ${MOUNT_POINT}
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	umount ${MOUNT_POINT}
	vnconfig -u ${VND_DEV}

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND_DEV}
	-/bin/rm -f ${IMAGE}

.include "${DISTRIBDIR}/common/Makefile.crunch"
.if defined(MAKEDEVTARGETS)
.include "${DISTRIBDIR}/common/Makefile.makedev"
.endif

release: $(IMAGE)
	gzip -c -9 < $(.OBJDIR)/${IMAGE} \
		> $(RELEASEDIR)/${IMAGE_RELEASEDIR}/${IMAGE}.gz
		
.include <bsd.prog.mk>
