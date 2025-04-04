/*	$NetBSD: pk_var.h,v 1.17 2003/08/07 16:33:05 agc Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by the
 * Laboratory for Computation Vision and the Computer Science Department
 * of the University of British Columbia and the Computer Science
 * Department (IV) of the University of Erlangen-Nuremberg, Germany.
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
 *	@(#)pk_var.h	8.1 (Berkeley) 6/10/93
 */

/*
 * Copyright (c) 1985 Computing Centre, University of British Columbia.
 * Copyright (c) 1990, 1991, 1992 Computer Science Department IV,
 * 		University of Erlangen-Nuremberg, Germany.
 *
 * This code is derived from software contributed to Berkeley by the
 * Laboratory for Computation Vision and the Computer Science Department
 * of the University of British Columbia and the Computer Science
 * Department (IV) of the University of Erlangen-Nuremberg, Germany.
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
 *	@(#)pk_var.h	8.1 (Berkeley) 6/10/93
 */

#ifndef _NETCCITT_PK_VAR_H_
#define _NETCCITT_PK_VAR_H_

/*
 *
 *  X.25 Logical Channel Descriptor
 *
 */

#include <sys/queue.h>

TAILQ_HEAD(pklcd_q, pklcd);
struct pklcd {
	TAILQ_ENTRY(pklcd) lcd_q;						/* debugging chain */
	int		(*lcd_upper)(struct mbuf *, void *);	/* switch to socket vs datagram vs ...*/
	caddr_t	lcd_upnext;				/* reference for lcd_upper() */
	void 	(*lcd_send)(struct pklcd *);			/* if X.25 front end, direct connect */
	caddr_t lcd_downnext;			/* reference for lcd_send() */
	short   lcd_lcn;				/* Logical channel number */
	short   lcd_state;				/* Logical Channel state */
	short   lcd_timer;				/* Various timer values */
	short   lcd_dg_timer;			/* to reclaim idle datagram circuits */
	bool	lcd_intrconf_pending;	/* Interrupt confirmation pending */
	u_char	lcd_intrdata;			/* Octet of incoming intr data */
	char	lcd_retry;				/* Timer retry count */
	char	lcd_rsn;				/* Seq no of last received packet */
	char	lcd_ssn;				/* Seq no of next packet to send */
	char	lcd_output_window;		/* Output flow control window */
	char	lcd_input_window;		/* Input flow control window */
	char	lcd_last_transmitted_pr;/* Last Pr value transmitted */
	bool	lcd_rnr_condition;		/* Remote in busy condition */
	bool	lcd_window_condition;	/* Output window size exceeded */
	bool	lcd_reset_condition;	/* True, if waiting reset confirm */
	bool	lcd_rxrnr_condition;	/* True, if we have sent rnr */
	char	lcd_packetsize;			/* Maximum packet size */
	char	lcd_windowsize;			/* Window size - both directions */
	u_char	lcd_closed_user_group;	/* Closed user group specification */
	char	lcd_flags;				/* copy of sockaddr_x25 op_flags */
	struct	mbuf *lcd_facilities;	/* user supplied facilities for cr */
	struct	mbuf *lcd_template;		/* Address of response packet */
	struct	socket *lcd_so;			/* Socket addr for connection */
	struct	sockaddr_x25 *lcd_craddr;/* Calling address pointer */
	struct	sockaddr_x25 *lcd_ceaddr;/* Called address pointer */
	time_t	lcd_stime;				/* time circuit established */
	long    lcd_txcnt;				/* Data packet transmit count */
	long    lcd_rxcnt;				/* Data packet receive count */
	short   lcd_intrcnt;			/* Interrupt packet transmit count */
	TAILQ_ENTRY(pklcd) lcd_listen;	/* Next lcd on listen queue */
	struct	pkcb *lcd_pkp;			/* Network this lcd is attached to */
	struct	mbuf *lcd_cps;			/* Complete Packet Sequence reassembly*/
	long	lcd_cpsmax;				/* Max length for CPS */
	struct	sockaddr_x25 lcd_faddr;	/* Remote Address (Calling) */
	struct	sockaddr_x25 lcd_laddr;	/* Local Address (Called) */
	struct	sockbuf lcd_sb;			/* alternate for datagram service */
};

/*
 * Per network information, allocated dynamically
 * when a new network is configured.
 */
