/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 *
 *	@(#)if_sl.c	7.6.1.1 (Berkeley) 3/15/88
 */

#ifndef _NET_IF_SLVAR_H_
#define _NET_IF_SLVAR_H_

struct sl_softc {
	struct	ifnet 	sc_if;			/* network-visible interface */
	short			sc_flags;		/* see below */
	short			sc_ilen;		/* length of input-packet-so-far */
	struct	tty 	*sc_ttyp;		/* pointer to tty structure */
	char			*sc_mp;			/* pointer to next available buf char */
	char			*sc_buf;		/* input buffer */
} sl_softc[NSL];

extern int	slopen(dev_t, struct tty *);
extern int	slclose(struct tty *, int);
extern int	slinput(int, struct tty *);
extern int	slioctl(struct ifnet *, int, caddr_t);

extern int	sloutput(struct ifnet *, struct mbuf *, const struct sockaddr *);
extern int	slstart(struct tty *);
extern int	sltioctl(struct tty *, u_long, void *, int, struct proc *);
#endif /* _NET_IF_SLVAR_H_ */
