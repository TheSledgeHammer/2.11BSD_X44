/*
 * Copyright (C) Dirk Husemann, Computer Science Department IV,
 * 		 University of Erlangen-Nuremberg, Germany, 1990, 1991, 1992
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Dirk Husemann and the Computer Science Department (IV) of
 * the University of Erlangen-Nuremberg, Germany.
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
 *	@(#)llc_var.h	8.2 (Berkeley) 2/9/95
 */

#ifndef _NETCCITT_LLC_VAR_H_
#define _NETCCITT_LLC_VAR_H_


#define	SAPINFO_LINK	0

struct llc_sapinfo {
	union {
		struct {
			/* MAC,DLSAP -> CONS */
			struct llc_linkcb 	*ne_link;
			struct rtentry 		*ne_rt;
		} ne;
		struct {
			u_short 			si_class;
			u_short 			si_window;
			u_short 			si_trace;
			void 				(*si_input)(struct mbuf *);
			void 				*(*si_ctlinput)(int, struct sockaddr *, void *);
		} si;
	} un;
#define np_link                 un.ne.ne_link
#define np_rt                   un.ne.ne_rt
#define si_class                un.si.si_class
#define si_window               un.si.si_window
#define si_trace                un.si.si_trace
#define si_input                un.si.si_input
#define si_ctlinput             un.si.si_ctlinput
};

#define NPDL_SAPNETMASK 		0x7e

static __inline void
llc_sapinfo_input(struct llc_linkcb *linkp, struct mbuf *m)
{
	(*linkp->llcl_sapinfo->si_input)(m);
}

static __inline void *
llc_sapinfo_ctlinput(struct llc_linkcb *linkp, int level, struct sockaddr *sa, void *addr)
{
	return ((*linkp->llcl_sapinfo->si_ctlinput)(level, sa, addr));
}

#define LLC_CLASS_I			0x1
#define	LLC_CLASS_II		0x3
#define LLC_CLASS_III		0x4		/* Future */
#define LLC_CLASS_IV		0x7		/* Future */

/*
 * Definitions for accessing bitfields/bitslices inside
 * LLC2 headers
 */
struct bitslice {
	unsigned int bs_mask;
	unsigned int bs_shift;
};

/* bitslice index */
#define	i_z	        	0
#define	i_ns	        1
#define	i_pf	        0
#define	i_nr	        1
#define	s_oz            2
#define	s_selector		3
#define	s_pf            0
#define	s_nr            1
#define	u_bb            2
#define	u_select_other	3
#define	u_pf            4
#define	u_select		5

#define	f_vs            1
#define	f_cr            0
#define	f_vr            1
#define	f_wxyzv         6

#define	LLCGBITS(Arg, Index)		(((Arg) & llc_bitslice[(Index)].bs_mask) >> llc_bitslice[(Index)].bs_shift)
#define	LLCSBITS(Arg, Index, Val)	(Arg) |= (((Val) << llc_bitslice[(Index)].bs_shift) & llc_bitslice[(Index)].bs_mask)
#define	LLCCSBITS(Arg, Index, Val)	(Arg) = (((Val) << llc_bitslice[(Index)].bs_shift) & llc_bitslice[(Index)].bs_mask)

extern struct bitslice llc_bitslice[];

#define LLC_CMD         0
#define LLC_RSP         1
#define LLC_MAXCMDRSP   2
#define LLC_CMDRSP(val)	((val) * LLC_MAXCMDRSP)

/*
 * LLC events --- These events may either be frames received from the
 *                remote LLC DSAP, request from the network layer user,
 *                timer events from llc_timer(), or diagnostic events from
 *                llc_input().
 */

