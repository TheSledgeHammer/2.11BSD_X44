#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.7.11 (2.11BSD) 1997/12/31
#

# machine-independent gen sources
.PATH: ${ARCHDIR}/gen ${.CURDIR}/gen

SRCS+=	abort.c alarm.c atoi.c atol.c calloc.c closedir.c crypt.c 			\
		ctime.c ctype_.c daemon.c devname.c disklabel.c err.c 				\
		execvp.c fakcu.c 													\
		fstab.c gcvt.c getenv.c getgrent.c getgrgid.c getgrnam.c getlogin.c \
		getgrouplist.c gethostname.c getpagesize.c 							\
		getpass.c getpwent.c getloadavg.c getmntinfo.c 						\
		getttyent.c getttynam.c getusershell.c getwd.c 						\
		initgroups.c isatty.c malloc.c mktemp.c ndbm.c nlist.c 				\
		nlist_aout.c nlist_coff.c nlist_ecoff.c nlist_elf32.c nlist_elf64.c \
		opendir.c perror.c popen.c psignal.c qsort.c random.c readdir.c 	\
		regex.c scandir.c seekdir.c 										\
		setmode.c sethostname.c 											\
		setenv.c siglist.c signal.c siginterrupt.c 							\
		sigsetops.c 														\
		sleep.c strcasecmp.c strftime.c swab.c sysctl.c syslog.c system.c 	\
		syserrlst.c 														\
		telldir.c time.c timezone.c ttyname.c ttyslot.c ualarm.c usleep.c 	\
		valloc.c strdup.c uname.c wait.c wait3.c waitpid.c

# machine-dependent gen sources
.include "${ARCHDIR}/gen/Makefile.inc"

MAN+=	abort.3 abs.3 alarm.3 atof.3 atoi.3 atol.3 bcmp.3 bcopy.3 bzero.3 calloc.3 							\
		clock.3 crypt.3 ctime.3 daemon.3 devname.3 err.3 ffs.3 fts.3 getenv.3 gethostname.3 				\
		getloadavg.3 getmntinfo.3 getpass.3 getpwent.3 getttyent.3 getusershell.3 initgroups.3 malloc.3 	\
		mktemp.3 nlist.3 popen.3 psignal.3 qsort.3 random.3 rindex.3 scandir.3 setmode.3 siginterrupt.3 	\
		signal.3 sigsetops.3 sleep.3 strcasecmp.3 strcat.3 strcmp.3 strcpy.3 strdup.3 strftime.3 strlen.3 	\
		swab.3 sysctl.3 syslog.3 system.3 time.3 timezone.3 ttyname.3 ualarm.3 uname.3 usleep.3 valloc.3
