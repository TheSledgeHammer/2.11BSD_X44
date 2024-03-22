#	$NetBSD: Makefile.ramdisk,v 1.4 2008/02/01 13:43:12 tsutsui Exp $

NOSANITIZER=	# defined

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

WARNS=		1
DBG=		-Os

CRUNCHBIN=		ramdiskbin

IMAGE=			ramdisk.fs
IMAGESIZE=		2880
DISKTYPE= 		floppy3

MOUNT_POINT?=	/mnt
VND?=			vnd0
VND_DEV=		/dev/${VND}a
VND_RDEV=		/dev/r${VND}a
VND_CDEV=		/dev/${VND}d
VND_CRDEV=		/dev/r${VND}d
MDEC=			${DESTDIR}/usr/mdec

LISTS=			list.ramdisk
MTREECONF=		${DISTRIBDIR}/common/mtree.common
CRUNCHCONF=		${CRUNCHBIN}.conf
IMAGEENDIAN=	le
IMAGEDEPENDS+= 	${CRUNCHBIN} \
				dot.profile \
        		${DESTDIR}/usr/mdec/boot0 \
				${DESTDIR}/usr/mdec/boot2 \
        		${DESTDIR}/usr/mdec/cdboot \
        		${DESTDIR}/usr/mdec/gptboot \
				${DESTDIR}/usr/mdec/loader \
				${DESTDIR}/usr/mdec/mbr \
				${DESTDIR}/usr/mdec/pmbr \
				${NETBSDSRCDIR}/etc/group \
				${NETBSDSRCDIR}/etc/master.passwd \
				${NETBSDSRCDIR}/etc/netconfig \
				${NETBSDSRCDIR}/etc/protocols \
				${NETBSDSRCDIR}/distrib/common/services \
				${NETBSDSRCDIR}/distrib/common/termcap.mini
				
all: ${IMAGEDEPENDS}
	dd if=/dev/zero of=${IMAGE} count=${IMAGESIZE}
	vnconfig -t ${DISKTYPE} -v -c ${VND_CDEV} ${IMAGE}
	disklabel -rw ${VND_CDEV} ${DISKTYPE}
	newfs -B ${IMAGEENDIAN} -m 0 -o space -i 5120 -c 80 ${VND_RDEV} ${DISKTYPE}
	mount ${VND_DEV} ${MOUNT_POINT}
	boot0cfg -v ${VND_DEV} -s 1 
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	umount ${MOUNT_POINT}
	vnconfig -u ${VND_CDEV}

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND_DEV}
	-/bin/rm -f ${IMAGE}

# This propagates through to the link of ramdiskbin
# The map file is useful when trying to prune the image
LDFLAGS+=	-Wl,-Map,${CRUNCHBIN}.map,--cref
CRUNCHENV+=	LDFLAGS=${LDFLAGS:Q}
CLEANFILES+=	${CRUNCHBIN}.map

.include "${DISTRIBDIR}/common/Makefile.crunch"

release: ${IMAGE}
	${HOST_INSTALL_FILE} -m ${NONBINMODE} ${IMAGE} \
		${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/ramdisk/

.include <bsd.prog.mk>