/* LLC frame types */
#define LLCFT_INFO                      LLC_CMDRSP(0)
#define LLCFT_RR                        LLC_CMDRSP(1)
#define LLCFT_RNR                       LLC_CMDRSP(2)
#define LLCFT_REJ                      	LLC_CMDRSP(3)
#define LLCFT_DM                        LLC_CMDRSP(4)
#define LLCFT_SABME                     LLC_CMDRSP(5)
#define LLCFT_DISC                      LLC_CMDRSP(6)
#define LLCFT_UA                        LLC_CMDRSP(7)
#define LLCFT_FRMR                      LLC_CMDRSP(8)
#define LLCFT_UI                        LLC_CMDRSP(9)
#define LLCFT_XID                       LLC_CMDRSP(10)
#define LLCFT_TEST                      LLC_CMDRSP(11)

/* LLC2 timer events */
#define LLC_ACK_TIMER_EXPIRED          	LLC_CMDRSP(12)
#define LLC_P_TIMER_EXPIRED             LLC_CMDRSP(13)
#define LLC_REJ_TIMER_EXPIRED          	LLC_CMDRSP(14)
#define LLC_BUSY_TIMER_EXPIRED          LLC_CMDRSP(15)

/* LLC2 diagnostic events */
#define LLC_INVALID_NR                  LLC_CMDRSP(16)
#define LLC_INVALID_NS                  LLC_CMDRSP(17)
#define LLC_BAD_PDU                     LLC_CMDRSP(18)
#define LLC_LOCAL_BUSY_DETECTED         LLC_CMDRSP(19)
#define LLC_LOCAL_BUSY_CLEARED          LLC_CMDRSP(20)

/* Network layer user requests */
#define NL_CONNECT_REQUEST              LLC_CMDRSP(21)
#define NL_CONNECT_RESPONSE             LLC_CMDRSP(22)
#define NL_RESET_REQUEST                LLC_CMDRSP(23)
#define NL_RESET_RESPONSE              	LLC_CMDRSP(24)
#define NL_DISCONNECT_REQUEST           LLC_CMDRSP(25)
#define NL_DATA_REQUEST                 LLC_CMDRSP(26)
#define NL_INITIATE_PF_CYCLE            LLC_CMDRSP(27)
#define NL_LOCAL_BUSY_DETECTED          LLC_CMDRSP(28)

#define LLCFT_NONE                      255

/* return message from state handlers */
#define LLC_CONNECT_INDICATION      	1
#define LLC_CONNECT_CONFIRM         	2
#define LLC_DISCONNECT_INDICATION   	3
#define LLC_RESET_CONFIRM           	4
#define LLC_RESET_INDICATION_REMOTE 	5
#define LLC_RESET_INDICATION_LOCAL  	6
#define LLC_FRMR_RECEIVED           	7
#define LLC_FRMR_SENT               	8
#define LLC_DATA_INDICATION         	9
#define LLC_REMOTE_NOT_BUSY         	10
#define LLC_REMOTE_BUSY             	11

/* Internal return code */
#define LLC_PASSITON                	255

#define INFORMATION_CONTROL				0x00
#define SUPERVISORY_CONTROL				0x02
#define UNUMBERED_CONTROL 				0x03

/*
 * Other necessary definitions
 */
#define LLC_MAX_SEQUENCE    			128
#define LLC_MAX_WINDOW	    			127
#define LLC_WINDOW_SIZE	    			7

#define NLHDRSIZEGUESS      			3

/*
 * LLC control block
 */

struct llccb_q;
TAILQ_HEAD(llccb_q, llc_linkcb);
struct llc_linkcb {
	TAILQ_ENTRY(llc_linkcb) llcl_q;			/* admin chain */
	struct llc_sapinfo 	*llcl_sapinfo;		/* SAP information */
	struct sockaddr_dl 	llcl_addr;			/* link snpa address */
	struct rtentry 		*llcl_nlrt;			/* layer 3 -> LLC */
	struct rtentry		*llcl_llrt;			/* LLC -> layer 3 */
	struct ifnet        *llcl_if;       	/* our interface */
	caddr_t				llcl_nlnext;		/* cb for network layer */
	struct mbuf   	 	*llcl_wqhead;		/* Write queue head */
	struct mbuf    		*llcl_wqtail;		/* Write queue tail */
	struct mbuf    		**llcl_output_buffers;
	short               llcl_timers[6]; 	/* timer array */
	long                llcl_timerflags;	/* flags signalling running timers */
	int                 (*llcl_statehandler)(struct llc_linkcb *, struct llc *, int, int, int);

