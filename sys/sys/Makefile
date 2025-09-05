#	$211BSD: Makefile,v 1.0 2021/05/25 23:59:27 Exp $

.include <bsd.own.mk>

INCSDIR= 	/usr/include/sys

INCS=		acct.h acl.h agpio.h ansi.h ataio.h atomic.h audioio.h bitlist.h boot.h 		\
			bswap.h buf.h bufq.h callout.h cdefs_aout.h cdefs_elf.h cdefs_linker.h cdefs.h 	\
			cdio.h chio.h clist.h conf.h cputopo.h device.h devsw.h dir.h dirent.h 		\
			disk.h diskapm.h diskgpt.h disklabel.h diskmbr.h diskslice.h dk.h dkbad.h 	\
			dmap.h domain.h dvdio.h endian.h errno.h event.h eventvar.h exec_aout.h 	\
			exec_coff.h exec_ecoff.h exec_elf.h exec_linker.h exec_macho.h 			\
			exec_pecoff.h exec_script.h exec_xcoff.h exec.h	extattr.h extent.h fcntl.h 	\
			fdio.h file.h filedesc.h fnv_hash.h fperr.h gmon.h hint.h inttypes.h 		\
			ioccom.h ioctl_compat.h ioctl.h kenv.h kernel.h ksyms.h kthread.h ktrace.h 	\
			lock.h lockf.h malloc.h malloctypes.h map.h mbuf.h memrange.h midiio.h 		\
			mman.h mount.h mpx.h msgbuf.h mtio.h mutex.h namei.h null.h param.h percpu.h 	\
			poll.h power.h proc.h protosw.h ptrace.h pty.h queue.h reboot.h resource.h 	\
			resourcevar.h rwlock.h scanio.h sched_cfs.h sched_edf.h	sched.h scsiio.h 	\
			select.h signal.h signalvar.h socket.h socketvar.h stat.h stdarg.h 		\
			stdbool.h stddef.h stdint.h syscall.h sysctl.h sysdecl.h syslimits.h		\
			syslog.h systm.h tablet.h termios.h thread.h time.h timeb.h timepps.h 		\
			times.h timetc.h tls.h tprintf.h trace.h tree.h tty.h ttychars.h ttydefaults.h 	\
			ttydev.h types.h ucontext.h ucred.h uio.h un.h unistd.h unpcb.h user.h 		\
			utsname.h uuid.h vcmd.h videoio.h vlimit.h vmmeter.h vmsystm.h vnode_if.h 	\
			vnode.h wait.h		

INCSYMLINKS=						\
		sys/exec_elf.h 	/usr/include/elf.h 	\
		sys/fcntl.h 	/usr/include/fcntl.h 	\
		sys/poll.h 	/usr/include/poll.h 	\
		sys/select.h 	/usr/include/select.h 	\
		sys/stdarg.h 	/usr/include/stdarg.h 	\
		sys/stdbool.h 	/usr/include/stdbool.h 	\
		sys/stddef.h 	/usr/include/stddef.h 	\
		sys/stdint.h 	/usr/include/stdint.h 	\
		sys/syslog.h 	/usr/include/syslog.h 	\
		sys/termios.h 	/usr/include/termios.h 	\

.include <bsd.kinc.mk>
