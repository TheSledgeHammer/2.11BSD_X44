#	@(#)Makefile.inc	8.3 (Berkeley) 10/24/94

# sys sources
.PATH: ${ARCHDIR}/sys ${.CURDIR}/sys

# modules with non-default implementations on at least one architecture:
SRCS+=	Ovfork.S brk.S cerror.S exect.S fork.S getcontext.S pipe.S ptrace.S \
        reboot.S sbrk.S setlogin.S sigpending.S sigprocmask.S sigreturn.S \
		sigsuspend.S syscall.S

# glue to provide compatibility between GCC 1.X and 2.X
SRCS+= _getcontext.c _getlogin.c _fork.c ftruncate.c lseek.c mmap.c truncate.c

# modules with default implementations on all architectures:
ASM=	accept.S access.S acct.S adjtime.S bind.S chdir.S chflags.S chmod.S \
	chown.S chroot.S close.S connect.S dup.S dup2.S execve.S fchdir.S \
	fchflags.S fchmod.S fchown.S fcntl.S flock.S fpathconf.S fstat.S \
	fstatfs.S fsync.S getdirentries.S getdtablesize.S getegid.S \
	geteuid.S getfh.S getfsstat.S getgid.S getgroups.S getitimer.S \
	getpeername.S getpgrp.S getpid.S getppid.S getpriority.S \
	getrlimit.S getrusage.S getsockname.S getsockopt.S gettimeofday.S \
	getuid.S ioctl.S kill.S ktrace.S lfs_bmapv.S lfs_markv.S \
	lfs_segclean.S lfs_segwait.S link.S listen.S lstat.S \
	madvise.S mincore.S mkdir.S mkfifo.S mknod.S mlock.S mount.S \
	mprotect.S msync.S munlock.S munmap.S nfssvc.S open.S \
	pathconf.S profil.S quotactl.S read.S \
	readlink.S readv.S recvfrom.S recvmsg.S rename.S revoke.S rmdir.S \
	select.S sendmsg.S sendto.S setegid.S seteuid.S setgid.S \
	setgroups.S setitimer.S setpgid.S setpriority.S setrlimit.S \
	setsid.S setsockopt.S settimeofday.S setuid.S shutdown.S \
	sigaction.S sigaltstack.S socket.S socketpair.S stat.S statfs.S \
	swapon.S symlink.S sync.S umask.S undelete.S unlink.S unmount.S \
	utimes.S vadvise.S wait4.S write.S writev.S __syscall.S __sysctl.S

PSEUDO=	_exit.S _getlogin.S

ASMDEPS=    ${.CURDIR}/sys/Makefile.inc ${ARCHDIR}/SYS.h ${ARCHDIR}/DEFS.h \
            ${DESTDIR}/usr/include/sys/syscall.h

PASM=	    ${ASM:.S=.po}
SASM=	    ${ASM:.S=.so}
PPSEUDO=    ${PSEUDO:.S=.po}
SPSEUDO=    ${PSEUDO:.S=.so}

OBJS+=	${ASM} ${PSEUDO}
POBJS+=	${PASM} ${SASM} ${PPSEUDO} ${SPSEUDO}
CLEANFILES+=${ASM} ${PSEUDO} ${POBJS}

${PASM}: ${ASMDEPS}
	${_MKTARGET_CREATE}
	printf '#include "SYS.h"\nRSYSCALL(${.PREFIX})\n' >${.TARGET}

${SASM}: ${ASMDEPS}
	${_MKTARGET_CREATE}
	printf '#include "SYS.h"\nRSYSCALL(${.PREFIX})\n' >${.TARGET}

${PPSEUDO}: ${ASMDEPS}
	${_MKTARGET_CREATE}
	printf '#include "SYS.h"\nPSEUDO(${.PREFIX},${.PREFIX:S/_//})\n' >${.TARGET}

${SPSEUDO}: ${ASMDEPS}
	${_MKTARGET_CREATE}
	printf '#include "SYS.h"\nPSEUDO(${.PREFIX},${.PREFIX:S/_//})\n' >${.TARGET}

MAN+=	accept.2 access.2 acct.2 adjtime.2 bind.2 brk.2 chdir.2 chflags.2 \
		chmod.2 chown.2 chroot.2 close.2 connect.2 dup.2 execve.2 _exit.2 \
		fcntl.2 flock.2 fork.2 fsync.2 getdirentries.2 getdtablesize.2 \
		getfh.2 getfsstat.2 getgid.2 getgroups.2 getitimer.2 getlogin.2 \
		getpeername.2 getpgrp.2 getpid.2 getpriority.2 getrlimit.2 \
		getrusage.2 getsockname.2 getsockopt.2 gettimeofday.2 getuid.2 \
		intro.2 ioctl.2 kill.2 ktrace.2 link.2 listen.2 lseek.2 \
		mkdir.2 mkfifo.2 mknod.2 madvise.2 mincore.2 mlock.2 mmap.2 \
		mount.2 mprotect.2 msync.2 munmap.2 \
		nfssvc.2 open.2 pathconf.2 pipe.2 profil.2 ptrace.2 quotactl.2 \
		munmap.2 nfssvc.2 open.2 pipe.2 profil.2 ptrace.2 quotactl.2 \
		read.2 readlink.2 reboot.2 recv.2 rename.2 revoke.2 rmdir.2 \
		select.2 send.2 setgroups.2 setpgid.2 setsid.2 setuid.2 shutdown.2 \
		sigaction.2 sigpending.2 sigprocmask.2 sigreturn.2 sigaltstack.2 \
		sigstack.2 sigsuspend.2 socket.2 socketpair.2 stat.2 statfs.2 \
		swapon.2 symlink.2 sync.2 syscall.2 truncate.2 umask.2 unlink.2 \
		undelete.2 utimes.2 vfork.2 wait.2 write.2

MLINKS+=brk.2 sbrk.2
MLINKS+=dup.2 dup2.2
MLINKS+=chdir.2 fchdir.2
MLINKS+=chflags.2 fchflags.2
MLINKS+=chmod.2 fchmod.2
MLINKS+=chown.2 fchown.2
MLINKS+=getgid.2 getegid.2
MLINKS+=getitimer.2 setitimer.2
MLINKS+=getlogin.2 setlogin.2
MLINKS+=getpid.2 getppid.2
MLINKS+=getpriority.2 setpriority.2
MLINKS+=getrlimit.2 setrlimit.2
MLINKS+=getsockopt.2 setsockopt.2
MLINKS+=gettimeofday.2 settimeofday.2
MLINKS+=getuid.2 geteuid.2
MLINKS+=intro.2 errno.2
MLINKS+=lseek.2 seek.2
MLINKS+=mlock.2 munlock.2
MLINKS+=mount.2 unmount.2
MLINKS+=pathconf.2 fpathconf.2
MLINKS+=read.2 readv.2
MLINKS+=recv.2 recvfrom.2 recv.2 recvmsg.2
MLINKS+=send.2 sendmsg.2 send.2 sendto.2
MLINKS+=setuid.2 setegid.2 setuid.2 seteuid.2 setuid.2 setgid.2
MLINKS+=stat.2 fstat.2 stat.2 lstat.2
MLINKS+=statfs.2 fstatfs.2
MLINKS+=syscall.2 __syscall.2
MLINKS+=truncate.2 ftruncate.2
MLINKS+=wait.2 wait3.2 wait.2 wait4.2 wait.2 waitpid.2
MLINKS+=write.2 writev.2
