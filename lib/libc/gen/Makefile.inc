#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.7.11 (2.11BSD) 1997/12/31
#
# Optional: USE_ARC4, USE_NDBM and USE_SGTTY
# To enable or disable the below options, please see bsd.own.mk.
# Modify the USE_* (i.e. USE_ARC4, USE_NDBM, etc) and change the
# variable to "yes" or "no".
# PLEASE DO NOT MODIFY THESE DIRECTLY!!

# machine-independent gen sources
.PATH: ${ARCHDIR}/gen ${.CURDIR}/gen

# use arc4 instead of chacha (0 = yes, 1 = no)
.if (${USE_ARC4} != "no")
RUN_ARC4?= 0
.else
RUN_ARC4?= 1
.endif

# use ndbm instead of db (0 = yes, 1 = no)
.if (${USE_NDBM} != "no")
RUN_NDBM?= 0
SRCS+= pw_ndbm.c
.else
RUN_NDBM?= 1
SRCS+= pw_db.c
.endif

# use sgtty instead of termios (0 = yes, 1 = no)
.if (${USE_SGTTY} != "no")
RUN_SGTTY?= 0
.else
RUN_SGTTY?= 1
.endif

SRCS+=	alarm.c arc4random.c assert.c basename.c bswap.c closedir.c confstr.c ctermid.c daemon.c devname.c  	\
        dirname.c disklabel.c ecvt.c err.c errlst.c errno.c execvp.c fabs.c fmtcheck.c fnmatch.c fstab.c fts.c gcvt.c 		\
	getbsize.c getcap.c getcwd.c getdevpath.c getdomainname.c getentropy.c getgrent.c getlogin.c        \
        getgrouplist.c gethostname.c getpagesize.c getpass.c getprogname.c getpwent.c getloadavg.c \
	getmntinfo.c getnetgrent.c getttyent.c getttynam.c getusershell.c getvfsbyname.c getwd.c glob.c          	\
        initgroups.c insque.c isatty.c l3.c lockf.c nlist.c nlist_aout.c nlist_coff.c nlist_ecoff.c         	\
        nlist_elf32.c nlist_elf64.c nlist_xcoff32.c nlist_xcoff64.c opendir.c popen.c psignal.c 	\
	pw_scan.c pwcache.c raise.c readdir.c rewinddir.c regex.c remque.c scandir.c seekdir.c 		\
	setdomainname.c sethostname.c setjmperr.c setmode.c setproctitle.c shquote.c siginterrupt.c 	\
	siglist.c signal.c sigsetops.c sleep.c stringlist.c sysconf.c sysctl.c syserrlst.c syslog.c 	\
	telldir.c time.c timezone.c ttyname.c ttyslot.c ualarm.c usleep.c utmp.c utmpx.c valloc.c 	\
	vis.c uname.c unvis.c wait.c wait3.c waitpid.c

# uses alloca
#COPTS.execvp1.c = -Wno-stack-protector

# Removed: frexp.c

# machine-dependent gen sources
.include "${ARCHDIR}/gen/Makefile.inc"

MAN+=	alarm.3 arc4random.3 basename.3 confstr.3 ctermid.3 daemon.3 devname.3 dirname.3 err.3 		\
	fmtcheck.3 fts.3 getcap.3 getcwd.3 getdevpath.3 gethostname.3 getloadavg.3 getmntinfo.3 getnetgrent.3 	\
	getpass.3 getpwent.3 getttyent.3 getusershell.3 glob.3 initgroups.3 nlist.3 popen.3 		\
	psignal.3 pwcache.3 raise.3 scandir.3 setmode.3 setproctitle.3 shquote.3 siginterrupt.3 	\
	signal.3 sigsetops.3 sleep.3 stringlist.3 sysconf.3 sysctl.3 syslog.3 time.3 timezone.3 	\
	ttyname.3 ualarm.3 uname.3 unvis.3 usleep.3 valloc.3 vis.3
