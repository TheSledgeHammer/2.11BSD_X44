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
void	loginit(void);
int     logopen(dev_t, int);
int     logclose(dev_t, int);
int     logisopen(int);
int     logread(dev_t, struct uio *, int);
int     logselect(dev_t, int);
void    logwakeup(int);
int     logioctl(dev_t, u_int, caddr_t, int);
int     logwrt(char *, int, int);
#endif
#endif /* !_SYS_MSGBUF_H_ */
