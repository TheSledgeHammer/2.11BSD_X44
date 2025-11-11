/*
 * Copyright (c) 1988 The Regents of the University of California.
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
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	@(#)pwd.h	4.3 (Berkeley) 2/22/89
 */

#ifndef _PWD_H_
#define	_PWD_H_

#include <sys/cdefs.h>
#include <sys/types.h>

#define	_PATH_PASSWD		"/etc/passwd"
#define	_PATH_MASTERPASSWD	"/etc/master.passwd"
#define	_PATH_MKPASSWD		"/etc/mkpasswd"
#define	_PATH_PTMP		"/etc/ptmp"
#define	_PATH_MASTERPASSWD_LOCK	_PATH_PTMP

#define	_PATH_PASSWD_CONF	"/etc/passwd.conf"
#define	_PATH_PASSWDCONF	_PATH_PASSWD_CONF	/* XXX: compat */
#define	_PATH_USERMGMT_CONF	"/etc/usermgmt.conf"

#define	_PATH_MP_DB		"/etc/pwd.db"
#define	_PATH_SMP_DB		"/etc/spwd.db"

#define	_PATH_PWD_MKDB		"/usr/sbin/pwd_mkdb"

#define	_PW_KEYBYNAME		'0'		/* stored by name */
#define	_PW_KEYBYUID		'1'		/* stored by uid */
#define	_PW_KEYBYNUM		'2'		/* stored by entry in the "file" */

#define	_PASSWORD_EFMT1		'_'		/* extended DES encryption format */
#define	_PASSWORD_NONDES	'$'		/* non-DES encryption formats */

#define	_PASSWORD_LEN		128		/* max length, not counting NUL */

#define _PASSWORD_NOUID		0x01		/* flag for no specified uid. */
#define _PASSWORD_NOGID		0x02		/* flag for no specified gid. */
#define _PASSWORD_NOCHG		0x04		/* flag for no specified change. */
#define _PASSWORD_NOEXP		0x08		/* flag for no specified expire. */

#define _PASSWORD_OLDFMT	0x10		/* flag to expect an old style entry */
#define _PASSWORD_NOWARN	0x20		/* no warnings for bad entries */

#define _PASSWORD_WARNDAYS	14		/* days to warn about expiry */
#define _PASSWORD_CHGNOW	-1		/* special day to force password change at next login */

struct passwd {
	char	*pw_name;			/* user name */
	char	*pw_passwd;			/* encrypted password */
	int		pw_uid;				/* user uid */
	int		pw_gid;				/* user gid */
	time_t	pw_change;			/* password change time */
	char	*pw_class;			/* user access class */
	char	*pw_gecos;			/* Honeywell login info */
	char	*pw_dir;			/* home directory */
	char	*pw_shell;			/* default shell */
	time_t	pw_expire;			/* account expiration */
};

__BEGIN_DECLS
#if (_POSIX_C_SOURCE - 0) >= 199506L || (_XOPEN_SOURCE - 0) >= 500 || \
    defined(_REENTRANT) || defined(__BSD_VISIBLE)
int		 getpwent_r(struct passwd *, char *, size_t, struct passwd **);
int		 getpwuid_r(uid_t, struct passwd *, char *, size_t, struct passwd **);
int		 getpwnam_r(const char *, struct passwd *, char *, size_t, struct passwd **);
#endif

struct passwd 	*getpwent(void);
struct passwd 	*getpwuid(uid_t);
struct passwd 	*getpwnam(const char *);
void		setpwent(void);
void	 	endpwent(void);
int 		setpassent(int);
void            setpwfile(const char *);
int		pw_gensalt(char *, size_t, const char *, const char *);
int		pw_scan(char *, struct passwd *, int *);
const char	*user_from_uid(uid_t, int);
int		uid_from_user(const char *, uid_t *);
int		pwcache_userdb(int (*)(int), void (*)(void), struct passwd * (*)(const char *), struct passwd * (*)(uid_t));
__END_DECLS
#endif /* !_PWD_H_ */
