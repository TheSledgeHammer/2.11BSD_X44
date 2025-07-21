/*
 * Copyright (c) 1982, 1986, 1990, 1993
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
 *	@(#)in_pcb.h	8.1 (Berkeley) 6/10/93
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

#ifndef _NETTPI_TPI_PCB_H_
#define _NETTPI_TPI_PCB_H_

#include <sys/queue.h>

#define REF_FROZEN 		3	/* has ref timer only */
#define REF_OPEN 		2	/* has timers, possibly active */
#define REF_OPENING 		1	/* in use (has a pcb) but no timers */
#define REF_FREE 		0	/* free to reallocate */

#define TM_NTIMERS 		6

struct tpi_ref {
	struct tpipcb 			*tpr_pcb;	/* back ptr to PCB */
};

/* PER system stuff (one static structure instead of a bunch of names) */
struct tpi_refinfo {
	struct tpi_ref			*tpr_base;
	int				tpr_size;
	int				tpr_maxopen;
	int				tpr_numopen;
};

#define	MAX_TSAP_SEL_LEN	64

/* sockaddr union structure */
union tpi_sockaddr_union {
	struct sockaddr_in 		tsu_sin4;	/* ipv4 */
	struct sockaddr_in6 		tsu_sin6;	/* ipv6 */
	struct sockaddr_iso 		tsu_siso;	/* iso */
	struct sockaddr_ns 		tsu_sns; 	/* xns */
	struct sockaddr_x25		tsu_sx25;	/* x25 */
	// sna, ipx, atm, atalk?
};

/* addr union structure */
union tpi_addr_union {
	struct in_addr 			tau_in4;	/* ipv4 */
	struct in6_addr 		tau_in6;	/* ipv6 */
	struct iso_addr			tau_iso;	/* iso */
	struct ns_addr			tau_ns;		/* xns */
	struct x25_addr			tau_x25;	/* x25 */
};

/* Transport Local structure */
struct tpi_local {
	struct tpipcb_hdr		tpl_head;
	union tpi_sockaddr_union	tpl_lsockaddr;	/* local sockaddr */
	union tpi_addr_union		tpl_laddr;	/* local addr */
	u_int16_t			tpl_lport;	/* local port */
};

/* Transport Foreign structure */
struct tpi_foreign {
	struct tpipcb_hdr		tpf_head;
	union tpi_sockaddr_union	tpf_fsockaddr;	/* foreign sockaddr */
	union tpi_addr_union		tpf_faddr;	/* foreign addr */
	u_int16_t			tpf_fport;	/* foreign port */
};

/* Transport pcb structure */
struct tpipcb_hdr {
	CIRCLEQ_ENTRY(tpipcb_hdr) 	tpph_queue;
	LIST_ENTRY(tpipcb_hdr)    	tpph_fhash;     /* tp foreign hash */
	LIST_ENTRY(tpipcb_hdr)    	tpph_lhash;     /* tp local hash */
	int				tpph_af;	/* address family */
	caddr_t 			tpph_ppcb;	/* pointer to per-protocol pcb */
	int	  			tpph_state;	/* bind/connect state */
	struct socket 			*tpph_socket;	/* back pointer to socket */
	struct tpipcbtable 		*tpph_table;
};

LIST_HEAD(tpipcbhead, tpipcb_hdr);
CIRCLEQ_HEAD(tpipcbqueue, tpipcb_hdr);

struct tpipcbtable {
	struct tpipcbqueue	tppt_queue;
	struct tpipcbhead	*tppt_lhashtbl;
	struct tpipcbhead	*tppt_fhashtbl;
	u_long	  		tppt_lhash;
	u_long	  		tppt_fhash;
};

struct tpipcb {
	struct tpipcb_hdr	tpp_head;
#define tpp_lhash		tpp_head.tpph_lhash
#define tpp_fhash		tpp_head.tpph_fhash
#define tpp_queue		tpp_head.tpph_queue
#define tpp_af			tpp_head.tpph_af
#define tpp_ppcb		tpp_head.tpph_ppcb
#define tpp_state		tpp_head.tpph_state
#define tpp_socket		tpp_head.tpph_socket
#define tpp_table		tpp_head.tpph_table

