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

#ifndef _NETCCITT_LLC_DEFS_H_
#define _NETCCITT_LLC_DEFS_H_

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

#define	LLCGBITS(Arg, Index)											\
	(((Arg) & llc_bitslice[(Index)].bs_mask) >> llc_bitslice[(Index)].bs_shift)

#define	LLCSBITS(Arg, Index, Val)										\
	(Arg) |= (((Val) << llc_bitslice[(Index)].bs_shift) & llc_bitslice[(Index)].bs_mask)

#define	LLCCSBITS(Arg, Index, Val)										\
	(Arg) = (((Val) << llc_bitslice[(Index)].bs_shift) & llc_bitslice[(Index)].bs_mask)

#define LLC_ENQUEUE(l, m) 												\
	if ((l)->llcl_wqhead == NULL) { 									\
		(l)->llcl_wqhead = (m); 										\
		(l)->llcl_wqtail = (m); 										\
	} else { 															\
		(l)->llcl_wqtail->m_nextpkt = (m); 								\
		(l)->llcl_wqtail = (m); 										\
	}

#define LLC_DEQUEUE(l, m) 												\
		if ((l)->llcl_wqhead == NULL) { 								\
			(m) = NULL; 												\
		} else { 														\
			(m) = (l)->llcl_wqhead; 									\
			(l)->llcl_wqhead = (l)->llcl_wqhead->m_nextpkt; 			\
		} 																\

#define LLC_SETFRAME(l, m) { 											\
			if ((l)->llcl_slotsfree > 0) { 								\
				(l)->llcl_slotsfree--; 									\
				(l)->llcl_output_buffers[(l)->llcl_freeslot] = (m); 	\
				(l)->llcl_freeslot = ((l)->llcl_freeslot+1) % (l)->llcl_window; \
				LLC_INC((l)->llcl_projvs); 								\
			}															\
		}

#define LLC_GETHDR(f, m) { 												\
	struct mbuf *_m = (struct mbuf *)(m); 								\
	if (_m) { 															\
		M_PREPEND(_m, LLC_ISFRAMELEN, M_DONTWAIT); 						\
		bzero(mtod(_m, caddr_t), LLC_ISFRAMELEN); 						\
	} else { 															\
		MGETHDR (_m, M_DONTWAIT, MT_HEADER); 							\
		if (_m != NULL) { 												\
			_m->m_pkthdr.len = _m->m_len = LLC_UFRAMELEN; 				\
			_m->m_next = _m->m_act = NULL; 								\
			bzero(mtod(_m, caddr_t), LLC_UFRAMELEN); 					\
		} else { 														\
			return; 													\
		}																\
		(m) = _m; 														\
		(f) = mtod(m, struct llc *); 									\
	}																	\
}

#define LLC_NEWSTATE(l, LLCstate) 	((l)->llcl_statehandler = llc_state_##LLCstate)
#define LLC_STATEEQ(l, LLCstate) 	((l)->llcl_statehandler == llc_state_##LLCstate ? 1 : 0)

#define LLC_SETFLAG(l, LLCflag, v) 	((l)->llcl_##LLCflag##_flag = (v))
#define LLC_GETFLAG(l, LLCflag) 	((l)->llcl_##LLCflag##_flag)

#define LLC_ACK_SHIFT      		0
#define LLC_P_SHIFT        		1
#define LLC_BUSY_SHIFT     		2
#define LLC_REJ_SHIFT      		3
#define LLC_AGE_SHIFT      		4
#define LLC_DACTION_SHIFT  		5

#define LLC_TIMER_NOTRUNNING    0
#define LLC_TIMER_RUNNING       1
#define LLC_TIMER_EXPIRED       2

