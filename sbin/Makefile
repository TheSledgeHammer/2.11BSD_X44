#	$NetBSD: Makefile,v 1.134 2018/09/30 15:56:46 jmcneill Exp $
#	@(#)Makefile	8.5 (Berkeley) 3/31/94

.include <bsd.own.mk>

SUBDIR=	ccdconfig 				\
		disklabel 				\
		fastboot fdisk fsck 	\
		init 					\
		ldconfig 				\
		mknod mount mountd		\
	    pfctl pflogd ping		\
        reboot rcorder 			\
		savecore shutdown 		\
		swapctl					\
		umount					\
		wsconsctl

# support for various file systems
SUBDIR+= newfs dumpfs
SUBDIR+= newlfs dumplfs
SUBDIR+= newfs_msdos fsck_msdos
SUBDIR+= mount_cd9660
SUBDIR+= mount_fdesc
SUBDIR+= mount_lfs
SUBDIR+= mount_lofs
SUBDIR+= mount_msdos
SUBDIR+= mount_ufml
SUBDIR+= mount_union

.if (${USE_INET6} != "no")
SUBDIR+= ping6
.endif

.include <bsd.subdir.mk>
