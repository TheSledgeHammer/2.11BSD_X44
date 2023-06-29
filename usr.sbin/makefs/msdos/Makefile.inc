#	$FreeBSD$
#

MSDOS=	${NETBSDSRCDIR}/sys/fs/msdosfs
MSDOS_NEWFS=	${NETBSDSRCDIR}/sbin/newfs_msdos

.PATH:	${.CURDIR}/msdos ${MSDOS} ${MSDOS_NEWFS} ${NETBSDSRCDIR}/sys/dev

CPPFLAGS+= -DMSDOS_EI -I${MSDOS} -I${MSDOS_NEWFS}
.if !defined(HOSTPROGNAME)
CPPFLAGS+= -I${NETBSDSRCDIR}/sys
.endif

SRCS+= mkfs_msdos.c
SRCS+= msdosfs_conv.c msdosfs_denode.c msdosfs_fat.c msdosfs_lookup.c
SRCS+= msdosfs_vnops.c msdosfs_vfsops.c
