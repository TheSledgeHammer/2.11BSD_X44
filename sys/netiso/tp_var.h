/*	$NetBSD: tp_var.h,v 1.6 2003/06/29 22:32:07 fvdl Exp $	*/

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

#ifndef _NETISO_TP_VAR_H_
#define _NETISO_TP_VAR_H_

#ifdef _KERNEL
struct isopcb;
struct mbuf;
struct sockaddr_iso;
struct tp_pcb;
struct sockaddr_in;
struct iso_addr;
struct tp_conn_param;
struct tp_event;
struct inpcb;
struct route;
struct pklcd;
struct sockaddr;
struct x25_packet;
struct in_addr;

/* tp_driver.c */
int tp_driver(struct tp_pcb *, struct tp_event *);

/* tp_emit.c */
int tp_emit(int, struct tp_pcb *, SeqNum, u_int, struct mbuf *);
int tp_error_emit(int, u_long, struct sockaddr_iso *,
		       struct sockaddr_iso *, struct mbuf *, int,
		       struct tp_pcb *, caddr_t, 
		       int (*)(struct mbuf *, ...));

/* tp_input.c */
struct mbuf *tp_inputprep(struct mbuf *);
void tp_input(struct mbuf *, ...);
int tp_headersize(int, struct tp_pcb *);

/* tp_meas.c */
void Tpmeas(u_int, u_int, struct timeval *, u_int, u_int, u_int);

/* tp_output.c */
int tp_consistency(struct tp_pcb *, u_int, struct tp_conn_param *);
int tp_ctloutput(int, struct socket *, int, int, struct mbuf **);

/* tp_subr.c */
int tp_goodXack(struct tp_pcb *, SeqNum);
void tp_rtt_rtv(struct tp_pcb *);
int tp_goodack(struct tp_pcb *, u_int, SeqNum, u_int);
int tp_sbdrop(struct tp_pcb *, SeqNum);
void tp_send(struct tp_pcb *);
int tp_packetize(struct tp_pcb *, struct mbuf *, int);
int tp_stash(struct tp_pcb *, struct tp_event *);
void tp_rsyflush(struct tp_pcb *);
void tp_rsyset(struct tp_pcb *);
void tpsbcheck(struct tp_pcb *, int);

/* tp_subr2.c */
void tp_local_credit(struct tp_pcb *);
int tp_protocol_error(struct tp_event *, struct tp_pcb *);
void tp_drain(void);
void tp_indicate(int, struct tp_pcb *, u_short);
void tp_getoptions(struct tp_pcb *);
void tp_recycle_tsuffix(void *);
void tp_quench(struct tp_pcb *, int);
void tp_netcmd(struct tp_pcb *, int);
int tp_mask_to_num(u_char);
void tp_mss(struct tp_pcb *, int);
int tp_route_to(struct mbuf *, struct tp_pcb *, caddr_t);
void tp0_stash(struct tp_pcb *, struct tp_event *);
void tp0_openflow(struct tp_pcb *);
int tp_setup_perf(struct tp_pcb *);
void dump_addr(struct sockaddr *);
void Dump_buf(caddr_t, int);

/* tp_timer.c */
void tp_timerinit(void);
void tp_etimeout(struct tp_pcb *, int, int);
void tp_euntimeout(struct tp_pcb *, int);
void tp_slowtimo(void);
void tp_data_retrans(struct tp_pcb *);
void tp_fasttimo(void);
void tp_ctimeout(struct tp_pcb *, int, int);
void tp_ctimeout_MIN(struct tp_pcb *, int, int);
void tp_cuntimeout(struct tp_pcb *, int);

/* tp_trace.c */
void tpTrace(struct tp_pcb *, u_int, u_int, u_int, u_int, u_int,
		     u_int);

/* tp_usrreq.c */
void dump_mbuf(struct mbuf *, char *);
int tp_rcvoob(struct tp_pcb *, struct socket *, struct mbuf *,
		     int *, int);
int tp_sendoob(struct tp_pcb *, struct socket *, struct mbuf *, int *);
int tp_usrreq(struct socket *, int, struct mbuf *, struct mbuf *,
		     struct mbuf *, struct proc *);
void tp_ltrace(struct socket *, struct uio *);
int tp_confirm(struct tp_pcb *);
int tp_snd_control(struct mbuf *, struct socket *, struct mbuf **);

#ifdef TPCONS
/* if_cons.c */
void nibble_copy(char *, unsigned, char *, unsigned, int);
int nibble_match(char *, unsigned, char *, unsigned, int);
void cons_init(void);
int tp_incoming(struct mbuf *, void *);
int cons_tpinput(struct mbuf *, void *);
int cons_connect(struct isopcb *);
void *cons_ctlinput(int, struct sockaddr *, void *);
int find_error_reason(struct x25_packet *);
#endif

#endif

#endif	/* _NETISO_TP_VAR_H_ */
