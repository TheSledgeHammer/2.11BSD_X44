#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.7.11 (2.11BSD) 1997/12/31
#

# machine-independent gen sources
.PATH: ${ARCHDIR}/gen ${.CURDIR}/gen

SRCS+=	abort.c alarm.c atoi.c atol.c calloc.c closedir.c crypt.c \
		ctime.c ctype_.c daemon.c devname.c disklabel.c err.c \
		execvp.c fakcu.c \
		fstab.c gcvt.c getenv.c getgrent.c getgrgid.c getgrnam.c getlogin.c \
		getgrouplist.c gethostname.c getpagesize.c \
		getpass.c getpwent.c getloadavg.c getmntinfo.c \
		getttyent.c getttynam.c getusershell.c getwd.c \
		initgroups.c isatty.c malloc.c mktemp.c ndbm.c nlist.c \
		opendir.c perror.c popen.c psignal.c qsort.c random.c readdir.c \
		regex.c scandir.c seekdir.c \
		setmode.c sethostname.c \
		setenv.c siglist.c signal.c siginterrupt.c \
		sigsetops.c \
		sleep.c strcasecmp.c strftime.c swab.c sysctl.c syslog.c system.c \
		syserrlst.c \
		telldir.c time.c timezone.c ttyname.c ttyslot.c ualarm.c usleep.c \
		valloc.c strdup.c uname.c wait.c wait3.c waitpid.c

# machine-dependent gen sources
.include "${ARCHDIR}/gen/Makefile.inc"
