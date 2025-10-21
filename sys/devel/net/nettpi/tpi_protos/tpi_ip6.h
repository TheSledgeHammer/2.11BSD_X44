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
 *	@(#)tp_ip.h	8.1 (Berkeley) 6/10/93
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
 * $Header: tp_ip.h,v 5.1 88/10/12 12:19:47 root Exp $
 * $Source: /usr/argo/sys/netiso/RCS/tp_ip.h,v $
 *
 * internet IP6-dependent structures and include files
 *
 */
#ifndef _NETTPI_IP6_H_
#define _NETTPI_IP6_H_

#ifndef SOCK_STREAM
#include <sys/socket.h>
#endif

#include <net/route.h>

#include <netinet6/in6.h>
#include <netinet6/in6_pcb.h>
#include <netinet6/ip6_var.h>

struct in6pcb tp_in6pcb;		/* queue of active inpcbs for tp ; for tp with dod ip */

void in6_getsufx(void *, u_short *, caddr_t, int);
void in6_putsufx(void *, caddr_t, int, int);
void in6_recycle_tsuffix(void *);
void in6_putnetaddr(void *, struct sockaddr *, int);
int in6_cmpnetaddr(void *, struct sockaddr *, int);
void in6_getnetaddr(void *, struct mbuf *, int);

int tpip6_mtu(struct tpipcb *);
int tpip6_output(void *, struct mbuf *, int, int);
int tpip6_output_dg(void *, void *, struct mbuf *, int, void *, int);
int  tpip6_output_sc(struct mbuf *m0, ...);
int tpip6_input(struct mbuf *, int);
void tpip6_quench(struct in6pcb *, int);
void tpip6_abort(struct in6pcb *, int);
void *tpip6_ctlinput(int, struct sockaddr *, void *);

void tpin6_quench(struct in6pcb *);
void tpin6_abort(struct in6pcb *);

#endif /* _NETTPI_IP6_H_ */
