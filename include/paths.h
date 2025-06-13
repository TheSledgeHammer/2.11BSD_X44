/*
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	@(#)pathnames.h	5.3.7 (2.11BSD) 1996/11/27
 */
#ifndef _PATHS_H_
#define	_PATHS_H_

/* Default search path. */
#ifdef RESCUEDIR
#define	_PATH_DEFPATH	RESCUEDIR ":/usr/bin:/bin:/usr/local/bin"
#else
#define	_PATH_DEFPATH	"/usr/bin:/bin:/usr/local/bin"
#endif

/* All standard utilities path. */
#define	_PATH_STDPATH \
	"/usr/bin:/bin:/usr/sbin:/sbin:/usr/contrib/bin:/usr/old/bin"

#define _PATH_DEVTAB_PATHS \
	"/usr/local/etc:/etc:/etc/defaults"

#define	_PATH_BSHELL	"/bin/sh"
#define	_PATH_CSHELL	"/bin/csh"
#define	_PATH_CP	"/bin/cp"
//#define	_PATH_RSH		"/usr/ucb/rsh"
//#define	_PATH_VI		"/usr/ucb/vi"

#define	_PATH_AUDIO	"/dev/audio"
#define	_PATH_AUDIO0	"/dev/audio0"
#define	_PATH_AUDIOCTL	"/dev/audioctl"
#define	_PATH_AUDIOCTL0	"/dev/audioctl0"
#define	_PATH_BPF	"/dev/bpf"
#define	_PATH_CONSOLE	"/dev/console"
#define	_PATH_CONSTTY	"/dev/constty"
#define	_PATH_CTIMED	"/usr/libexec/ctimed"
#define	_PATH_DEFTAPE	"/dev/rst0"
#define	_PATH_DEVCDB	"/var/run/dev.cdb"
#define	_PATH_DEVDB	"/var/run/dev.db"
#define	_PATH_DEVNULL	"/dev/null"
#define	_PATH_DEVZERO	"/dev/zero"
#define	_PATH_DRUM	"/dev/drum"
#define	_PATH_FTPUSERS	"/etc/ftpusers"
#define	_PATH_GETTYTAB	"/etc/gettytab"
#define	_PATH_KMEM	"/dev/kmem"
#define	_PATH_KSYMS	"/dev/ksyms"
#define	_PATH_KVMDB	"/var/db/kvm.db"
#define	_PATH_LOCALE	"/usr/share/locale"
#define	_PATH_LASTLOG	"/usr/adm/lastlog"
#define	_PATH_MAILDIR	"/var/mail"
#define	_PATH_MAN	"/usr/share/man"
#define	_PATH_MEM	"/dev/mem"
#define	_PATH_NOLOGIN	"/etc/nologin"
#define	_PATH_PRINTCAP	"/etc/printcap"
#define	_PATH_SENDMAIL	"/usr/sbin/sendmail"
#define	_PATH_SHELLS	"/etc/shells"
#define	_PATH_SKEYKEYS	"/etc/skeykeys"
#define	_PATH_SOUND	"/dev/sound"
#define	_PATH_SOUND0	"/dev/sound0"
#define	_PATH_TTY	"/dev/tty"
#define	_PATH_UNIX	"/kernel/twobsd" /* aka. PATH_KERNEL (sys/boot.h) */
#define	_PATH_VIDEO	"/dev/video"
#define	_PATH_VIDEO0	"/dev/video0"
//#define	_PATH_DEV		"/dev"
//#define	_PATH_TMP		"/tmp/"
//#define	_PATH_VARRUN	"/var/run/"
//#define	_PATH_VARTMP	"/usr/tmp/"

/*
 * Provide trailing slash, since mostly used for building pathnames.
 * See the __CONCAT() macro from <sys/cdefs.h> for cpp examples.
 */
#define	_PATH_DEV	"/dev/"
#define	_PATH_DEV_PTS	"/dev/pts/"
#define	_PATH_EMUL_AOUT	"/emul/aout/"
#define	_PATH_TMP	"/tmp/"
#define	_PATH_VARDB	"/var/db/"
#define	_PATH_VARRUN	"/var/run/"
#define	_PATH_VARTMP	"/var/tmp/"

/*
 * Paths that may change if RESCUEDIR is defined.
 * Used by tools in /rescue.
 */
#ifdef RESCUEDIR
#define	_PATH_BSHELL	RESCUEDIR "/sh"
#define	_PATH_CSHELL	RESCUEDIR "/csh"
#define	_PATH_VI	RESCUEDIR "/vi"
#else
#define	_PATH_BSHELL	"/bin/sh"
#define	_PATH_CSHELL	"/bin/csh"
#define	_PATH_VI	"/usr/bin/vi"
#endif

#endif /* !_PATHS_H_ */
