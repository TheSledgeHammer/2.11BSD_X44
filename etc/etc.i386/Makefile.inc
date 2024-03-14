#	$NetBSD: Makefile.inc,v 1.69 2018/07/26 16:22:49 maxv Exp $
#
#	etc.i386/Makefile.inc -- i386-specific etc Makefile targets
#

KERNEL_SETS=		GENERIC.i386
KERNEL_SETS+=		GENERIC_PAE.i386
KERNEL_SETS+=		GENERIC_SMP.i386

BUILD_KERNELS+=		INSTALL
BUILD_KERNELS+=		INSTALL_PAE
BUILD_KERNELS+=		INSTALL_SMP

INSTALLATION_DIRS+=	installation/misc
INSTALLATION_DIRS+=	installation/cdrom
INSTALLATION_DIRS+=	installation/floppy
INSTALLATION_DIRS+=	installation/miniroot

# i386 specific distrib stuff
snap_md_post:
	  cd ${NETBSDSRCDIR}/distrib && ${MAKE} depend && ${MAKE}
	  cd ${NETBSDSRCDIR}/distrib/i386 && ${MAKE} release
	  cd ${NETBSDSRCDIR}/distrib/notes && ${MAKE} release
	  ${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/cdrom '*.iso'
	  ${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/floppy '*.fs'
	  ${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/miniroot '*.*'
	  ${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/misc '*.*'
