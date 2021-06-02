#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.7.11 (2.11BSD) 1997/12/31
#

# machine-independent gen sources
.PATH: ${ARCHDIR}/gen ${.CURDIR}/gen

SRCS+=	abort.c alarm.c atoi.c atol.c											\
		basename.c bcmp.c bcopy.c bswap16.c bswap32.c bswap64.c bzero.c 		\
		calloc.c closedir.c crypt.c ctime.c ctype_.c 							\
		daemon.c devname.c dirname.c disklabel.c 								\
		err.c errc.c errx.c execvp.c 											\
		fabs.c fakcu.c ffs.c frexp.c fstab.c fts.c 								\
		gcvt.c getcap.c getenv.c getgrent.c getgrgid.c getgrnam.c getlogin.c 	\
		getgrouplist.c gethostname.c getpagesize.c 								\
		getpass.c getpwent.c getloadavg.c getmntinfo.c 							\
		getttyent.c getttynam.c getusershell.c getwd.c glob.c 					\
		initgroups.c isatty.c malloc.c mktemp.c ndbm.c nlist.c 					\
		opendir.c perror.c popen.c psignal.c pw_scan.c pwcache.c 				\
		qsort.c random.c readdir.c regex.c remque.c rindex.c					\
		scandir.c seekdir.c setenv.c sethostname.c setmode.c					\
		setprogname.c siglist.c signal.c siginterrupt.c 						\
		sigsetops.c sleep.c 													\
		strcasecmp.c strcat.c strcmp.c strcpy.c strdup.c 						\
		strftime.c swab.c sysctl.c syserrlst.c syslog.c system.c				\
		telldir.c time.c timezone.c ttyname.c ttyslot.c ualarm.c uname.c 		\
		unvis.c usleep.c 														\
		valloc.c verr.c verrc.c verrx.c vis.c vwarn.c vwarnc.c vwarnx.c 		\
		wait.c wait3.c waitpid.c warn.c warnc.c warnx.c

# machine-dependent gen sources
.include "${ARCHDIR}/gen/Makefile.inc"