	struct tpi_local	tpp_local;
#define tpp_lsockaddr		tpp_local.tpl_lsockaddr
#define	tpp_laddr 		tpp_local.tpl_laddr
#define	tpp_lport 		tpp_local.tpl_lport

	struct tpi_foreign	tpp_foreign;
#define tpp_fsockaddr		tpp_foreign.tpf_fsockaddr
#define	tpp_faddr 		tpp_foreign.tpf_faddr
#define	tpp_fport 		tpp_foreign.tpf_fport

	short 				tpp_retrans;	/* # times can still retrans */
	caddr_t				tpp_npcb;	/* to lower layer pcb */
	struct tpi_protosw	*tpp_tpproto;	/* lower-layer dependent routines */
	struct rtentry		**tpp_routep;	/* obtain mtu; inside npcb */

	RefNum				tpp_lref;	 	/* local reference */
	RefNum 				tpp_fref;		/* foreign reference */

	u_int				tpp_seqmask;	/* mask for seq space */
	u_int				tpp_seqbit;		/* bit for seq number wraparound */
	u_int				tpp_seqhalf;	/* half the seq space */

	u_long				tpp_cong_win;	/* congestion window in bytes.
										 * see profuse comments in TCP code
										 */

	u_long				tpp_rhiwat;		/* remember original RCVBUF size */

	/* parameters per-connection controllable by user */
	struct tpi_conn_param tpp_param;
#define	tpp_Nretrans 		tpp_param.p_Nretrans
#define	tpp_dr_ticks 		tpp_param.p_dr_ticks
#define	tpp_cc_ticks 		tpp_param.p_cc_ticks
#define	tpp_dt_ticks 		tpp_param.p_dt_ticks
#define	tpp_xpd_ticks		tpp_param.p_x_ticks
#define	tpp_cr_ticks 		tpp_param.p_cr_ticks
#define	tpp_keepalive_ticks tpp_param.p_keepalive_ticks
#define	tpp_sendack_ticks 	tpp_param.p_sendack_ticks
#define	tpp_refer_ticks 	tpp_param.p_ref_ticks
#define	tpp_inact_ticks 	tpp_param.p_inact_ticks
#define	tpp_xtd_format 		tpp_param.p_xtd_format
#define	tpp_xpd_service 	tpp_param.p_xpd_service
#define	tpp_ack_strat 		tpp_param.p_ack_strat
#define	tpp_rx_strat 		tpp_param.p_rx_strat
#define	tpp_use_checksum 	tpp_param.p_use_checksum
#define	tpp_use_efc 		tpp_param.p_use_efc
#define	tpp_use_nxpd 		tpp_param.p_use_nxpd
#define	tpp_use_rcc 		tpp_param.p_use_rcc
#define	tpp_tpdusize 		tpp_param.p_tpdusize
#define	tpp_class 			tpp_param.p_class
#define	tpp_winsize 		tpp_param.p_winsize
#define	tpp_no_disc_indications tpp_param.p_no_disc_indications
#define	tpp_dont_change_params 	tpp_param.p_dont_change_params
#define	tpp_netservice 		tpp_param.p_netservice
#define	tpp_version 		tpp_param.p_version
#define	tpp_ptpdusize 		tpp_param.p_ptpdusize

	int					tpp_l_tpdusize;

	u_char			 	tpp_flags;		/* values: */

	unsigned int 	 	tpp_perf_on:1;			/* 0/1 -> performance measuring on  */
	unsigned int	 	tpp_notdetached:1;		/* Call tp_detach before freeing XXXXXXX */

	/* addressing */
	u_short			 	tpp_domain;		/* domain (INET, ISO) */
	/* for compatibility with the *old* way and with INET, be sure that
	 * that lsuffix and fsuffix are aligned to a short addr.
	 * having them follow the u_short *suffixlen should suffice (choke)
	 */
	u_short			 	tpp_fsuffixlen;	/* foreign suffix */
	char			 	tpp_fsuffix[MAX_TSAP_SEL_LEN];
	u_short			 	tpp_lsuffixlen;	/* local suffix */
	char			 	tpp_lsuffix[MAX_TSAP_SEL_LEN];

