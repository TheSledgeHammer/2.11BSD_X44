/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 *
 *	@(#)un.h	7.2 (Berkeley) 12/30/87
 */

#ifndef _SYS_UN_H_
#define _SYS_UN_H_

#ifdef KERNEL
#include <sys/unpcb.h>
#endif /* KERNEL */

/*
 * Definitions for UNIX IPC domain.
 */
struct	sockaddr_un {
	short	sun_family;			/* AF_UNIX */
	char	sun_path[108];		/* path name (gag) */
};

#ifdef KERNEL
int		unp_connect2 __P((struct socket*,struct socket*));
void    unp_detach __P((struct unpcb *));
void    unp_disconnect __P((struct unpcb *));
void    unp_shutdown __P((struct unpcb *));
void    unp_drop __P((struct unpcb *, int));
void    unp_gc __P((void));
void    unp_scan __P((struct mbuf *, void (*)(struct file *)));
void    unp_mark __P((struct file *));
void    unp_discard __P((struct file *));
int     unp_attach __P((struct socket *));
int     unp_bind __P((struct unpcb *,struct mbuf *, struct proc *));
int     unp_connect __P((struct socket *,struct mbuf *, struct proc *));
int     unp_internalize __P((struct mbuf *, struct proc *));
#else /* KERNEL */
/* actual length of an initialized sockaddr_un */
#define SUN_LEN(su) \
	(sizeof(*(su)) - sizeof((su)->sun_path) + strlen((su)->sun_path))
#endif

#endif
