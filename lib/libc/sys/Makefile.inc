#	@(#)Makefile.inc	8.3 (Berkeley) 10/24/94

# sys sources
.PATH: ${ARCHDIR}/sys ${.CURDIR}/sys

# TODO: Fix compiler warning below:  
# - relocation against SYS_ in read-only section '.text'
# For the following:
# - _resumecontext.c _swapcontext.c

# modules with non-default implementations on at least one architecture:
SRCS+=      	Ovfork.S brk.S exect.S fork.S pipe.S \
            	getcontext.S ptrace.S reboot.S \
            	sbrk.S setlogin.S sigpending.S \
            	sigprocmask.S sigreturn.S sigsuspend.S \
                syscall.S __syscall.S cerror.S

#acl_file.S acl_filedesc.S
# glue to provide compatibility between GCC 1.X and 2.X
GLUE+=          acl_file.c acl_filedesc.c execv.c \
                extattr_file.c extattr_filedesc.c \
                ftruncate.c lseek.c mmap.c mpx.c \
				pread.c preadv.c pwrite.c pwritev.c \
				truncate.c
GLUENOERR+=		_fork.c _getcontext.c _getlogin.c

# 'glue' files might .c or .S depending on the architecture
.for glue in ${GLUE}
. if exists(${glue:.c=.S})
# Build the ASM glue file
SRCS+=${glue:.c=.S}
. else
.  if exists(__${glue:.c=.S})
SRCS+=__${glue:.c=.S} ${glue}
.  else
# Build the C glue file
SRCS+= ${glue}
# and an asm entry for __<syscall>
ASM_GLUE+= tmp_${glue:.c=.S}
.  endif
. endif
.endfor

# 'glue' files might .c or .S depending on the architecture
.for glue in ${GLUENOERR}
. if exists(${glue:.c=.S})
# Build the ASM glue file
SRCS+=${glue:.c=.S}
. else
.  if exists(__${glue:.c=.S})
SRCS+=__${glue:.c=.S} ${glue}
.  else
# Build the C glue file
SRCS+= ${glue}
# and an asm entry for __<syscall>
ASM_GLUENOERR+= tmp_${glue:.c=.S}
.  endif
. endif
.endfor

# modules with default implementations on all architectures:
ASM=            access.S acct.S adjtime.S bind.S chdir.S chflags.S chmod.S \
                chown.S chroot.S clock_gettime.S clock_getres.S clock_settime.S \
		dup.S dup2.S fchdir.S fchflags.S fchmod.S fchown.S flock.S \
                fstat.S fstatfs.S getdirentries.S getdtablesize.S \
                getfsstat.S getgroups.S getitimer.S getpeername.S getpriority.S \
                getrlimit.S getrusage.S getsockname.S getsockopt.S gettimeofday.S \
                ioctl.S ktrace.S lfs_bmapv.S lfs_markv.S \
                lfs_segclean.S lfs_segwait.S link.S kenv.S kevent.S \
                kqueue.S listen.S lstat.S madvise.S mincore.S minherit.S mkdir.S \
		mkfifo.S mknod.S mlock.S mount.S mprotect.S msync.S munlock.S \
                munmap.S nanosleep.S nostk.S pathconf.S pipe2.S quotactl.S \
                rename.S revoke.S rmdir.S setegid.S seteuid.S setgid.S \
                setgroups.S setitimer.S setpgid.S setpriority.S \
                setrlimit.S setsid.S setsockopt.S settimeofday.S setuid.S \
                sigaltstack.S sigstack.S sigtimedwait.S shutdown.S \
                socket.S socketpair.S sstk.S stat.S statfs.S swapctl.S \
                symlink.S undelete.S unlink.S unmount.S utimes.S utrace.S \
		uuidgen.S vtrace.S __sysctl.S

WEAKASM=        accept.S close.S connect.S execve.S fcntl.S fsync.S kill.S \
                open.S poll.S pselect.S read.S readlink.S readv.S recvfrom.S \
                recvmsg.S select.S sendmsg.S sendto.S setcontext.S \
                sysarch.S wait4.S write.S writev.S

NOERR=          getegid.S geteuid.S getgid.S getpgid.S getpid.S getpgrp.S \
		getppid.S getsid.S getuid.S issetugid.S profil.S sigaction.S \
		sync.S umask.S

PSEUDONOERR=    _exit.S

_GENFILES=	${ASM} ${WEAKASM} ${NOERR} ${PSEUDONOERR} \
	        ${ASM_GLUE}

SRCS+=		${_GENFILES}
CLEANFILES+=    ${_GENFILES}

ASMDEPS=        ${.CURDIR}/sys/Makefile.inc ${ARCHDIR}/SYS.h \
                ${DESTDIR}/usr/include/sys/syscall.h

${ASM}: ${ASMDEPS}
	${_MKTARGET_CREATE}
	printf '#include "SYS.h"\nRSYSCALL(${.PREFIX:S/tmp_//})\n' >${.TARGET}

${ASM_GLUE}: ${ASMDEPS}
	${_MKTARGET_CREATE}
	printf '#include "SYS.h"\nPSEUDO(${.PREFIX:S/tmp_/__/},${.PREFIX:S/tmp_//})\n' >${.TARGET}


${ASM_GLUENOERR}: ${ASMDEPS}
	${_MKTARGET_CREATE}
	printf '#include "SYS.h"\nPSEUDO_NOERROR(${.PREFIX:S/tmp_/__/},${.PREFIX:S/tmp_//})\n' >${.TARGET}

${WEAKASM}: ${ASMDEPS}
	${_MKTARGET_CREATE}
	printf '#include "SYS.h"\nWSYSCALL(${.PREFIX},${.PREFIX})\nWEAK_ALIAS(_${.PREFIX},${.PREFIX})\n' >${.TARGET}

${NOERR}: ${ASMDEPS}
	${_MKTARGET_CREATE}
	printf '#include "SYS.h"\nRSYSCALL_NOERROR(${.PREFIX})\n' >${.TARGET}

${PSEUDONOERR}: ${ASMDEPS}
	${_MKTARGET_CREATE}
	printf '#include "SYS.h"\nPSEUDO_NOERROR(${.PREFIX},${.PREFIX:S/_//})\n' >${.TARGET}
	# add an _Exit alias to _exit
	printf 'STRONG_ALIAS(${.PREFIX:S/e/E/},${.PREFIX})\n' >>${.TARGET}

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
