/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)ttyent.h	5.1 (Berkeley) 5/30/85
 */


#ifndef	_TTYENT_H_
#define	_TTYENT_H_

#include <sys/cdefs.h>

#define	_PATH_TTYS		"/etc/ttys"

#define	_TTYS_OFF		"off"
#define	_TTYS_ON		"on"
#define	_TTYS_SECURE	"secure"
#define	_TTYS_WINDOW	"window"
#define	_TTYS_CLASS		"class"
#define	_TTYS_LOCAL		"local"
#define	_TTYS_RTSCTS	"rtscts"
#define	_TTYS_DTRCTS    "dtrcts"
#define	_TTYS_SOFTCAR	"softcar"
#define	_TTYS_MDMBUF	"mdmbuf"

struct	ttyent { 			/* see getttyent(3) */
	char	*ty_name;		/* terminal device name */
	char	*ty_getty;		/* command to execute, usually getty */
	char	*ty_type;		/* terminal type for termcap (3X) */
	int		ty_status;		/* status flags (see below for defines) */
	char 	*ty_window;		/* command to start up window manager */
	char	*ty_comment;	/* usually the location of the terminal */
	char 	*ty_class;/* category of tty usage */
};

#define TTY_ON		0x01	/* enable logins (startup getty) */
#define TTY_SECURE	0x02	/* allow root to login */
#define	TTY_LOCAL	0x04	/* set 'CLOCAL' on open (dev. specific) */
#define	TTY_RTSCTS	0x08	/* set 'CRTSCTS' on open (dev. specific) */
#define	TTY_SOFTCAR	0x10	/* ignore hardware carrier (dev. spec.) */
#define	TTY_MDMBUF	0x20	/* set 'MDMBUF' on open (dev. specific) */
#define TTY_DTRCTS  0x40    /* set 'CDTRCTS' on open (dev. specific) */

__BEGIN_DECLS
struct ttyent *getttyent(void);
struct ttyent *getttynam(const char *);
int setttyent(void);
int endttyent(void);
__END_DECLS

#endif /* !_TTYENT_H_ */
