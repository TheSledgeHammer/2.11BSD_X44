#	$NetBSD: Makefile,v 1.134 2018/09/30 15:56:46 jmcneill Exp $
#	@(#)Makefile	8.5 (Berkeley) 3/31/94

.include <bsd.own.mk>

SUBDIR=	atactl                                 			\
		badsect											\
        ccdconfig 				                        \
		disklabel dmesg	dump	                        \
		fastboot fdisk fsck 	                        \
		gpt												\
		ifconfig init 			                        \
		ldconfig 				                        \
		mknod mount mountd		                        \
		nologin 				                        \
	    pfctl pflogd ping pppoectl                      \
	    quotacheck										\
        rcorder reboot route routed			            \
		savecore scsictl shutdown slattach swapctl		\
        ttyflags                                        \
		umount					                        \
		wsconsctl

# support for various file systems
SUBDIR+= newfs dumpfs clri restore tunefs
SUBDIR+= newfs_lfs dumpfs_lfs
SUBDIR+= newfs_msdos fsck_msdos
SUBDIR+= mount_cd9660
SUBDIR+= mount_fdesc
SUBDIR+= mount_ffs
SUBDIR+= mount_lfs
SUBDIR+= mount_lofs
SUBDIR+= mount_msdos
SUBDIR+= mount_ufml
SUBDIR+= mount_union

.if (${USE_INET6} != "no")
SUBDIR+= ping6
.endif

# IPsec
SUBDIR+= setkey

.include <bsd.subdir.mk>