	/* Timer stuff */
	u_char 			 	tpp_vers;			/* protocol version */

	u_char	 		 	tpp_refstate;		/* values REF_FROZEN, etc. above */
};

typedef unsigned short RefNum;
typedef unsigned int SeqNum;

/* flags for which */
#define TPI_LOCAL 		0x01
#define TPI_FOREIGN 		0x02

/* states */
#define TPI_ATTACHED		0
#define TPI_BOUND		1
#define TPI_CONNECTED		2

#define TPI_CLOSED		3
#define TPI_OPEN		4

#define	sototpcb(so) 	((struct tpipcb *)(so->so_pcb))
#define	sototpref(so)	((sototpcb(so)->tp_ref))
#define	tpcbtoso(tp)	((struct socket *)((tp)->tpp_socket))
#define	tpcbtoref(tp)	((struct tpi_ref *)((tp)->tp_ref))


extern struct tpi_refinfo 	tpi_refinfo;
extern struct tpi_ref		*tpi_ref;

/* Transport Interface */
uint32_t tpi_pcbnethash(void *, uint16_t);
void tpi_pcbinit(struct tpipcbtable *, int);
int tpi_pcballoc(struct socket *, void *, int);
int tpi_tselinuse(struct tpipcbtable *, struct tpipcb *, u_short, char *, struct sockaddr_iso *, int);
int tpi_set_npcb(struct tpipcb **, struct socket *, int);
int tpi_get_npcb(struct tpipcb *, struct socket *, int);
int tpi_pcbbind(void *, struct mbuf *, struct proc *);
int tpi_pcbconnect(void *, struct mbuf *, int, int);
void tpi_pcbdisconnect(void *, int, int);
void tpi_pcbdetach(void *, int, int);
void tpi_setusockaddr(struct tpipcb *, union tpi_sockaddr_union *, void *, uint16_t, int);
union tpi_sockaddr_union *tpi_getusockaddr(struct tpipcbtable *, void *, uint16_t, int);

struct tpipcb *tpi_pcblookup(struct tpipcbtable *, void *, uint16_t, int, int);
struct tpipcb *tpi_pcblookup_connect(struct tpipcbtable *, void *, u_int16_t, void *, u_int16_t, int, int);
struct tpipcb *tpi_pcblookup_bind(struct tpipcbtable *, void *, u_int16_t, int);
void tpi_pcbstate(struct tpipcb *, int, int, int);
int tpi_pcbisvalid(void *, void *, u_int16_t, void *, u_int16_t, int, int);
int tpi_pcbisvalid_sockaddr(union tpi_sockaddr_union *, void *, int);

/* Transport Local */
struct tpipcbhead *tpi_local_hash(struct tpipcbtable *, void *, uint16_t);
void tpi_local_insert(struct tpipcbtable *, void *, uint16_t);
struct tpi_local *tpi_local_lookup(struct tpipcbtable *, void *, uint16_t);
void tpi_local_remove(struct tpipcbtable *, void *, uint16_t);
int tpi_local_compare(struct tpi_local *, struct tpi_local *);
void tpi_local_set_lsockaddr(struct tpi_local *, void *, void *, uint16_t);
void *tpi_local_get_lsockaddr(struct tpi_local *);

/* Transport Foreign */
struct tpipcbhead *tpi_foreign_hash(struct tpipcbtable *, void *, uint16_t);
void tpi_foreign_insert(struct tpipcbtable *, void *, uint16_t);
struct tpi_foreign *tpi_foreign_lookup(struct tpipcbtable *, void *, uint16_t);
void tpi_foreign_remove(struct tpipcbtable *, void *, uint16_t);
int tpi_foreign_compare(struct tpi_foreign *, struct tpi_foreign *);
void tpi_foreign_set_fsockaddr(struct tpi_foreign *, void *, void *, uint16_t);
void *tpi_foreign_get_fsockaddr(struct tpi_foreign *);

#endif /* _NETTPI_TPI_PCB_H_ */
