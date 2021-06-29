#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.7.11 (2.11BSD) 1997/12/31
#

# machine-independent gen sources
.PATH: ${ARCHDIR}/gen ${.CURDIR}/gen

SRCS+=	alarm.c closedir.c crypt.c ctype_.c daemon.c devname.c disklabel.c 	\
		err.c execvp.c fabs.c fakcu.c frexp.c fstab.c fts.c gcvt.c 			\
		getgrent.c getgrgid.c getgrnam.c getlogin.c getgrouplist.c 			\
		gethostname.c getpagesize.c getpass.c getpwent.c 					\
		getloadavg.c getmntinfo.c getttyent.c getttynam.c getusershell.c 	\
		getwd.c glob.c initgroups.c insque.c isatty.c ndbm.c nlist.c 		\
		nlist_aout.c nlist_coff.c nlist_ecoff.c nlist_elf32.c nlist_elf64.c \
		opendir.c popen.c psignal.c pwcache.c readdir.c regex.c remque.c 	\
		scandir.c seekdir.c sethostname.c siginterrupt.c siglist.c signal.c \
		sigsetops.c sleep.c sysctl.c syserrlst.c syslog.c					\
		telldir.c time.c timezone.c ttyname.c ttyslot.c ualarm.c usleep.c 	\
		valloc.c vis.c uname.c unvis.c wait.c wait3.c waitpid.c

# machine-dependent gen sources
.include "${ARCHDIR}/gen/Makefile.inc"

MAN+=	alarm.3 clock.3 crypt.3 daemon.3 devname.3 err.3 fts.3 gethostname.3 getloadavg.3 	\
		getmntinfo.3 getpass.3 getpwent.3 getttyent.3 getusershell.3 glob.3 initgroups.3	\
		nlist.3 popen.3 psignal.3 pwcache.3 scandir.3 siginterrupt.3 signal.3 sigsetops.3 	\
		sleep.3	sysctl.3 syslog.3 time.3 timezone.3 ttyname.3 ualarm.3 uname.3 unvis.3 		\
		usleep.3 valloc.3 vis.3