	int                 llcl_P_flag;		/* poll flag */
	int                 llcl_F_flag;		/* final flag */
	int                 llcl_S_flag; 		/* supervisory flag */
	int                 llcl_DATA_flag;
	int                 llcl_REMOTE_BUSY_flag;
	int                 llcl_DACTION_flag; /* delayed action */
	int                 llcl_retry;

	/*
	 * The following components deal --- in one way or the other ---
	 * with the LLC2 window. Indicated by either [L] or [W] is the
	 * domain of the specific component:
	 *
	 *        [L]    The domain is 0--LLC_MAX_WINDOW
     *    	  [W]    The domain is 0--llcl_window
	 */
	short           	llcl_vr;        	/* next to receive [L] */
	short           	llcl_vs;        	/* next to send [L] */
	short           	llcl_nr_received;   /* next frame to b ack'd [L] */
	short               llcl_freeslot;  	/* next free slot [W] */
	short               llcl_projvs;    	/* V(S) associated with freeslot */
	short               llcl_slotsfree; 	/* free slots [W] */
	short           	llcl_window;    	/* window size */
	/*
	 * In llcl_frmrinfo we jot down the last frmr info field, which we
	 * need to do as we need to be able to resend it in the ERROR state.
	 */
	struct frmrinfo     llcl_frmrinfo;  	/* last FRMR info field */
#define llcl_frmr_pdu0          llcl_frmrinfo.rej_pdu_0
#define llcl_frmr_pdu1          llcl_frmrinfo.rej_pdu_1
#define llcl_frmr_control       llcl_frmrinfo.frmr_control
#define llcl_frmr_control_ext   llcl_frmrinfo.frmr_control_ext
#define llcl_frmr_cause         llcl_frmrinfo.frmr_cause
};

#define LLC_ENQUEUE(l, m) 										\
	if ((l)->llcl_wqhead == NULL) { 							\
		(l)->llcl_wqhead = (m); 								\
		(l)->llcl_wqtail = (m); 								\
	} else { 													\
		(l)->llcl_wqtail->m_nextpkt = (m); 						\
		(l)->llcl_wqtail = (m); 								\
	}

#define LLC_DEQUEUE(l, m) 										\
		if ((l)->llcl_wqhead == NULL) { 						\
			(m) = NULL; 										\
		} else { 												\
			(m) = (l)->llcl_wqhead; 							\
			(l)->llcl_wqhead = (l)->llcl_wqhead->m_nextpkt; 	\
		} 														\

#define LLC_SETFRAME(l, m) { \
			if ((l)->llcl_slotsfree > 0) { 						\
				(l)->llcl_slotsfree--; 							\
				(l)->llcl_output_buffers[(l)->llcl_freeslot] = (m); \
				(l)->llcl_freeslot = ((l)->llcl_freeslot+1) % (l)->llcl_window; \
				LLC_INC((l)->llcl_projvs); 						\
			}													\
		}

#define LLC_GETHDR(f, m) { 										\
	struct mbuf *_m = (struct mbuf *)(m); 						\
	if (_m) { 													\
		M_PREPEND(_m, LLC_ISFRAMELEN, M_DONTWAIT); 				\
		bzero(mtod(_m, caddr_t), LLC_ISFRAMELEN); 				\
	} else { 													\
		MGETHDR (_m, M_DONTWAIT, MT_HEADER); 					\
		if (_m != NULL) { 										\
			_m->m_pkthdr.len = _m->m_len = LLC_UFRAMELEN; 		\
			_m->m_next = _m->m_act = NULL; 						\
			bzero(mtod(_m, caddr_t), LLC_UFRAMELEN); 			\
		} else { 												\
			return; 											\
		}														\
		(m) = _m; 												\
		(f) = mtod(m, struct llc *); 							\
	}															\
}

