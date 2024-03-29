#	@(#)Makefile	8.2 (Berkeley) 3/21/94

# Makefile for kernel tags files, init_sysent, etc.

ARCH=	i386

all:
		@echo "make tags, links, init_sysent.c"
	

SYSCALLSRC = makesyscalls.sh syscalls.conf syscalls.master
			init_sysent.c syscalls.c ../sys/syscall.h: ${SYSCALLSRC}
			-mv -f init_sysent.c init_sysent.c.bak
			-mv -f syscalls.c syscalls.c.bak
			-mv -f ../sys/syscall.h ../sys/syscall.h.bak	
			sh makesyscalls.sh syscalls.conf syscalls.master


# Kernel tags:
# Tags files are built in the top-level directory for each architecture,
# with a makefile listing the architecture-dependent files, etc.  The list
# of common files is in ./Make.tags.inc.  Links to the correct tags file
# are placed in each source directory.  We need to have links to tags files
# from the generic directories that are relative to the machine type, even
# via remote mounts; therefore we use symlinks to $SYSTAGS, which points at
# ${SYSDIR}/${MACHINE}/tags.

SYSTAGS=/var/db/sys_tags
SYSDIR=/sys

.ifmake links
# Directories in which to place tags links (other than machine-dependent)
DGEN=	(cd ${SYSDIR};							\
	    find -H . -name arch -prune -o -name "*.[chs]" -print |	\
	    sed -e 's@^\./@@' -e 's@/[^/]*\..*@@' | sort -t / -u )
.endif

tags::
	-for i in ${ARCH}; do \
	    (cd ../$$i && make ${MFLAGS} tags); done

links::
	rm -f ${SYSTAGS}
	ln -s ${SYSDIR}/${MACHINE}/tags ${SYSTAGS}
	-for i in ${DGEN}; do \
	    (cd ../$$i && { rm -f tags; ln -s ${SYSTAGS} tags; }) done
	-for i in ${ARCH}; do \
	    (cd ../$$i && make ${MFLAGS} SYSTAGS=${SYSTAGS} links); done