#define LLC_STARTTIMER(l, LLCtimer) { 									\
	(l)->llcl_timers[LLC_##LLCtimer##_SHIFT] = llc_##LLCtimer##_timer; 	\
	(l)->llcl_timerflags |= (1<<LLC_##LLCtimer##_SHIFT); 				\
}

#define LLC_STOPTIMER(l, LLCtimer) { 									\
	(l)->llcl_timers[LLC_##LLCtimer##_SHIFT] = 0; 						\
	(l)->llcl_timerflags &= ~(1<<LLC_##LLCtimer##_SHIFT); 				\
}

#define LLC_AGETIMER(l, LLCtimer) 										\
		if ((l)->llcl_timers[LLC_##LLCtimer##_SHIFT] > 0) { 			\
			(l)->llcl_timers[LLC_##LLCtimer##_SHIFT]--; 				\
		}

#define LLC_TIMERXPIRED(l, LLCtimer) 									\
		(((l)->llcl_timerflags & (1<<LLC_##LLCtimer##_SHIFT)) ? 		\
				(((l)->llcl_timers[LLC_##LLCtimer##_SHIFT] == 0 ) ? 	\
						LLC_TIMER_EXPIRED : LLC_TIMER_RUNNING) : LLC_TIMER_NOTRUNNING)

#define FOR_ALL_LLC_TIMERS(t) 											\
	for ((t) = LLC_ACK_SHIFT; (t) < LLC_AGE_SHIFT; (t)++)

#define LLC_SETFLAG(l, LLCflag, v) 	(l)->llcl_##LLCflag##_flag = (v)
#define LLC_GETFLAG(l, LLCflag) 	(l)->llcl_##LLCflag##_flag

#define LLC_RESETCOUNTER(l) { 											\
		(l)->llcl_vs = (l)->llcl_vr = (l)->llcl_retry = 0; 				\
		llc_resetwindow((l)); 											\
}

/*
 * LLC2 macro definitions
 */

#define LLC_START_ACK_TIMER(l) 	LLC_STARTTIMER((l), ACK)
#define LLC_STOP_ACK_TIMER(l) 	LLC_STOPTIMER((l), ACK)
#define LLC_START_REJ_TIMER(l) 	LLC_STARTTIMER((l), REJ)
#define LLC_STOP_REJ_TIMER(l) 	LLC_STOPTIMER((l), REJ)

#define LLC_START_P_TIMER(l) { 											\
	LLC_STARTTIMER((l), P); 											\
	if (LLC_GETFLAG((l), P) == 0) { 									\
		(l)->llcl_retry = 0; 											\
		LLC_SETFLAG((l), P, 1); 										\
	}																	\
}

#define LLC_STOP_P_TIMER(l) { 											\
	LLC_STOPTIMER((l), P); 												\
	LLC_SETFLAG((l), P, 0); 											\
}

#define LLC_STOP_ALL_TIMERS(l) { 										\
	LLC_STOPTIMER((l), ACK); 											\
	LLC_STOPTIMER((l), REJ); 											\
	LLC_STOPTIMER((l), BUSY); 											\
	LLC_STOPTIMER((l), P); 												\
}

#define LLC_INC(i) 														\
		((i) = ((i)+1) % LLC_MAX_SEQUENCE)

#define LLC_NR_VALID(l, nr)    											\
	((l)->llcl_vs < (l)->llcl_nr_received ? 							\
			(((nr) >= (l)->llcl_nr_received) || 						\
			((nr) <= (l)->llcl_vs) ? 1 : 0) : 							\
			(((nr) <= (l)->llcl_vs) && 									\
			((nr) >= (l)->llcl_nr_received) ? 1 : 0))

#define LLC_UPDATE_P_FLAG(l, cr, pf) { 									\
	if ((cr) == LLC_RSP && (pf) == 1) { 								\
		LLC_SETFLAG((l), P, 0); 										\
		LLC_STOPTIMER((l), P); 											\
	} 																	\
}

#define LLC_UPDATE_NR_RECEIVED(l, nr) { 								\
	while ((l)->llcl_nr_received != (nr)) { 							\
		struct mbuf *_m; 												\
		register short seq; 											\
		if (_m = (l)->llcl_output_buffers[seq = llc_seq2slot((l), (l)->llcl_nr_received)]) { \
			m_freem(_m); 												\
			(l)->llcl_output_buffers[seq] = NULL; 						\
			LLC_INC((l)->llcl_nr_received); 							\
			(l)->llcl_slotsfree++; 										\
		} 																\
		(l)->llcl_retry = 0; 											\
		if ((l)->llcl_slotsfree < (l)->llcl_window) { 					\
			LLC_START_ACK_TIMER(l); 									\
		} else { 														\
			LLC_STOP_ACK_TIMER(l); 										\
			LLC_STARTTIMER((l), DACTION); 								\
		} 																\
	} 																	\
}

#define LLC_SET_REMOTE_BUSY(l,a) { 										\
	if (LLC_GETFLAG((l), REMOTE_BUSY) == 0) { 							\
		LLC_SETFLAG((l), REMOTE_BUSY, 1); 								\
		LLC_STARTTIMER((l), BUSY); 										\
		(a) = LLC_REMOTE_BUSY; 											\
	} else { 															\
		(a) = 0; 														\
	} 																	\
}

#define LLC_CLEAR_REMOTE_BUSY(l,a) { 									\
	if (LLC_GETFLAG((l), REMOTE_BUSY) == 1) { 							\
		LLC_SETFLAG((l), REMOTE_BUSY, 1); 								\
		LLC_STOPTIMER((l), BUSY); 										\
		if (LLC_STATEEQ((l), normal) || 								\
				LLC_STATEEQ((l), reject) || 							\
				LLC_STATEEQ((l), busy)) { 								\
			llc_resend((l), LLC_CMD, 0); 								\
		} 																\
		(a) = LLC_REMOTE_NOT_BUSY; 										\
	} else { 															\
		(a) = 0; 														\
	} 																	\
}

