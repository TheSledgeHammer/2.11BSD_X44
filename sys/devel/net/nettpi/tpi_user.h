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
 *	@(#)tp_user.h	8.1 (Berkeley) 6/10/93
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
 * $Header: tp_user.h,v 5.2 88/11/04 15:44:44 nhall Exp $
 * $Source: /usr/argo/sys/netiso/RCS/tp_user.h,v $
 *
 * These are the values a real-live user ;-) needs.
 */

#ifndef _NETTPI_TPI_USER_H_
#define _NETTPI_TPI_USER_H_

#include  <sys/types.h>

struct tpi_conn_param {
	/* PER CONNECTION parameters */
	short	p_Nretrans;
	short	p_dr_ticks;

	short	p_cc_ticks;
	short	p_dt_ticks;

	short	p_x_ticks;
	short	p_cr_ticks;

	short	p_keepalive_ticks;
	short	p_sendack_ticks;

	short	p_ref_ticks;
	short	p_inact_ticks;

	short	p_ptpdusize;			/* preferred tpdusize/128 */
	short	p_winsize;

	u_char	p_tpdusize; 			/* log 2 of size */

	u_char	p_ack_strat;			/* see comments in tp_pcb.h */
	u_char	p_rx_strat;				/* see comments in tp_pcb.h */
	u_char	p_class;	 			/* class bitmask */
	u_char	p_xtd_format;
	u_char	p_xpd_service;
	u_char	p_use_checksum;
	u_char	p_use_nxpd; 			/* netwk expedited data: not implemented */
	u_char	p_use_rcc;				/* receipt confirmation: not implemented */
	u_char	p_use_efc;				/* explicit flow control: not implemented */
	u_char	p_no_disc_indications;	/* don't deliver indic on disc */
	u_char	p_dont_change_params;	/* use these params as they are */
	u_char	p_netservice;
	u_char	p_version;				/* only here for checking */
};

/* read only flags */
#define TPFLAG_NLQOS_PDN		(u_char)0x01
#define TPFLAG_PEER_ON_SAMENET	(u_char)0x02
#define TPFLAG_GENERAL_ADDR		(u_char)0x04 /* bound to wildcard addr */

struct tpi_conn_param tpi_conn_param[] = {
		{
				.p_Nretrans = 0,
				.p_dr_ticks = 0,
				.p_cc_ticks = 0,
				.p_dt_ticks = 0,
				.p_x_ticks = 0,
				.p_cr_ticks = 0,
				.p_keepalive_ticks = 0,
				.p_sendack_ticks = 0,
				.p_ref_ticks = 0,
				.p_inact_ticks = 0,
				.p_ptpdusize = 0,
				.p_winsize = 0,
				.p_tpdusize = 0,
				.p_ack_strat = 0,
				.p_rx_strat = 0,
				.p_class = 0,
				.p_xtd_format = 0,
				.p_xpd_service = 0,
				.p_use_checksum = 0,
				.p_use_nxpd = 0,
				.p_use_rcc = 0,
				.p_use_efc = 0,
				.p_no_disc_indications = 0,
				.p_dont_change_params = 0,
				.p_netservice = 0,
				.p_version = 0,
		},
};

#endif /* _NETTPI_TPI_USER_H_ */
