/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)msgbuf.h	1.2 (2.11BSD) 1998/12/5
 */

#ifndef _SYS_MSGBUF_H_
#define _SYS_MSGBUF_H_

#define	MSG_MAGIC	0x063061
#define	MSG_BSIZE	4096

struct	msgbuf {
	long	msg_magic;
	long	msg_bufx;	/* write pointer */
	long	msg_bufr;	/* read pointer */
	long	msg_click;	/* real msg_bufc size (bytes) */
	char	*msg_bufc;	/* buffer */
};

#define	logMSG	0		/* /dev/klog */
#define	logDEV	1		/* /dev/erlg */
#define	logACCT	2		/* /dev/acct */

#ifdef _KERNEL
struct	msgbuf *msgbufp;

void	loginit(void);
int     logisopen(int);
void    logwakeup(int);
int     logwrt(char *, int, int);
#endif
#endif /* !_SYS_MSGBUF_H_ */
