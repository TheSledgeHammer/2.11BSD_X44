# Temporary File For Setting up the kernel to work with the bootloader
# Code here comes from FreeBSD kern.pre.mk & kern.post.mk

# Can be overridden by makeoptions or /etc/make.conf
KERNEL_KO?=		twobsd
KERNEL?=		twobsd
KODIR?=			/boot/${KERNEL}
MACHINE_NAME!=  uname -n

install: install-kernel-${MACHINE_NAME}
.if !target(install-kernel-${MACHINE_NAME})
install-kernel-${MACHINE_NAME}:
	@if [ ! -f ${KERNEL_KO} ] ; then \
		echo "You must build a kernel first." ; \
		exit 1 ; \
	fi
.if exists(${DESTDIR}${KODIR})
	-thiskernel=`sysctl -n kern.bootfile || echo /boot/kernel/kernel` ; \
	if [ ! "`dirname "$$thiskernel"`" -ef ${DESTDIR}${KODIR} ] ; then \
		chflags -R noschg ${DESTDIR}${KODIR} ; \
		rm -rf ${DESTDIR}${KODIR} ; \
		rm -rf ${DESTDIR}${KERN_DEBUGDIR}${KODIR} ; \
	else \
		if [ -d ${DESTDIR}${KODIR}.old ] ; then \
			chflags -R noschg ${DESTDIR}${KODIR}.old ; \
			rm -rf ${DESTDIR}${KODIR}.old ; \
		fi ; \
		mv ${DESTDIR}${KODIR} ${DESTDIR}${KODIR}.old ; \
		if [ -n "${KERN_DEBUGDIR}" -a \
		     -d ${DESTDIR}${KERN_DEBUGDIR}${KODIR} ]; then \
			rm -rf ${DESTDIR}${KERN_DEBUGDIR}${KODIR}.old ; \
			mv ${DESTDIR}${KERN_DEBUGDIR}${KODIR} ${DESTDIR}${KERN_DEBUGDIR}${KODIR}.old ; \
		fi ; \
		sysctl kern.bootfile=${DESTDIR}${KODIR}.old/"`basename "$$thiskernel"`" ; \
	fi
.endif
.endif
	mkdir -p ${DESTDIR}${KODIR}
	${INSTALL} -p -m 555 -o ${KMODOWN} -g ${KMODGRP} ${KERNEL_KO} ${DESTDIR}${KODIR}/
#.if defined(DEBUG) && !defined(INSTALL_NODEBUG) && ${MK_KERNEL_SYMBOLS} != "no"
#	mkdir -p ${DESTDIR}${KERN_DEBUGDIR}${KODIR}
#	${INSTALL} -p -m 555 -o ${KMODOWN} -g ${KMODGRP} ${KERNEL_KO}.debug ${DESTDIR}${KERN_DEBUGDIR}${KODIR}/
#.endif
