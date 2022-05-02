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
 *	@(#)unpcb.h	7.2 (Berkeley) 1/7/88
 */
#ifndef _SYS_UNPCB_H_
#define _SYS_UNPCB_H_
/*
 * Protocol control block for an active
 * instance of a UNIX internal protocol.
 *
 * A socket may be associated with an inode in the
 * file system.  If so, the unp_inode pointer holds
 * a reference count to this inode, which should be irele'd
 * when the socket goes away.
 *
 * A socket may be connected to another socket, in which
 * case the control block of the socket to which it is connected
 * is given by unp_conn.
 *
 * A socket may be referenced by a number of sockets (e.g. several
 * sockets may be connected to a datagram socket.)  These sockets
 * are in a linked list starting with unp_refs, linked through
 * unp_nextref and null-terminated.  Note that a socket may be referenced
 * by a number of other sockets and may also reference a socket (not
 * necessarily one which is referencing it).  This generates
 * the need for unp_refs and unp_nextref to be separate fields.
 *
 * Stream sockets keep copies of receive sockbuf sb_cc and sb_mbcnt
 * so that changes in the sockbuf may be computed to modify
 * back pressure on the sender accordingly.
 */
struct	unpcb {
	struct socket 	*unp_socket;	/* pointer back to socket */
	struct vnode 	*unp_vnode;		/* if associated with file */
	ino_t			unp_ino;		/* fake inode number */
	struct unpcb 	*unp_conn;		/* control block of connected socket */
	struct unpcb 	*unp_refs;		/* referencing socket linked list */
	struct unpcb 	*unp_nextref;	/* link in unp_refs list */
	struct mbuf		*unp_addr;		/* bound address of socket */
	size_t			unp_addrlen;	/* size of socket address */
	int				unp_cc;			/* copy of rcv.sb_cc */
	int				unp_mbcnt;		/* copy of rcv.sb_mbcnt */
	struct timespec unp_ctime;		/* holds creation time */
	int				unp_flags;		/* misc flags; see below*/
};

#define	sotounpcb(so)	((struct unpcb *)((so)->so_pcb))
#endif /* _SYS_UNPCB_H_ */
