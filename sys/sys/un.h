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

#ifdef _KERNEL
#include <sys/unpcb.h>
#endif /* KERNEL */

/*
 * Definitions for UNIX IPC domain.
 */
struct	sockaddr_un {
	short	sun_family;			/* AF_UNIX */
	char	sun_path[108];		/* path name (gag) */
};

#ifdef _KERNEL
int     uipc_usrreq(struct socket *, int, struct mbuf *, struct mbuf *, struct mbuf *);
int     unp_attach(struct socket *);
void    unp_detach(struct unpcb *);
int     unp_bind(struct unpcb *,struct mbuf *);
int     unp_connect(struct socket *, struct mbuf*);
int	    unp_connect2(struct socket*,struct socket*);
void    unp_disconnect(struct unpcb *);
void    unp_shutdown(struct unpcb *);
void    unp_drop(struct unpcb *, int);
int	    unp_externalize(struct mbuf *);
int	    unp_internalize(struct mbuf *);
void    unp_gc(void);
int	    unp_dispose(struct mbuf *);
void	unp_scan(struct mbuf *, void (*)(struct file *));
void	unp_mark(struct file *);
void	unp_discard(struct file *);

#else /* KERNEL */
/* actual length of an initialized sockaddr_un */
#define SUN_LEN(su) \
	(sizeof(*(su)) - sizeof((su)->sun_path) + strlen((su)->sun_path))
#endif

#endif /* !_SYS_UN_H_ */