#define LLC_NEWSTATE(l, LLCstate) 	((l)->llcl_statehandler = llc_state_##LLCstate)
#define LLC_STATEEQ(l, LLCstate) 	((l)->llcl_statehandler == llc_state_##LLCstate ? 1 : 0)

#define LLC_SETFLAG(l, LLCflag, v) 	((l)->llcl_##LLCflag##_flag = (v))
#define LLC_GETFLAG(l, LLCflag) 	((l)->llcl_##LLCflag##_flag)

#define LLC_INC(i) 												\
		((i) = ((i)+1) % LLC_MAX_SEQUENCE)

#define LLC_NR_VALID(l, nr)    									\
	((l)->llcl_vs < (l)->llcl_nr_received ? 					\
			(((nr) >= (l)->llcl_nr_received) || 				\
			((nr) <= (l)->llcl_vs) ? 1 : 0) : 					\
			(((nr) <= (l)->llcl_vs) && 							\
			((nr) >= (l)->llcl_nr_received) ? 1 : 0))

#define LLC_DACKCMD      0x1
#define LLC_DACKCMDPOLL  0x2
#define LLC_DACKRSP      0x3
#define LLC_DACKRSPFINAL 0x4

#define LLC_FRMR_W     	(1<<0)
#define LLC_FRMR_X     	(1<<1)
#define LLC_FRMR_Y     	(1<<2)
#define LLC_FRMR_Z     	(1<<3)
#define LLC_FRMR_V     	(1<<4)

#define LLC_SETFRMR(l, f, cr, c) { 								\
	if ((f)->llc_control & 0x3) { 								\
		(l)->llcl_frmr_pdu0 = (f)->llc_control; 				\
		(l)->llcl_frmr_pdu1 = 0; 								\
	} else { 													\
		(l)->llcl_frmr_pdu0 = (f)->llc_control; 				\
		(l)->llcl_frmr_pdu1 = (f)->llc_control_ext; 			\
	} 															\
	LLCCSBITS((l)->llcl_frmr_control, f_vs, (l)->llcl_vs); 		\
	LLCCSBITS((l)->llcl_frmr_control_ext, f_cr, (cr)); 			\
	LLCSBITS((l)->llcl_frmr_control_ext, f_vr, (l)->llcl_vr); 	\
	LLCCSBITS((l)->llcl_frmr_cause, f_wxyzv, (c)); 				\
}

/*
 * LLC tracing levels:
 *     LLCTR_INTERESTING        interesting event, we might care to know about
 *                              it, but then again, we might not ...
 *     LLCTR_SHOULDKNOW         we probably should know about this event
 *     LLCTR_URGENT             something has gone utterly wrong ...
 */
#define LLCTR_INTERESTING       1
#define LLCTR_SHOULDKNOW        2
#define LLCTR_URGENT            3

#ifdef LLCDEBUG
#define LLC_TRACE(lp, l, msg) llc_trace((lp), (l), (msg))
#else /* LLCDEBUG */
#define LLC_TRACE(lp, l, msg) /* NOOP */
#endif /* LLCDEBUG */

struct ifqueue llcintrq;
extern struct llccb_q llccb_q;

struct llc_sapinfo 	*llc_setsapinfo(struct ifnet *ifp, sa_family_t af, u_char sap, struct dllconfig *llconf, u_char llclass);
struct llc_sapinfo 	*llc_getsapinfo(u_char sap, struct ifnet *ifp);
struct llc_linkcb 	*llc_newlink(struct sockaddr_dl *dst, struct ifnet *ifp, struct rtentry *nlrt, caddr_t nlnext, struct rtentry *llrt);
void				llc_dellink(struct llc_linkcb *linkp);

#endif /* _NETCCITT_LLC_VAR_H_ */