#define LLC_DACKCMD      0x1
#define LLC_DACKCMDPOLL  0x2
#define LLC_DACKRSP      0x3
#define LLC_DACKRSPFINAL 0x4

#define LLC_SENDACKNOWLEDGE(l, cmd, pf) { 								\
	if ((cmd) == LLC_CMD) { 											\
		LLC_SETFLAG((l), DACTION, ((pf) == 0 ? LLC_DACKCMD : LLC_DACKCMDPOLL)); 	\
	} else { 															\
		LLC_SETFLAG((l), DACTION, ((pf) == 0 ? LLC_DACKRSP : LLC_DACKRSPFINAL)); 	\
	} 																	\
}

#define LLC_FRMR_W     	(1<<0)
#define LLC_FRMR_X     	(1<<1)
#define LLC_FRMR_Y     	(1<<2)
#define LLC_FRMR_Z     	(1<<3)
#define LLC_FRMR_V     	(1<<4)

#define LLC_SETFRMR(l, f, cr, c) { 										\
	if ((f)->llc_control & 0x3) { 										\
		(l)->llcl_frmr_pdu0 = (f)->llc_control; 						\
		(l)->llcl_frmr_pdu1 = 0; 										\
	} else { 															\
		(l)->llcl_frmr_pdu0 = (f)->llc_control; 						\
		(l)->llcl_frmr_pdu1 = (f)->llc_control_ext; 					\
	} 																	\
	LLCCSBITS((l)->llcl_frmr_control, f_vs, (l)->llcl_vs); 				\
	LLCCSBITS((l)->llcl_frmr_control_ext, f_cr, (cr)); 					\
	LLCSBITS((l)->llcl_frmr_control_ext, f_vr, (l)->llcl_vr); 			\
	LLCCSBITS((l)->llcl_frmr_cause, f_wxyzv, (c)); 						\
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

#define LLC_N2_VALUE	  		15             /* up to 15 retries */
#define LLC_ACK_TIMER     		10             /*  5 secs */
#define LLC_P_TIMER       	 	4              /*  2 secs */
#define LLC_BUSY_TIMER    		12             /*  6 secs */
#define LLC_REJ_TIMER     		12             /*  6 secs */
#define LLC_AGE_TIMER     		40             /* 20 secs */
#define LLC_DACTION_TIMER  		2              /*  1 secs */

#endif /* _NETCCITT_LLC_DEFS_H_ */
