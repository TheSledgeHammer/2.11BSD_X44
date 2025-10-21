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
 * 3. Neither the name of the University nor the names of its contributors
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
 *	@(#)tp_iso.c	8.2 (Berkeley) 9/22/94
 */

#ifndef _NETTPI_NS_H_
#define _NETTPI_NS_H_

#ifndef SOCK_STREAM
#include <sys/socket.h>
#endif

#include <net/route.h>

#include <netns/ns.h>
#include <netns/ns_pcb.h>
#include <netns/ns_var.h>

struct nspcb tp_nspcb;		/* queue of active inpcbs for tp ; for tp with dod ip */

void in_getsufx(void *, u_short *, caddr_t, int);
void in_putsufx(void *, caddr_t, int, int);
void in_recycle_tsuffix(void *);
void in_putnetaddr(void *, struct sockaddr *, int);
int in_cmpnetaddr(void *, struct sockaddr *, int);
void in_getnetaddr(void *, struct mbuf *, int);

int tpidp_mtu(struct tpipcb *);
int tpidp_output(void *, struct mbuf *, int, int);
int tpidp_output_dg(void *, void *, struct mbuf *, int, void *, int);
int tpidp_input(struct mbuf *, int);
void tpidp_quench(struct nspcb *, int);
void tpidp_abort(struct nspcb *, int);
void *tpidp_ctlinput(int, struct sockaddr *, void *);

void tpns_quench(struct nspcb *);
void tpns_abort(struct nspcb *);

#endif /* _NETTPI_NS_H_ */
