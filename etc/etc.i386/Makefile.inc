#	$NetBSD: Makefile.inc,v 1.69 2018/07/26 16:22:49 maxv Exp $
#
#	etc.i386/Makefile.inc -- i386-specific etc Makefile targets
#

# If you change the list of distributed kernels, don't forget
# to update the release documentation in distrib/notes/common/contents

KERNEL_SETS=		GENERIC.i386
KERNEL_SETS+=		GENERIC_PAE.i386
KERNEL_SETS+=		GENERIC_SMP.i386

BUILD_KERNELS+=		GENERIC.i386
BUILD_KERNELS+=		GENERIC_PAE.i386
BUILD_KERNELS+=		GENERIC_SMP.i386

INSTALLATION_DIRS+=	installation/misc
INSTALLATION_DIRS+=	installation/cdrom
INSTALLATION_DIRS+=	installation/floppy

# i386 specific distrib stuff
#snap_md_post:
#	cd stand/boot/arch/i386
