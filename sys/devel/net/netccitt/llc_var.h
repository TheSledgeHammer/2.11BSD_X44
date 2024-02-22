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

/*
 * Other necessary definitions
 */

#define LLC_MAX_SEQUENCE    128
#define LLC_MAX_WINDOW	    127
#define LLC_WINDOW_SIZE	    7

/*
 * LLC control block
 */

struct llccb_q;
TAILQ_HEAD(llccb_q, llc_linkcb);
struct llc_linkcb {
	TAILQ_ENTRY(llc_linkcb) llcl_q;		/* admin chain */
	struct llc_sapinfo 	*llcl_sapinfo;	/* SAP information */
	struct sockaddr_dl 	llcl_addr;		/* link snpa address */
	struct rtentry 		*llcl_nlrt;		/* layer 3 -> LLC */
	struct rtentry		*llcl_llrt;		/* LLC -> layer 3 */
	struct ifnet        *llcl_if;       /* our interface */
	caddr_t				llcl_nlnext;	/* cb for network layer */
	struct mbuf   	 	*llcl_wqhead;	/* Write queue head */
	struct mbuf    		*llcl_wqtail;	/* Write queue tail */
	struct mbuf    		**llcl_output_buffers;
	/*
	 * The following components deal --- in one way or the other ---
	 * with the LLC2 window. Indicated by either [L] or [W] is the
	 * domain of the specific component:
	 *
	 *        [L]    The domain is 0--LLC_MAX_WINDOW
     *    	  [W]    The domain is 0--llcl_window
	 */
	short           	llcl_vr;        /* next to receive [L] */
	short           	llcl_vs;        /* next to send [L] */
	short               llcl_freeslot;  /* next free slot [W] */
	short               llcl_projvs;    /* V(S) associated with freeslot */
	short               llcl_slotsfree; /* free slots [W] */
	short           	llcl_window;    /* window size */
	/*
	 * In llcl_frmrinfo we jot down the last frmr info field, which we
	 * need to do as we need to be able to resend it in the ERROR state.
	 */
	struct frmrinfo     llcl_frmrinfo;  /* last FRMR info field */
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

struct sockaddr_dl_header {
	struct sockaddr_dl 	sdlhdr_dst;
	struct sockaddr_dl 	sdlhdr_src;
	long 				sdlhdr_len;
};

#define LLC_GETHDR(f, m) { 										\
	struct mbuf *_m = (struct mbuf *) (m); 						\
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

#define LLC_INC(i) 		((i) = ((i)+1) % LLC_MAX_SEQUENCE)

#define LLC_FRMR_W     (1<<0)
#define LLC_FRMR_X     (1<<1)
#define LLC_FRMR_Y     (1<<2)
#define LLC_FRMR_Z     (1<<3)
#define LLC_FRMR_V     (1<<4)

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

struct ifqueue llcintrq;
extern struct llccb_q llccb_q;

#endif /* _NETCCITT_LLC_VAR_H_ */
