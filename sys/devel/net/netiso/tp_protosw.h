/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)tp_pcb.h	8.2 (Berkeley) 9/22/94
 */

/***********************************************************
		Copyright IBM Corporation 1987

                      All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of IBM not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

IBM DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
IBM BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

/*
 * ARGO Project, Computer Sciences Dept., University of Wisconsin - Madison
 */
/*
 * ARGO TP
 *
 * $Header: tp_pcb.h,v 5.2 88/11/18 17:09:32 nhall Exp $
 * $Source: /usr/argo/sys/netiso/RCS/tp_pcb.h,v $
 *
 *
 * This file defines the transport protocol control block (tpcb).
 * and a bunch of #define values that are used in the tpcb.
 */

#ifndef _NETTPI_TPI_PROTOSW_H_
#define _NETTPI_TPI_PROTOSW_H_

struct tp_protosw {
	int		tp_afamily;												/* address family */
	//void 	(*tp_init)();
	void	(*tp_putnetaddr)(void *, struct sockaddr *, int);		/* puts addresses in tp pcb */
	void	(*tp_getnetaddr)(void *, struct mbuf *, int);			/* gets addresses from tp pcb */
	int		(*tp_cmpnetaddr)(void *, struct sockaddr *, int);		/* compares address in pcb with sockaddr */
	int		(*tp_putsufx)(void *, caddr_t, int, int);				/* puts transport suffixes in tp pcb */
	int		(*tp_getsufx)(void *, u_short *, caddr_t, int);			/* gets transport suffixes from tp pcb */
	int		(*tp_recycle_suffix)(void *);							/* clears suffix from tp pcb */
	int		(*tp_mtu)(void *);										/* figures out mtu based on tp used */
	int		(*tp_pcbbind)(void *);									/* bind to pcb for net level */
	int		(*tp_pcbconnect)(void *, struct mbuf *);				/* connect for net level */
	void	(*tp_pcbdisconnect)(void *);							/* disconnect net level */
	int 	(*tp_pcbattach)(struct socket *, int);					/* attach net level pcb */
	void	(*tp_pcbdetach)(void *);								/* detach net level pcb */
	int		(*tp_pcballoc)(struct socket *, void *);				/* allocate a net level pcb */
	int		(*tp_output)(void *, struct mbuf *, int, int);			/* prepare a packet to give to tp */
	int		(*tp_dgoutput)(void *, void *, struct mbuf *, int, void *, int); /* prepare a packet to give to tp */
	int		(*tp_ctloutput)(int, struct sockaddr *, void *);		/* hook for network set/get options */
	caddr_t	tp_pcblist;												/* list of xx_pcb's for connections */
};

extern struct tp_protosw *tp_protosw;

/* network protocols */
extern struct tp_protosw tpin4_protosw;
extern struct tp_protosw tpin6_protosw;
extern struct tp_protosw tpiso_protosw;
extern struct tp_protosw tpns_protosw;
extern struct tp_protosw tpx25_protosw;

void tp_protosw_init(void);

#endif /* _NETTPI_TPI_PROTOSW_H_ */