TAILQ_HEAD(pkcb_q, pkcb);
struct pkcb {
	TAILQ_ENTRY(pkcb) pk_q;
	short	pk_state;				/* packet level status */
	u_short	pk_maxlcn;				/* local copy of xc_maxlcn */
	int		(*pk_lloutput)(struct mbuf *, ...);		/* link level output procedure */
	void 	*(*pk_llctlinput)(int, struct sockaddr *, void *);	/* link level ctloutput procedure */
	caddr_t pk_llnext;				/* handle for next level down */
	struct x25config *pk_xcp;		/* network specific configuration */
	struct x25_ifaddr *pk_ia;		/* backpointer to ifaddr */
	struct pklcd **pk_chan;			/* actual size == xc_maxlcn+1 */
	short 	pk_dxerole;				/* DXE role of PLE over LLC2 */
	short 	pk_restartcolls;		/* counting RESTART collisions til resolved */
	struct rtentry *pk_rt;			/* back pointer to route */
	struct rtentry *pk_llrt;       	/* pointer to reverse mapping */
	u_short pk_refcount;  			/* ref count */
};

/*
 *	Interface address, x25 version. Exactly one of these structures is
 *	allocated for each interface with an x25 address.
 *
 *	The ifaddr structure conatins the protocol-independent part
 *	of the structure, and is assumed to be first.
 */
struct x25_ifaddr {
	struct ifaddr 	ia_ifa;			/* protocol-independent info */
#define ia_ifp		ia_ifa.ifa_ifp
#define	ia_flags 	ia_ifa.ifa_flags
	struct x25config ia_xc;			/* network specific configuration */
	struct pkcb 	*ia_pkcb;
#define ia_maxlcn 	ia_xc.xc_maxlcn
	int	(*ia_start)(struct pklcd *);/* connect, confirm method */
	struct sockaddr_x25 ia_dstaddr; /* reserve space for route dst */
};

struct llinfo_x25 {
	LIST_ENTRY(llinfo_x25) lx_list;
	struct rtentry *lx_rt;			/* back pointer to route */
	struct pklcd *lx_lcd;			/* local connection block */
	struct x25_ifaddr *lx_ia;		/* may not be same as rt_ifa */
	int	lx_state;					/* can't trust lcd->lcd_state */
	int	lx_flags;
	int	lx_timer;					/* for idle timeout */
	int	lx_family;					/* for dispatch */
};

/* States for lx_state */
#define LXS_NEWBORN			0
#define LXS_RESOLVING		1
#define LXS_FREE			2
#define LXS_CONNECTING		3
#define LXS_CONNECTED		4
#define LXS_DISCONNECTING 	5
#define LXS_LISTENING 		6

/* flags */
#define LXF_VALID	0x1		/* Circuit is live, etc. */
#define LXF_RTHELD	0x2		/* this lcb references rtentry */
#define LXF_LISTEN	0x4		/* accepting incoming calls */

/*
 * Definitions for accessing bitfields/bitslices inside X.25 structs
 */

struct x25bitslice {
	unsigned int bs_mask;
	unsigned int bs_shift;
};

#define	calling_addrlen	0
#define	called_addrlen	1
#define	q_bit	        2
#define	d_bit           3
#define	fmt_identifier	4
#define	lc_group_number	1
#define	p_r             5
#define	m_bit           6
#define	p_s             7
#define	zilch           8

#define	X25GBITS(Arg, Index)		(((Arg) & x25_bitslice[(Index)].bs_mask) >> x25_bitslice[(Index)].bs_shift)
#define	X25SBITS(Arg, Index, Val)	(Arg) |= (((Val) << x25_bitslice[(Index)].bs_shift) & x25_bitslice[(Index)].bs_mask)
#define	X25CSBITS(Arg, Index, Val)	(Arg) = (((Val) << x25_bitslice[(Index)].bs_shift) & x25_bitslice[(Index)].bs_mask)

#define ISOFIFTTYPE(i,t) ((i)->if_type == (t))
#define ISISO8802(i) 						\
	((ISOFIFTTYPE(i, IFT_ETHER) || 			\
			ISOFIFTTYPE(i, IFT_ISO88023) || \
			ISOFIFTTYPE(i, IFT_ISO88024) || \
			ISOFIFTTYPE(i, IFT_ISO88025) || \
			ISOFIFTTYPE(i, IFT_ISO88026) || \
			ISOFIFTTYPE(i, IFT_P10) || 		\
			ISOFIFTTYPE(i, IFT_P80) || 		\
			ISOFIFTTYPE(i, IFT_FDDI)))

/*
 * miscellenous debugging info
 */
struct mbuf_cache {
	int	mbc_size;
	int	mbc_num;
	int	mbc_oldsize;
	struct mbuf **mbc_cache;
};

extern const struct x25bitslice x25_bitslice[];
extern struct pklcd_q	pk_lcdhead;
extern struct pklcd_q	pk_listenhead;
extern struct pkcb_q 	pk_cbhead;

extern char	*pk_name[], *pk_state[];
extern int	pk_t20, pk_t21, pk_t22, pk_t23;

#endif /* _NETCCITT_PK_VAR_H_ */
