#	$NetBSD: Makefile.inc,v 1.7.2.2 1997/11/29 21:29:07 perry Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

IMAGE=		    boot.fs
DISKSIZE?=	    2880
DISKTYPE?=	    floppy3
KERN=		    twobsd-INSTALL

MOUNT_POINT?=	/mnt
VND?=		    vnd0
VND_DEV=	    /dev/${VND}a
VND_RDEV=	    /dev/r${VND}a
VND_CDEV=	    /dev/${VND}d
VND_CRDEV=	    /dev/r${VND}d
MDEC=		    ${DESTDIR}/usr/mdec

LISTS=		    list

# Some reasonable values for the -i parameter to newfs are:
#
#   6144	1147k, 189 inodes free
#  16384	1159k,  93 inodes free
# 204800	1167k,  29 inodes free
INO_BYTES?=	204800

CLEANFILES+=	twobsd.ram.gz

all:	twobsd.ram.gz
	dd if=/dev/zero of=${IMAGE} count=${DISKSIZE}
	vnconfig -t ${DISKTYPE} -v -c ${VND_CDEV} ${IMAGE}
	disklabel -rw ${VND_CDEV} ${DISKTYPE}
	newfs -m 0 -o space -i ${INO_BYTES} -c 80 ${VND_RDEV} ${DISKTYPE}
	mount ${VND_DEV} ${MOUNT_POINT}
	boot0cfg -v ${VND_DEV} -s 1
	@echo ""
	@df -i ${MOUNT_POINT}
	@echo ""
	@echo "installing new bootblocks"
	-rm -f ${MOUNT_POINT}/boot
	umount ${MOUNT_POINT}
	vnconfig -u ${VND_CDEV}
	cat /*bin/* > /dev/null

unconfig:
	-umount -f ${MOUNT_POINT}
	-vnconfig -u ${VND_DEV}
	-/bin/rm -f ${IMAGE}

twobsd.ram.gz: ${KERN} ${RAMDISK}
	cp ${KERN} twobsd.tmp
	${DESTDIR}/usr/sbin/mdsetimage -v twobsd.tmp ${RAMDISK}
	${STRIP} twobsd.tmp
	gzip -9 twobsd.tmp
	mv twobsd.tmp.gz ${.TARGET}

release:
	-mkdir -p ${RELEASEDIR}/installation/floppy
	cp -p ${IMAGE} ${RELEASEDIR}/installation/floppy
	gzip -c -9 <${IMAGE} >${RELEASEDIR}/installation/floppy/${IMAGE}.gz

clean cleandir:
	/bin/rm -f *.core ${IMAGE} ${CLEANFILES}

.include <bsd.prog.mk>
