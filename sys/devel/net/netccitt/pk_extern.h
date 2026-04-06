/*	$NetBSD: pk_extern.h,v 1.12 2003/06/29 22:31:55 fvdl Exp $	*/

/*-
 * Copyright (c) 1995 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _NETCCITT_PK_EXTERN_H_
#define _NETCCITT_PK_EXTERN_H_

#ifdef _KERNEL

#include <netccitt/x25isr.h>

struct pklcd;
struct mbuf_cache;
struct x25_ifaddr;
struct x25_calladdr;
struct x25_packet;
struct llc_linkcb;
struct bcdinfo;
struct sockaddr_dl;

/* pk_acct.c */
int pk_accton(char *);
void pk_acct(struct pklcd *);

/* pk_debug.c */
void pk_trace(struct x25config *, struct mbuf *, char *);
void mbuf_cache(struct mbuf_cache *, struct mbuf *);

/* pk_input.c */
struct pkcb *pk_newlink(struct x25_ifaddr *, caddr_t);
int pk_dellink(struct pkcb *);
int pk_resize(struct pkcb *);
void *pk_ctlinput(int, struct sockaddr *, void *);
void pkintr(void);
void pk_input(struct mbuf *, ...);
void pk_simple_bsd(octet *, octet *, int, int);
void pk_from_bcd(struct x25_calladdr *, int, struct sockaddr_x25 *, struct x25config *);
void pk_incoming_call(struct pkcb *, struct mbuf *);
void pk_call_accepted(struct pklcd *, struct mbuf *);
void pk_parse_facilities(octet *, struct sockaddr_x25 *);

/* pk_llc.c */
void cons_rtrequest(int, struct rtentry *, struct rt_addrinfo *);
struct rtentry *llc_sapinfo_enter(struct sockaddr_dl *, struct sockaddr *, struct rtentry *, struct llc_linkcb *);
struct rtentry *llc_sapinfo_enrich(short, caddr_t, struct sockaddr_dl *);
int llc_sapinfo_destroy(struct rtentry *);
int x25_llc(int, struct sockaddr *);

/* Compatibility */
#define npaidb_enter(key, value, rt, link) 	llc_sapinfo_enter((key), (value), (rt), (link))
#define npaidb_enrich(type, info, sdl)	 	llc_sapinfo_enrich((type), (info), (sdl))
#define npaidb_destroy(rt)					llc_sapinfo_destroy((rt))
#define x25_llcglue(prc, addr)				x25_llc((prc), (addr))

/* pk_output.c */
void pk_output(struct pklcd *);
struct mbuf *nextpk(struct pklcd *);

/* pk_subr.c */
struct pklcd *pk_attach(struct socket *);
void pk_disconnect(struct pklcd *);
void pk_close(struct pklcd *);
struct mbuf *pk_template(int, int);
void pk_restart(struct pkcb *, int);
void pk_freelcd(struct pklcd *);
int pk_bind(struct pklcd *, struct mbuf *);
int pk_listen(struct pklcd *);
int pk_protolisten(int, int, int (*)(struct mbuf *, void *));
void pk_assoc(struct pkcb *, struct pklcd *, struct sockaddr_x25 *);
int pk_connect(struct pklcd *, struct sockaddr_x25 *);
void pk_callcomplete(struct pkcb *);
void pk_callrequest(struct pklcd *, struct sockaddr_x25 *, struct x25config *);
void pk_build_facilities(struct mbuf *, struct sockaddr_x25 *, int);
int to_bcd(struct bcdinfo *, struct sockaddr_x25 *, struct x25config *);
int pk_getlcn(struct pkcb *);
void pk_clear(struct pklcd *, int, int);
void pk_flowcontrol(struct pklcd *, int, int);
void pk_flush(struct pklcd *);
void pk_procerror(int, struct pklcd *, char *, int);
int pk_ack(struct pklcd *, unsigned int);
int pk_decode(struct x25_packet *);
void pk_restartcause(struct pkcb *, struct x25_packet *);
void pk_resetcause(struct pkcb *, struct x25_packet *);
void pk_clearcause(struct pkcb *, struct x25_packet *);
char *format_ntn(struct x25config *);
void pk_message(int, struct x25config *, char *, ...) __attribute__((__format__(__printf__, 3, 4)));

int pk_fragment(struct pklcd *, struct mbuf *, int, int, int);

/* pk_timer.c */
void pk_timer(void);

/* pk_usrreq.c */
int pk_usrreq(struct socket *, int, struct mbuf *, struct mbuf *, struct mbuf *, struct proc *);
int pk_start(struct pklcd *);
int pk_control(struct socket *, u_long, caddr_t, struct ifnet *, struct proc *);
int pk_ctloutput(int, struct socket *, int, int, struct mbuf **);
int pk_checksockaddr(struct mbuf *);
int pk_send(struct mbuf *, void *);

#endif

#endif /* _NETCCITT_PK_EXTERN_H_ */
