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
 *	@(#)llc_subr.c	8.1 (Berkeley) 6/10/93
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/socketvar.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/queue.h>
#include <sys/types.h>

#include <net/if_dl.h>
#include <net/if_llc.h>

#include <netccitt/dll.h>
#include <netccitt/llc_var.h>

/*
 * Trace level
 */
int llc_tracelevel = LLCTR_URGENT;

#define LLC_COMMAND(frame_kind, cmdrsp) ((frame_kind) + (cmdrsp))

int
llc_state_adm(struct llc_linkcb *linkp, struct llc *frame, int frame_kind, int cmdrsp, int pollfinal)
{
	int action, command;

	action = 0;
	command = LLC_COMMAND(frame_kind, cmdrsp);
	switch (command) {
	case NL_CONNECT_REQUEST:
		llc_send(linkp, LLCFT_SABME, LLC_CMD, pollfinal);
		LLC_SETFLAG(linkp, P, pollfinal);
		LLC_SETFLAG(linkp, S, 0);
		linkp->llcl_retry = 0;
		LLC_NEWSTATE(linkp, setup);
		break;
	case LLCFT_SABME + LLC_CMD:
		/*
		 * ISO 8802-2, table 7-1, ADM state says to set
		 * the P flag, yet this will cause an SABME [P] to be
		 * answered with an UA only, not an UA [F], all
		 * other `disconnected' states set the F flag, so ...
		 */
		LLC_SETFLAG(linkp, F, pollfinal);
		LLC_NEWSTATE(linkp, conn);
		action = LLC_CONNECT_INDICATION;
		break;
	case LLCFT_DISC + LLC_CMD:
		llc_send(linkp, LLCFT_DM, LLC_RSP, pollfinal);
		break;
	default:
		if (cmdrsp == LLC_CMD && pollfinal == 1) {
			llc_send(linkp, LLCFT_DM, LLC_RSP, 1);
		}
		/* remain in ADM state */
	}

	return (action);
}

int
llc_state_conn(struct llc_linkcb *linkp, struct llc *frame, int frame_kind, int cmdrsp, int pollfinal)
{
	int action, command;

	action = 0;
	command = LLC_COMMAND(frame_kind, cmdrsp);
	switch (command) {
	case NL_CONNECT_RESPONSE:
		llc_send(linkp, LLCFT_UA, LLC_RSP, LLC_GETFLAG(linkp, F));
		LLC_RESETCOUNTER(linkp);
		LLC_SETFLAG(linkp, P, 0);
		LLC_SETFLAG(linkp, REMOTE_BUSY, 0);
		LLC_NEWSTATE(linkp, normal);
		break;
	case NL_DISCONNECT_REQUEST:
		llc_send(linkp, LLCFT_DM, LLC_RSP, LLC_GETFLAG(linkp, F));
		LLC_NEWSTATE(linkp, adm);
		break;
	case LLCFT_SABME + LLC_CMD:
		LLC_SETFLAG(linkp, F, pollfinal);
		break;
	case LLCFT_DM + LLC_RSP:
		LLC_NEWSTATE(linkp, adm);
		action = LLC_DISCONNECT_INDICATION;
		break;
	/* all other frames effect nothing here */
	}

	return (action);
}

int
llc_state_reset_wait(struct llc_linkcb *linkp, struct llc *frame, int frame_kind, int cmdrsp, int pollfinal)
{
	int action, command;

	action = 0;
	command = LLC_COMMAND(frame_kind, cmdrsp);
	switch (command) {
	case NL_RESET_REQUEST:
		if (LLC_GETFLAG(linkp, S) == 0) {
			llc_send(linkp, LLCFT_SABME, LLC_CMD, pollfinal);
			LLC_SETFLAG(linkp, P, pollfinal);
			LLC_START_ACK_TIMER(linkp);
			linkp->llcl_retry = 0;
			LLC_NEWSTATE(linkp, reset);
		} else {
			llc_send(linkp, LLCFT_UA, LLC_RSP,
				      LLC_GETFLAG(linkp, F));
			LLC_RESETCOUNTER(linkp);
			LLC_SETFLAG(linkp, P, 0);
			LLC_SETFLAG(linkp, REMOTE_BUSY, 0);
			LLC_NEWSTATE(linkp, normal);
			action = LLC_RESET_CONFIRM;
		}
		break;
	case NL_DISCONNECT_REQUEST:
		if (LLC_GETFLAG(linkp, S) == 0) {
			llc_send(linkp, LLCFT_DISC, LLC_CMD, pollfinal);
			LLC_SETFLAG(linkp, P, pollfinal);
			LLC_START_ACK_TIMER(linkp);
			linkp->llcl_retry = 0;
			LLC_NEWSTATE(linkp, d_conn);
		} else {
			llc_send(linkp, LLCFT_DM, LLC_RSP,
				      LLC_GETFLAG(linkp, F));
			LLC_NEWSTATE(linkp, adm);
		}
		break;
	case LLCFT_DM + LLC_RSP:
		LLC_NEWSTATE(linkp, adm);
		action = LLC_DISCONNECT_INDICATION;
		break;
	case LLCFT_SABME + LLC_CMD:
		LLC_SETFLAG(linkp, S, 1);
		LLC_SETFLAG(linkp, F, pollfinal);
		break;
	case LLCFT_DISC + LLC_CMD:
		llc_send(linkp, LLCFT_DM, LLC_RSP, pollfinal);
		LLC_NEWSTATE(linkp, adm);
		action = LLC_DISCONNECT_INDICATION;
		break;
	}

	return (action);
}

int
llc_state_reset_check(struct llc_linkcb *linkp, struct llc *frame, int frame_kind, int cmdrsp, int pollfinal)
{
	int action, command;

	action = 0;
	command = LLC_COMMAND(frame_kind, cmdrsp);
	switch (command) {
	case NL_RESET_RESPONSE:
		llc_send(linkp, LLCFT_UA, LLC_RSP, LLC_GETFLAG(linkp, F));
		LLC_RESETCOUNTER(linkp);
		LLC_SETFLAG(linkp, P, 0);
		LLC_SETFLAG(linkp, REMOTE_BUSY, 0);
		LLC_NEWSTATE(linkp, normal);
		break;
	case NL_DISCONNECT_REQUEST:
		llc_send(linkp, LLCFT_DM, LLC_RSP, LLC_GETFLAG(linkp, F));
		LLC_NEWSTATE(linkp, adm);
		break;
	case LLCFT_DM + LLC_RSP:
		action = LLC_DISCONNECT_INDICATION;
		break;
	case LLCFT_SABME + LLC_CMD:
		LLC_SETFLAG(linkp, F, pollfinal);
		break;
	case LLCFT_DISC + LLC_CMD:
		llc_send(linkp, LLCFT_DM, LLC_RSP, pollfinal);
		LLC_NEWSTATE(linkp, adm);
		action = LLC_DISCONNECT_INDICATION;
		break;
	}
	return (action);
}

int
llc_state_setup(struct llc_linkcb *linkp, struct llc *frame, int frame_kind, int cmdrsp, int pollfinal)
{
	int action, command;

	action = 0;
	command = LLC_COMMAND(frame_kind, cmdrsp);
	switch (command) {
	case LLCFT_SABME + LLC_CMD:
		LLC_RESETCOUNTER(linkp);
		llc_send(linkp, LLCFT_UA, LLC_RSP, pollfinal);
		LLC_SETFLAG(linkp, S, 1);
		break;
	case LLCFT_UA + LLC_RSP:
		if (LLC_GETFLAG(linkp, P) == pollfinal) {
			LLC_STOP_ACK_TIMER(linkp);
			LLC_RESETCOUNTER(linkp);
			LLC_UPDATE_P_FLAG(linkp, cmdrsp, pollfinal);
			LLC_SETFLAG(linkp, REMOTE_BUSY, 0);
			LLC_NEWSTATE(linkp, normal);
			action = LLC_CONNECT_CONFIRM;
		}
		break;
	case LLC_ACK_TIMER_EXPIRED:
		if (LLC_GETFLAG(linkp, S) == 1) {
			LLC_SETFLAG(linkp, P, 0);
			LLC_SETFLAG(linkp, REMOTE_BUSY, 0),
			LLC_NEWSTATE(linkp, normal);
			action = LLC_CONNECT_CONFIRM;
		} else if (linkp->llcl_retry < llc_n2) {
			llc_send(linkp, LLCFT_SABME, LLC_CMD, pollfinal);
			LLC_SETFLAG(linkp, P, pollfinal);
			LLC_START_ACK_TIMER(linkp);
			linkp->llcl_retry++;
		} else {
			LLC_NEWSTATE(linkp, adm);
			action = LLC_DISCONNECT_INDICATION;
		}
		break;
	case LLCFT_DISC + LLC_CMD:
		llc_send(linkp, LLCFT_DM, LLC_RSP, pollfinal);
		LLC_STOP_ACK_TIMER(linkp);
		LLC_NEWSTATE(linkp, adm);
		action = LLC_DISCONNECT_INDICATION;
		break;
	case LLCFT_DM + LLC_RSP:
		LLC_STOP_ACK_TIMER(linkp);
		LLC_NEWSTATE(linkp, adm);
		action = LLC_DISCONNECT_INDICATION;
		break;
	}
	return (action);
}

int
llc_state_reset(struct llc_linkcb *linkp, struct llc *frame, int frame_kind, int cmdrsp, int pollfinal)
{
	int action, command;

	action = 0;
	command = LLC_COMMAND(frame_kind, cmdrsp);
	switch (command) {
	case LLCFT_SABME + LLC_CMD:
		LLC_RESETCOUNTER(linkp);
		LLC_SETFLAG(linkp, S, 1);
		llc_send(linkp, LLCFT_UA, LLC_RSP, pollfinal);
		break;
	case LLCFT_UA + LLC_RSP:
		if (LLC_GETFLAG(linkp, P) == pollfinal) {
			LLC_STOP_ACK_TIMER(linkp);
			LLC_RESETCOUNTER(linkp);
			LLC_UPDATE_P_FLAG(linkp, cmdrsp, pollfinal);
			LLC_SETFLAG(linkp, REMOTE_BUSY, 0);
			LLC_NEWSTATE(linkp, normal);
			action = LLC_RESET_CONFIRM;
		}
		break;
	case LLC_ACK_TIMER_EXPIRED:
		if (LLC_GETFLAG(linkp, S) == 1) {
			LLC_SETFLAG(linkp, P, 0);
			LLC_SETFLAG(linkp, REMOTE_BUSY, 0);
			LLC_NEWSTATE(linkp, normal);
			action = LLC_RESET_CONFIRM;
		} else if (linkp->llcl_retry < llc_n2) {
			llc_send(linkp, LLCFT_SABME, LLC_CMD, pollfinal);
			LLC_SETFLAG(linkp, P, pollfinal);
			LLC_START_ACK_TIMER(linkp);
			linkp->llcl_retry++;
		} else {
			LLC_NEWSTATE(linkp, adm);
			action = LLC_DISCONNECT_INDICATION;
		}
		break;
	case LLCFT_DISC + LLC_CMD:
		llc_send(linkp, LLCFT_DM, LLC_RSP, pollfinal);
		LLC_STOP_ACK_TIMER(linkp);
		LLC_NEWSTATE(linkp, adm);
		action = LLC_DISCONNECT_INDICATION;
		break;
	case LLCFT_DM + LLC_RSP:
		LLC_STOP_ACK_TIMER(linkp);
		LLC_NEWSTATE(linkp, adm);
		action = LLC_DISCONNECT_INDICATION;
		break;
	}
	return (action);
}

int
llc_state_d_conn(struct llc_linkcb *linkp, struct llc *frame, int frame_kind, int cmdrsp, int pollfinal)
{
	int action, command;

	action = 0;
	command = LLC_COMMAND(frame_kind, cmdrsp);
	switch (command) {
	case LLCFT_SABME + LLC_CMD:
		llc_send(linkp, LLCFT_DM, LLC_RSP, pollfinal);
		LLC_STOP_ACK_TIMER(linkp);
		LLC_NEWSTATE(linkp, adm);
		break;
	case LLCFT_UA + LLC_RSP:
		if (LLC_GETFLAG(linkp, P) == pollfinal) {
			LLC_STOP_ACK_TIMER(linkp);
			LLC_NEWSTATE(linkp, adm);
		}
		break;
	case LLCFT_DISC + LLC_CMD:
		llc_send(linkp, LLCFT_UA, LLC_RSP, pollfinal);
		break;
	case LLCFT_DM + LLC_RSP:
		LLC_STOP_ACK_TIMER(linkp);
		LLC_NEWSTATE(linkp, adm);
		break;
	case LLC_ACK_TIMER_EXPIRED:
		if (linkp->llcl_retry < llc_n2) {
			llc_send(linkp, LLCFT_DISC, LLC_CMD, pollfinal);
			LLC_SETFLAG(linkp, P, pollfinal);
			LLC_START_ACK_TIMER(linkp);
			linkp->llcl_retry++;
		} else LLC_NEWSTATE(linkp, adm);
		break;
	}
	return (action);
}

int
llc_state_error(struct llc_linkcb *linkp, struct llc *frame, int frame_kind, int cmdrsp, int pollfinal)
{
	int action, command;

	action = 0;
	command = LLC_COMMAND(frame_kind, cmdrsp);
	switch (command) {
	case LLCFT_SABME + LLC_CMD:
		LLC_STOP_ACK_TIMER(linkp);
		LLC_NEWSTATE(linkp, reset_check);
		action = LLC_RESET_INDICATION_REMOTE;
		break;
	case LLCFT_DISC + LLC_CMD:
		llc_send(linkp, LLCFT_UA, LLC_RSP, pollfinal);
		LLC_STOP_ACK_TIMER(linkp);
		LLC_NEWSTATE(linkp, adm);
		action = LLC_DISCONNECT_INDICATION;
		break;
	case LLCFT_DM + LLC_RSP:
		LLC_STOP_ACK_TIMER(linkp);
		LLC_NEWSTATE(linkp, adm);
		action = LLC_DISCONNECT_INDICATION;
		break;
	case LLCFT_FRMR + LLC_RSP:
		LLC_STOP_ACK_TIMER(linkp);
		LLC_SETFLAG(linkp, S, 0);
		LLC_NEWSTATE(linkp, reset_wait);
		action = LLC_FRMR_RECEIVED;
		break;
	case LLC_ACK_TIMER_EXPIRED:
		if (linkp->llcl_retry < llc_n2) {
			llc_send(linkp, LLCFT_FRMR, LLC_RSP, 0);
			LLC_START_ACK_TIMER(linkp);
			linkp->llcl_retry++;
		} else {
			LLC_SETFLAG(linkp, S, 0);
			LLC_NEWSTATE(linkp, reset_wait);
			action = LLC_RESET_INDICATION_LOCAL;
		}
		break;
	default:
		if (cmdrsp == LLC_CMD){
			llc_send(linkp, LLCFT_FRMR, LLC_RSP, pollfinal);
			LLC_START_ACK_TIMER(linkp);
		}
		break;
	}
	return (action);
}

int
llc_state_core(struct llc_linkcb *linkp, struct llc *frame, int frame_kind, int cmdrsp, int pollfinal)
{
	int action, command;

	action = 0;
	command = LLC_COMMAND(frame_kind, cmdrsp);
	switch (command) {
	case NL_DISCONNECT_REQUEST:
		llc_send(linkp, LLCFT_DISC, LLC_CMD, pollfinal);
		LLC_SETFLAG(linkp, P, pollfinal);
		LLC_STOP_ALL_TIMERS(linkp);
		LLC_START_ACK_TIMER(linkp);
		linkp->llcl_retry = 0;
		LLC_NEWSTATE(linkp, d_conn);
		break;
	case NL_RESET_REQUEST:
		llc_send(linkp, LLCFT_SABME, LLC_CMD, pollfinal);
		LLC_SETFLAG(linkp, P, pollfinal);
		LLC_STOP_ALL_TIMERS(linkp);
		LLC_START_ACK_TIMER(linkp);
		linkp->llcl_retry = 0;
		LLC_SETFLAG(linkp, S, 0);
		LLC_NEWSTATE(linkp, reset);
		break;
	case LLCFT_SABME + LLC_CMD:
		LLC_SETFLAG(linkp, F, pollfinal);
		LLC_STOP_ALL_TIMERS(linkp);
		LLC_NEWSTATE(linkp, reset_check);
		action = LLC_RESET_INDICATION_REMOTE;
		break;
	case LLCFT_DISC + LLC_CMD:
		llc_send(linkp, LLCFT_UA, LLC_RSP, pollfinal);
		LLC_STOP_ALL_TIMERS(linkp);
		LLC_NEWSTATE(linkp, adm);
		action = LLC_DISCONNECT_INDICATION;
		break;
	case LLCFT_FRMR + LLC_RSP:
		LLC_STOP_ALL_TIMERS(linkp);
		LLC_SETFLAG(linkp, S, 0);
		LLC_NEWSTATE(linkp, reset_wait);
		action =  LLC_FRMR_RECEIVED;
		break;
	case LLCFT_DM + LLC_RSP:
		LLC_STOP_ALL_TIMERS(linkp);
		LLC_NEWSTATE(linkp, adm);
		action = LLC_DISCONNECT_INDICATION;
		break;
	case LLC_INVALID_NR + LLC_CMD:
	case LLC_INVALID_NS + LLC_CMD:
		LLC_SETFRMR(linkp, frame, cmdrsp,
			 (frame_kind == LLC_INVALID_NR ? LLC_FRMR_Z :
			  (LLC_FRMR_V | LLC_FRMR_W)));
		llc_send(linkp, LLCFT_FRMR, LLC_RSP, pollfinal);
		LLC_STOP_ALL_TIMERS(linkp);
		LLC_START_ACK_TIMER(linkp);
		linkp->llcl_retry = 0;
		LLC_NEWSTATE(linkp, error);
		action = LLC_FRMR_SENT;
		break;
	case LLC_INVALID_NR + LLC_RSP:
	case LLC_INVALID_NS + LLC_RSP:
	case LLCFT_UA + LLC_RSP:
	case LLC_BAD_PDU: {
		char frmrcause = 0;

		switch (frame_kind) {
		case LLC_INVALID_NR:
			frmrcause = LLC_FRMR_Z;
			break;
		case LLC_INVALID_NS:
			frmrcause = LLC_FRMR_V | LLC_FRMR_W;
			break;
		default:
			frmrcause = LLC_FRMR_W;
		}
		LLC_SETFRMR(linkp, frame, cmdrsp, frmrcause);
		llc_send(linkp, LLCFT_FRMR, LLC_RSP, 0);
		LLC_STOP_ALL_TIMERS(linkp);
		LLC_START_ACK_TIMER(linkp);
		linkp->llcl_retry = 0;
		LLC_NEWSTATE(linkp, error);
		action = LLC_FRMR_SENT;
		break;
	}
	default:
		if (cmdrsp == LLC_RSP && pollfinal == 1 &&
		    LLC_GETFLAG(linkp, P) == 0) {
			LLC_SETFRMR(linkp, frame, cmdrsp, LLC_FRMR_W);
			LLC_STOP_ALL_TIMERS(linkp);
			LLC_START_ACK_TIMER(linkp);
			linkp->llcl_retry = 0;
			LLC_NEWSTATE(linkp, error);
			action = LLC_FRMR_SENT;
		}
		break;
	case LLC_P_TIMER_EXPIRED:
	case LLC_ACK_TIMER_EXPIRED:
	case LLC_REJ_TIMER_EXPIRED:
	case LLC_BUSY_TIMER_EXPIRED:
		if (linkp->llcl_retry >= llc_n2) {
			LLC_STOP_ALL_TIMERS(linkp);
			LLC_SETFLAG(linkp, S, 0);
			LLC_NEWSTATE(linkp, reset_wait);
			action = LLC_RESET_INDICATION_LOCAL;
		}
		break;
	}
	return (action);
}

int
llc_state_normal(struct llc_linkcb *linkp, struct llc *frame, int frame_kind, int cmdrsp, int pollfinal)
{
	int action, command;

	action = LLC_PASSITON;
	command = LLC_COMMAND(frame_kind, cmdrsp);
	switch (command) {
	case NL_DATA_REQUEST:
		if (LLC_GETFLAG(linkp, REMOTE_BUSY) == 0) {
			/* multiple possibilities */
			llc_send(linkp, LLCFT_INFO, LLC_CMD, 0);
			if (LLC_TIMERXPIRED(linkp, ACK) != LLC_TIMER_RUNNING) {
				LLC_START_ACK_TIMER(linkp);
			}
		}
		action = 0;
		break;
	case LLC_LOCAL_BUSY_DETECTED:
		if (LLC_GETFLAG(linkp, P) == 0) {
			/* multiple possibilities --- action-wise */
			/* multiple possibilities --- CMD/RSP-wise */
			llc_send(linkp, LLCFT_RNR, LLC_CMD, 0);
			LLC_START_P_TIMER(linkp);
			LLC_SETFLAG(linkp, DATA, 0);
			LLC_NEWSTATE(linkp, busy);
			action = 0;
		} else {
			/* multiple possibilities --- CMD/RSP-wise */
			llc_send(linkp, LLCFT_RNR, LLC_CMD, 0);
			LLC_SETFLAG(linkp, DATA, 0);
			LLC_NEWSTATE(linkp, busy);
			action = 0;
		}
		break;
	case LLC_INVALID_NS + LLC_CMD:
	case LLC_INVALID_NS + LLC_RSP: {
		register int p = LLC_GETFLAG(linkp, P);
		register int nr = LLCGBITS(frame->llc_control_ext, s_nr);

		if (cmdrsp == LLC_CMD && pollfinal == 1) {
			llc_send(linkp, LLCFT_REJ, LLC_RSP, 1);
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			LLC_START_REJ_TIMER(linkp);
			LLC_NEWSTATE(linkp, reject);
			action = 0;
		} else if (pollfinal == 0 && p == 1) {
			llc_send(linkp, LLCFT_REJ, LLC_CMD, 0);
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			LLC_START_REJ_TIMER(linkp);
			LLC_NEWSTATE(linkp, reject);
			action = 0;
		} else if ((pollfinal == 0 && p == 0) ||
			   (pollfinal == 1 && p == 1 && cmdrsp == LLC_RSP)) {
			llc_send(linkp, LLCFT_REJ, LLC_CMD, 1);
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			LLC_START_P_TIMER(linkp);
			LLC_START_REJ_TIMER(linkp);
			if (cmdrsp == LLC_RSP && pollfinal == 1) {
				LLC_CLEAR_REMOTE_BUSY(linkp, action);
			} else action = 0;
			LLC_NEWSTATE(linkp, reject);
		}
		break;
	}
	case LLCFT_INFO + LLC_CMD:
	case LLCFT_INFO + LLC_RSP: {
		register int p = LLC_GETFLAG(linkp, P);
		register int nr = LLCGBITS(frame->llc_control_ext, s_nr);

		if (cmdrsp == LLC_CMD && pollfinal == 1) {
			LLC_INC(linkp->llcl_vr);
			LLC_SENDACKNOWLEDGE(linkp, LLC_RSP, 1);
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			action = LLC_DATA_INDICATION;
		} else if (pollfinal == 0 && p == 1) {
			LLC_INC(linkp->llcl_vr);
			LLC_SENDACKNOWLEDGE(linkp, LLC_CMD, 0);
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			action = LLC_DATA_INDICATION;
		} else if ((pollfinal == 0 && p == 0 && cmdrsp == LLC_CMD) ||
			   (pollfinal == p && cmdrsp == LLC_RSP)) {
			LLC_INC(linkp->llcl_vr);
			LLC_UPDATE_P_FLAG(linkp, cmdrsp, pollfinal);
			LLC_SENDACKNOWLEDGE(linkp, LLC_CMD, 0);
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			if (cmdrsp == LLC_RSP && pollfinal == 1)
				LLC_CLEAR_REMOTE_BUSY(linkp, action);
			action = LLC_DATA_INDICATION;
		}
		break;
	}
	case LLCFT_RR + LLC_CMD:
	case LLCFT_RR + LLC_RSP: {
		register int p = LLC_GETFLAG(linkp, P);
		register int nr = LLCGBITS(frame->llc_control_ext, s_nr);

		if (cmdrsp == LLC_CMD && pollfinal == 1) {
			LLC_SENDACKNOWLEDGE(linkp, LLC_RSP, 1);
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			LLC_CLEAR_REMOTE_BUSY(linkp, action);
		} else if ((pollfinal == 0) ||
			   (cmdrsp == LLC_RSP && pollfinal == 1 && p == 1)) {
			LLC_UPDATE_P_FLAG(linkp, cmdrsp, pollfinal);
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			LLC_CLEAR_REMOTE_BUSY(linkp, action);
		}
		break;
	}
	case LLCFT_RNR + LLC_CMD:
	case LLCFT_RNR + LLC_RSP: {
		register int p = LLC_GETFLAG(linkp, P);
		register int nr = LLCGBITS(frame->llc_control_ext, s_nr);

		if (cmdrsp == LLC_CMD && pollfinal == 1) {
			llc_send(linkp, LLCFT_RR, LLC_RSP, 1);
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			LLC_SET_REMOTE_BUSY(linkp, action);
		} else if ((pollfinal == 0) ||
			   (cmdrsp == LLC_RSP && pollfinal == 1 && p == 1)) {
			LLC_UPDATE_P_FLAG(linkp, cmdrsp, pollfinal);
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			LLC_SET_REMOTE_BUSY(linkp, action);
		}
		break;
	}
	case LLCFT_REJ + LLC_CMD:
	case LLCFT_REJ + LLC_RSP: {
		register int p = LLC_GETFLAG(linkp, P);
		register int nr = LLCGBITS(frame->llc_control_ext, s_nr);

		if (cmdrsp == LLC_CMD && pollfinal == 1) {
			linkp->llcl_vs = nr;
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			llc_resend(linkp, LLC_RSP, 1);
			LLC_CLEAR_REMOTE_BUSY(linkp, action);
		} else if (pollfinal == 0 && p == 1) {
			linkp->llcl_vs = nr;
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			llc_resend(linkp, LLC_CMD, 0);
			LLC_CLEAR_REMOTE_BUSY(linkp, action);
		} else if ((pollfinal == 0 && p == 0 && cmdrsp == LLC_CMD) ||
			   (pollfinal == p && cmdrsp == LLC_RSP)) {
			linkp->llcl_vs = nr;
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			LLC_START_P_TIMER(linkp);
			llc_resend(linkp, LLC_CMD, 1);
			LLC_CLEAR_REMOTE_BUSY(linkp, action);
		}
		break;
	}
	case NL_INITIATE_PF_CYCLE:
		if (LLC_GETFLAG(linkp, P) == 0) {
			llc_send(linkp, LLCFT_RR, LLC_CMD, 1);
			LLC_START_P_TIMER(linkp);
			action = 0;
		}
		break;
	case LLC_P_TIMER_EXPIRED:
		if (linkp->llcl_retry < llc_n2) {
			llc_send(linkp, LLCFT_RR, LLC_CMD, 1);
			LLC_START_P_TIMER(linkp);
			linkp->llcl_retry++;
			LLC_NEWSTATE(linkp, await);
			action = 0;
		}
		break;
	case LLC_ACK_TIMER_EXPIRED:
	case LLC_BUSY_TIMER_EXPIRED:
		if ((LLC_GETFLAG(linkp, P) == 0)
		    && (linkp->llcl_retry < llc_n2)) {
			llc_send(linkp, LLCFT_RR, LLC_CMD, 1);
			LLC_START_P_TIMER(linkp);
			linkp->llcl_retry++;
			LLC_NEWSTATE(linkp, await);
			action = 0;
		}
		break;
	}
	if (action == LLC_PASSITON) {
		action = llc_state_core(linkp, frame, frame_kind, cmdrsp, pollfinal);
	}
	return (action);
}

int
llc_state_busy(struct llc_linkcb *linkp, struct llc *frame, int frame_kind, int cmdrsp, int pollfinal)
{
	int action, command;

	action = LLC_PASSITON;
	command = LLC_COMMAND(frame_kind, cmdrsp);
	switch (command) {
	case NL_DATA_REQUEST:
		if (LLC_GETFLAG(linkp, REMOTE_BUSY) == 0)
			if (LLC_GETFLAG(linkp, P) == 0) {
				llc_send(linkp, LLCFT_INFO, LLC_CMD, 1);
				LLC_START_P_TIMER(linkp);
				if (LLC_TIMERXPIRED(linkp, ACK) != LLC_TIMER_RUNNING)
					LLC_START_ACK_TIMER(linkp);
				action = 0;
			} else {
				llc_send(linkp, LLCFT_INFO, LLC_CMD, 0);
				if (LLC_TIMERXPIRED(linkp, ACK) != LLC_TIMER_RUNNING)
					LLC_START_ACK_TIMER(linkp);
				action = 0;
			}
		break;
	case LLC_LOCAL_BUSY_CLEARED: {
		register int p = LLC_GETFLAG(linkp, P);
		register int df = LLC_GETFLAG(linkp, DATA);

		switch (df) {
		case 1:
			if (p == 0) {
				/* multiple possibilities */
				llc_send(linkp, LLCFT_REJ, LLC_CMD, 1);
				LLC_START_REJ_TIMER(linkp);
				LLC_START_P_TIMER(linkp);
				LLC_NEWSTATE(linkp, reject);
				action = 0;
			} else {
				llc_send(linkp, LLCFT_REJ, LLC_CMD, 0);
				LLC_START_REJ_TIMER(linkp);
				LLC_NEWSTATE(linkp, reject);
				action = 0;
			}
			break;
		case 0:
			if (p == 0) {
				/* multiple possibilities */
				llc_send(linkp, LLCFT_RR, LLC_CMD, 1);
				LLC_START_P_TIMER(linkp);
				LLC_NEWSTATE(linkp, normal);
				action = 0;
			} else {
				llc_send(linkp, LLCFT_RR, LLC_CMD, 0);
				LLC_NEWSTATE(linkp, normal);
				action = 0;
			}
			break;
		case 2:
			if (p == 0) {
				/* multiple possibilities */
				llc_send(linkp, LLCFT_RR, LLC_CMD, 1);
				LLC_START_P_TIMER(linkp);
				LLC_NEWSTATE(linkp, reject);
				action = 0;
			} else {
				llc_send(linkp, LLCFT_RR, LLC_CMD, 0);
				LLC_NEWSTATE(linkp, reject);
				action = 0;
			}
			break;
		}
		break;
	}
	case LLC_INVALID_NS + LLC_CMD:
	case LLC_INVALID_NS + LLC_RSP: {
		register int p = LLC_GETFLAG(linkp, P);
		register int nr = LLCGBITS(frame->llc_control_ext, s_nr);

		if (cmdrsp == LLC_CMD && pollfinal == 1) {
			llc_send(linkp, LLCFT_RNR, LLC_RSP, 1);
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			if (LLC_GETFLAG(linkp, DATA) == 0)
				LLC_SETFLAG(linkp, DATA, 1);
			action = 0;
		} else if ((cmdrsp == LLC_CMD && pollfinal == 0 && p == 0)
				|| (cmdrsp == LLC_RSP && pollfinal == p)) {
			llc_send(linkp, LLCFT_RNR, LLC_CMD, 0);
			LLC_UPDATE_P_FLAG(linkp, cmdrsp, pollfinal);
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			if (LLC_GETFLAG(linkp, DATA) == 0)
				LLC_SETFLAG(linkp, DATA, 1);
			if (cmdrsp == LLC_RSP && pollfinal == 1) {
				LLC_CLEAR_REMOTE_BUSY(linkp, action);
			} else
				action = 0;
		} else if (pollfinal == 0 && p == 1) {
			llc_send(linkp, LLCFT_RNR, LLC_RSP, 1);
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			if (LLC_GETFLAG(linkp, DATA) == 0)
				LLC_SETFLAG(linkp, DATA, 1);
			action = 0;
		}
		break;
	}
	case LLCFT_INFO + LLC_CMD:
	case LLCFT_INFO + LLC_RSP: {
		register int p = LLC_GETFLAG(linkp, P);
		register int nr = LLCGBITS(frame->llc_control_ext, s_nr);

		if (cmdrsp == LLC_CMD && pollfinal == 1) {
			LLC_INC(linkp->llcl_vr);
			llc_send(linkp, LLCFT_RNR, LLC_RSP, 1);
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			if (LLC_GETFLAG(linkp, DATA) == 2)
				LLC_STOP_REJ_TIMER(linkp);
			LLC_SETFLAG(linkp, DATA, 0);
			action = LLC_DATA_INDICATION;
		} else if ((cmdrsp == LLC_CMD && pollfinal == 0 && p == 0)
				|| (cmdrsp == LLC_RSP && pollfinal == p)) {
			LLC_INC(linkp->llcl_vr);
			llc_send(linkp, LLCFT_RNR, LLC_CMD, 1);
			LLC_START_P_TIMER(linkp);
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			if (LLC_GETFLAG(linkp, DATA) == 2)
				LLC_STOP_REJ_TIMER(linkp);
			if (cmdrsp == LLC_RSP && pollfinal == 1)
				LLC_CLEAR_REMOTE_BUSY(linkp, action);
			action = LLC_DATA_INDICATION;
		} else if (pollfinal == 0 && p == 1) {
			LLC_INC(linkp->llcl_vr);
			llc_send(linkp, LLCFT_RNR, LLC_CMD, 0);
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			if (LLC_GETFLAG(linkp, DATA) == 2)
				LLC_STOP_REJ_TIMER(linkp);
			LLC_SETFLAG(linkp, DATA, 0);
			action = LLC_DATA_INDICATION;
		}
		break;
	}
	case LLCFT_RR + LLC_CMD:
	case LLCFT_RR + LLC_RSP:
	case LLCFT_RNR + LLC_CMD:
	case LLCFT_RNR + LLC_RSP: {
		register int p = LLC_GETFLAG(linkp, P);
		register int nr = LLCGBITS(frame->llc_control_ext, s_nr);

		if (cmdrsp == LLC_CMD && pollfinal == 1) {
			llc_send(linkp, LLCFT_RNR, LLC_RSP, 1);
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			if (frame_kind == LLCFT_RR) {
				LLC_CLEAR_REMOTE_BUSY(linkp, action);
			} else {
				LLC_SET_REMOTE_BUSY(linkp, action);
			}
		} else if (pollfinal == 0 || (cmdrsp == LLC_RSP && pollfinal == 1)) {
			LLC_UPDATE_P_FLAG(linkp, cmdrsp, pollfinal);
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			if (frame_kind == LLCFT_RR) {
				LLC_CLEAR_REMOTE_BUSY(linkp, action);
			} else {
				LLC_SET_REMOTE_BUSY(linkp, action);
			}
		}
		break;
	}
	case LLCFT_REJ + LLC_CMD:
	case LLCFT_REJ + LLC_RSP: {
		register int p = LLC_GETFLAG(linkp, P);
		register int nr = LLCGBITS(frame->llc_control_ext, s_nr);

		if (cmdrsp == LLC_CMD && pollfinal == 1) {
			linkp->llcl_vs = nr;
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			llc_send(linkp, LLCFT_RNR, LLC_RSP, 1);
			llc_resend(linkp, LLC_CMD, 0);
			LLC_CLEAR_REMOTE_BUSY(linkp, action);
		} else if ((cmdrsp == LLC_CMD && pollfinal == 0 && p == 0)
				|| (cmdrsp == LLC_RSP && pollfinal == p)) {
			linkp->llcl_vs = nr;
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			LLC_UPDATE_P_FLAG(linkp, cmdrsp, pollfinal);
			llc_resend(linkp, LLC_CMD, 0);
			LLC_CLEAR_REMOTE_BUSY(linkp, action);
		} else if (pollfinal == 0 && p == 1) {
			linkp->llcl_vs = nr;
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			llc_resend(linkp, LLC_CMD, 0);
			LLC_CLEAR_REMOTE_BUSY(linkp, action);
		}
		break;
	}
	case NL_INITIATE_PF_CYCLE:
		if (LLC_GETFLAG(linkp, P) == 0) {
			llc_send(linkp, LLCFT_RNR, LLC_CMD, 1);
			LLC_START_P_TIMER(linkp);
			action = 0;
		}
		break;
	case LLC_P_TIMER_EXPIRED:
		/* multiple possibilities */
		if (linkp->llcl_retry < llc_n2) {
			llc_send(linkp, LLCFT_RNR, LLC_CMD, 1);
			LLC_START_P_TIMER(linkp);
			linkp->llcl_retry++;
			LLC_NEWSTATE(linkp, await_busy);
			action = 0;
		}
		break;
	case LLC_ACK_TIMER_EXPIRED:
	case LLC_BUSY_TIMER_EXPIRED:
		if (LLC_GETFLAG(linkp, P) == 0 && linkp->llcl_retry < llc_n2) {
			llc_send(linkp, LLCFT_RNR, LLC_CMD, 1);
			LLC_START_P_TIMER(linkp);
			linkp->llcl_retry++;
			LLC_NEWSTATE(linkp, await_busy);
			action = 0;
		}
		break;
	case LLC_REJ_TIMER_EXPIRED:
		if (linkp->llcl_retry < llc_n2)
			if (LLC_GETFLAG(linkp, P) == 0) {
				/* multiple possibilities */
				llc_send(linkp, LLCFT_RNR, LLC_CMD, 1);
				LLC_START_P_TIMER(linkp);
				linkp->llcl_retry++;
				LLC_SETFLAG(linkp, DATA, 1);
				LLC_NEWSTATE(linkp, await_busy);
				action = 0;
			} else {
				LLC_SETFLAG(linkp, DATA, 1);
				LLC_NEWSTATE(linkp, busy);
				action = 0;
			}

		break;
	}
	if (action == LLC_PASSITON) {
		action = llc_state_core(linkp, frame, frame_kind, cmdrsp, pollfinal);
	}
	return (action);
}

int
llc_state_reject(struct llc_linkcb *linkp, struct llc *frame, int frame_kind, int cmdrsp, int pollfinal)
{
	int action, command;

	action = LLC_PASSITON;
	command = LLC_COMMAND(frame_kind, cmdrsp);
	switch (command) {
	case NL_DATA_REQUEST:
		if (LLC_GETFLAG(linkp, P) == 0) {
			llc_send(linkp, LLCFT_INFO, LLC_CMD, 1);
			LLC_START_P_TIMER(linkp);
			if (LLC_TIMERXPIRED(linkp, ACK) != LLC_TIMER_RUNNING)
				LLC_START_ACK_TIMER(linkp);
			LLC_NEWSTATE(linkp, reject);
			action = 0;
		} else {
			llc_send(linkp, LLCFT_INFO, LLC_CMD, 0);
			if (LLC_TIMERXPIRED(linkp, ACK) != LLC_TIMER_RUNNING)
				LLC_START_ACK_TIMER(linkp);
			LLC_NEWSTATE(linkp, reject);
			action = 0;
		}
		break;
	case NL_LOCAL_BUSY_DETECTED:
		if (LLC_GETFLAG(linkp, P) == 0) {
			llc_send(linkp, LLCFT_RNR, LLC_CMD, 1);
			LLC_START_P_TIMER(linkp);
			LLC_SETFLAG(linkp, DATA, 2);
			LLC_NEWSTATE(linkp, busy);
			action = 0;
		} else {
			llc_send(linkp, LLCFT_RNR, LLC_CMD, 0);
			LLC_SETFLAG(linkp, DATA, 2);
			LLC_NEWSTATE(linkp, busy);
			action = 0;
		}
		break;
	case LLC_INVALID_NS + LLC_CMD:
	case LLC_INVALID_NS + LLC_RSP: {
		register int p = LLC_GETFLAG(linkp, P);
		register int nr = LLCGBITS(frame->llc_control_ext, s_nr);

		if (cmdrsp == LLC_CMD && pollfinal == 1) {
			llc_send(linkp, LLCFT_RR, LLC_RSP, 1);
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			action = 0;
		} else if (pollfinal == 0
				|| (cmdrsp == LLC_RSP && pollfinal == 1 && p == 1)) {
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			LLC_UPDATE_P_FLAG(linkp, cmdrsp, pollfinal);
			if (cmdrsp == LLC_RSP && pollfinal == 1) {
				LLC_CLEAR_REMOTE_BUSY(linkp, action);
			} else
				action = 0;
		}
		break;
	}
	case LLCFT_INFO + LLC_CMD:
	case LLCFT_INFO + LLC_RSP: {
		register int p = LLC_GETFLAG(linkp, P);
		register int nr = LLCGBITS(frame->llc_control_ext, s_nr);

		if (cmdrsp == LLC_CMD && pollfinal == 1) {
			LLC_INC(linkp->llcl_vr);
			LLC_SENDACKNOWLEDGE(linkp, LLC_RSP, 1);
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			LLC_STOP_REJ_TIMER(linkp);
			LLC_NEWSTATE(linkp, normal);
			action = LLC_DATA_INDICATION;
		} else if ((cmdrsp = LLC_RSP && pollfinal == p)
				|| (cmdrsp == LLC_CMD && pollfinal == 0 && p == 0)) {
			LLC_INC(linkp->llcl_vr);
			LLC_SENDACKNOWLEDGE(linkp, LLC_CMD, 1);
			LLC_START_P_TIMER(linkp);
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			if (cmdrsp == LLC_RSP && pollfinal == 1)
				LLC_CLEAR_REMOTE_BUSY(linkp, action);
			LLC_STOP_REJ_TIMER(linkp);
			LLC_NEWSTATE(linkp, normal);
			action = LLC_DATA_INDICATION;
		} else if (pollfinal == 0 && p == 1) {
			LLC_INC(linkp->llcl_vr);
			LLC_SENDACKNOWLEDGE(linkp, LLC_CMD, 0);
			LLC_STOP_REJ_TIMER(linkp);
			LLC_NEWSTATE(linkp, normal);
			action = LLC_DATA_INDICATION;
		}
		break;
	}
	case LLCFT_RR + LLC_CMD:
	case LLCFT_RR + LLC_RSP: {
		register int p = LLC_GETFLAG(linkp, P);
		register int nr = LLCGBITS(frame->llc_control_ext, s_nr);

		if (cmdrsp == LLC_CMD && pollfinal == 1) {
			LLC_SENDACKNOWLEDGE(linkp, LLC_RSP, 1);
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			LLC_CLEAR_REMOTE_BUSY(linkp, action);
		} else if (pollfinal == 0
				|| (cmdrsp == LLC_RSP && pollfinal == 1 && p == 1)) {
			LLC_UPDATE_P_FLAG(linkp, cmdrsp, pollfinal);
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			LLC_CLEAR_REMOTE_BUSY(linkp, action);
		}
		break;
	}
	case LLCFT_RNR + LLC_CMD:
	case LLCFT_RNR + LLC_RSP: {
		register int p = LLC_GETFLAG(linkp, P);
		register int nr = LLCGBITS(frame->llc_control_ext, s_nr);

		if (cmdrsp == LLC_CMD && pollfinal == 1) {
			llc_send(linkp, LLCFT_RR, LLC_RSP, 1);
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			LLC_SET_REMOTE_BUSY(linkp, action);
		} else if (pollfinal == 0
				|| (cmdrsp == LLC_RSP && pollfinal == 1 && p == 1)) {
			LLC_UPDATE_P_FLAG(linkp, cmdrsp, pollfinal);
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			action = 0;
		}
		break;
	}
	case LLCFT_REJ + LLC_CMD:
	case LLCFT_REJ + LLC_RSP: {
		register int p = LLC_GETFLAG(linkp, P);
		register int nr = LLCGBITS(frame->llc_control_ext, s_nr);

		if (cmdrsp == LLC_CMD && pollfinal == 1) {
			linkp->llcl_vs = nr;
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			llc_resend(linkp, LLC_RSP, 1);
			LLC_CLEAR_REMOTE_BUSY(linkp, action);
		} else if ((cmdrsp == LLC_CMD && pollfinal == 0 && p == 0)
				|| (cmdrsp == LLC_RSP && pollfinal == p)) {
			linkp->llcl_vs = nr;
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			LLC_UPDATE_P_FLAG(linkp, cmdrsp, pollfinal);
			llc_resend(linkp, LLC_CMD, 0);
			LLC_CLEAR_REMOTE_BUSY(linkp, action);
		} else if (pollfinal == 0 && p == 1) {
			linkp->llcl_vs = nr;
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			llc_resend(linkp, LLC_CMD, 0);
			LLC_CLEAR_REMOTE_BUSY(linkp, action);
		}
		break;
	}
	case NL_INITIATE_PF_CYCLE:
		if (LLC_GETFLAG(linkp, P) == 0) {
			llc_send(linkp, LLCFT_RR, LLC_CMD, 1);
			LLC_START_P_TIMER(linkp);
			action = 0;
		}
		break;
	case LLC_REJ_TIMER_EXPIRED:
		if (LLC_GETFLAG(linkp, P) == 0 && linkp->llcl_retry < llc_n2) {
			llc_send(linkp, LLCFT_REJ, LLC_CMD, 1);
			LLC_START_P_TIMER(linkp);
			LLC_START_REJ_TIMER(linkp);
			linkp->llcl_retry++;
			action = 0;
		}
	case LLC_P_TIMER_EXPIRED:
		if (linkp->llcl_retry < llc_n2) {
			llc_send(linkp, LLCFT_RR, LLC_CMD, 1);
			LLC_START_P_TIMER(linkp);
			LLC_START_REJ_TIMER(linkp);
			linkp->llcl_retry++;
			LLC_NEWSTATE(linkp, await_reject);
			action = 0;
		}
		break;
	case LLC_ACK_TIMER_EXPIRED:
	case LLC_BUSY_TIMER_EXPIRED:
		if (LLC_GETFLAG(linkp, P) == 0 && linkp->llcl_retry < llc_n2) {
			llc_send(linkp, LLCFT_RR, LLC_CMD, 1);
			LLC_START_P_TIMER(linkp);
			LLC_START_REJ_TIMER(linkp);
			linkp->llcl_retry++;
			/*
			 * I cannot locate the description of RESET_V(S)
			 * in ISO 8802-2, table 7-1, state REJECT, last event,
			 * and  assume they meant to set V(S) to 0 ...
			 */
			linkp->llcl_vs = 0; /* XXX */
			LLC_NEWSTATE(linkp, await_reject);
			action = 0;
		}

		break;
	}
	if (action == LLC_PASSITON) {
		action = llc_state_core(linkp, frame, frame_kind, cmdrsp, pollfinal);
	}
	return (action);
}

int
llc_state_await(struct llc_linkcb *linkp, struct llc *frame, int frame_kind, int cmdrsp, int pollfinal)
{
	int action, command;

	action = 0;
	command = LLC_COMMAND(frame_kind, cmdrsp);
	switch (command) {
	case LLC_LOCAL_BUSY_DETECTED:
		llc_send(linkp, LLCFT_RNR, LLC_CMD, 0);
		LLC_SETFLAG(linkp, DATA, 0);
		LLC_NEWSTATE(linkp, await_busy);
		action = 0;
		break;
	case LLC_INVALID_NS + LLC_CMD:
	case LLC_INVALID_NS + LLC_RSP: {
		register int p = LLC_GETFLAG(linkp, P);
		register int nr = LLCGBITS(frame->llc_control_ext, s_nr);

		if (cmdrsp == LLC_CMD && pollfinal == 1) {
			llc_send(linkp, LLCFT_REJ, LLC_RSP, 1);
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			LLC_START_REJ_TIMER(linkp);
			LLC_NEWSTATE(linkp, await_reject);
			action = 0;
		} else if (cmdrsp == LLC_RSP && pollfinal == 1) {
			llc_send(linkp, LLCFT_REJ, LLC_CMD, 0);
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			linkp->llcl_vs = nr;
			LLC_STOP_P_TIMER(linkp);
			llc_resend(linkp, LLC_CMD, 0);
			LLC_START_REJ_TIMER(linkp);
			LLC_CLEAR_REMOTE_BUSY(linkp, action);
			LLC_NEWSTATE(linkp, reject);
		} else if (pollfinal == 0) {
			llc_send(linkp, LLCFT_REJ, LLC_CMD, 0);
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			LLC_START_REJ_TIMER(linkp);
			LLC_NEWSTATE(linkp, await_reject);
			action = 0;
		}
		break;
	}
	case LLCFT_INFO + LLC_RSP:
	case LLCFT_INFO + LLC_CMD: {
		register int p = LLC_GETFLAG(linkp, P);
		register int nr = LLCGBITS(frame->llc_control_ext, s_nr);

		LLC_INC(linkp->llcl_vr);
		if (cmdrsp == LLC_CMD && pollfinal == 1) {
			llc_send(linkp, LLCFT_RR, LLC_RSP, 1);
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			action = LLC_DATA_INDICATION;
		} else if (cmdrsp == LLC_RSP && pollfinal == 1) {
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			linkp->llcl_vs = nr;
			llc_resend(linkp, LLC_CMD, 1);
			LLC_START_P_TIMER(linkp);
			LLC_CLEAR_REMOTE_BUSY(linkp, action);
			LLC_NEWSTATE(linkp, normal);
			action = LLC_DATA_INDICATION;
		} else if (pollfinal == 0) {
			llc_send(linkp, LLCFT_RR, LLC_CMD, 0);
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			action = LLC_DATA_INDICATION;
		}
		break;
	}
	case LLCFT_RR + LLC_CMD:
	case LLCFT_RR + LLC_RSP:
	case LLCFT_REJ + LLC_CMD:
	case LLCFT_REJ + LLC_RSP: {
		register int p = LLC_GETFLAG(linkp, P);
		register int nr = LLCGBITS(frame->llc_control_ext, s_nr);

		if (cmdrsp == LLC_CMD && pollfinal == 1) {
			llc_send(linkp, LLCFT_RR, LLC_RSP, 1);
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			LLC_CLEAR_REMOTE_BUSY(linkp, action);
		} else if (cmdrsp == LLC_RSP && pollfinal == 1) {
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			linkp->llcl_vs = nr;
			LLC_STOP_P_TIMER(linkp);
			llc_resend(linkp, LLC_CMD, 0);
			LLC_CLEAR_REMOTE_BUSY(linkp, action);
			LLC_NEWSTATE(linkp, normal);
		} else if (pollfinal == 0) {
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			LLC_CLEAR_REMOTE_BUSY(linkp, action);
		}
		break;
	}
	case LLCFT_RNR + LLC_CMD:
	case LLCFT_RNR + LLC_RSP: {
		register int p = LLC_GETFLAG(linkp, P);
		register int nr = LLCGBITS(frame->llc_control_ext, s_nr);

		if (pollfinal == 1 && cmdrsp == LLC_CMD) {
			llc_send(linkp, LLCFT_RR, LLC_RSP, 1);
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			LLC_SET_REMOTE_BUSY(linkp, action);
		} else if (pollfinal == 1 && cmdrsp == LLC_RSP) {
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			linkp->llcl_vs = nr;
			LLC_STOP_P_TIMER(linkp);
			LLC_SET_REMOTE_BUSY(linkp, action);
			LLC_NEWSTATE(linkp, normal);
		} else if (pollfinal == 0) {
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			LLC_SET_REMOTE_BUSY(linkp, action);
		}
		break;
	}
	case LLC_P_TIMER_EXPIRED:
		if (linkp->llcl_retry < llc_n2) {
			llc_send(linkp, LLCFT_RR, LLC_CMD, 1);
			LLC_START_P_TIMER(linkp);
			linkp->llcl_retry++;
			action = 0;
		}
		break;
	}
	if (action == LLC_PASSITON) {
		action = llc_state_core(linkp, frame, frame_kind, cmdrsp, pollfinal);
	}
	return (action);
}

int
llc_state_await_busy(struct llc_linkcb *linkp, struct llc *frame, int frame_kind, int cmdrsp, int pollfinal)
{
	int action, command;

	action = 0;
	command = LLC_COMMAND(frame_kind, cmdrsp);
	switch (command) {
	case LLC_LOCAL_BUSY_CLEARED:
		switch (LLC_GETFLAG(linkp, DATA)) {
		case 1:
			llc_send(linkp, LLCFT_REJ, LLC_CMD, 0);
			LLC_START_REJ_TIMER(linkp)
			;
			LLC_NEWSTATE(linkp, await_reject);
			action = 0;
			break;
		case 0:
			llc_send(linkp, LLCFT_RR, LLC_CMD, 0);
			LLC_NEWSTATE(linkp, await);
			action = 0;
			break;
		case 2:
			llc_send(linkp, LLCFT_RR, LLC_CMD, 0);
			LLC_NEWSTATE(linkp, await_reject);
			action = 0;
			break;
		}
		break;
	case LLC_INVALID_NS + LLC_CMD:
	case LLC_INVALID_NS + LLC_RSP: {
		register int p = LLC_GETFLAG(linkp, P);
		register int nr = LLCGBITS(frame->llc_control_ext, s_nr);

		if (cmdrsp == LLC_CMD && pollfinal == 1) {
			llc_send(linkp, LLCFT_RNR, LLC_RSP, 1);
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			LLC_SETFLAG(linkp, DATA, 1);
			action = 0;
		} else if (cmdrsp == LLC_RSP && pollfinal == 1) {
			/* optionally */
			llc_send(linkp, LLCFT_RNR, LLC_CMD, 0);
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			linkp->llcl_vs = nr;
			LLC_STOP_P_TIMER(linkp);
			LLC_SETFLAG(linkp, DATA, 1);
			LLC_CLEAR_REMOTE_BUSY(linkp, action);
			llc_resend(linkp, LLC_CMD, 0);
			LLC_NEWSTATE(linkp, busy);
		} else if (pollfinal == 0) {
			/* optionally */
			llc_send(linkp, LLCFT_RNR, LLC_CMD, 0);
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			LLC_SETFLAG(linkp, DATA, 1);
			action = 0;
		}
	}
	case LLCFT_INFO + LLC_CMD:
	case LLCFT_INFO + LLC_RSP: {
		register int p = LLC_GETFLAG(linkp, P);
		register int nr = LLCGBITS(frame->llc_control_ext, s_nr);

		if (cmdrsp == LLC_CMD && pollfinal == 1) {
			llc_send(linkp, LLCFT_RNR, LLC_RSP, 1);
			LLC_INC(linkp->llcl_vr);
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			LLC_SETFLAG(linkp, DATA, 0);
			action = LLC_DATA_INDICATION;
		} else if (cmdrsp == LLC_RSP && pollfinal == 1) {
			llc_send(linkp, LLCFT_RNR, LLC_CMD, 1);
			LLC_INC(linkp->llcl_vr);
			LLC_START_P_TIMER(linkp);
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			linkp->llcl_vs = nr;
			LLC_SETFLAG(linkp, DATA, 0);
			LLC_CLEAR_REMOTE_BUSY(linkp, action);
			llc_resend(linkp, LLC_CMD, 0);
			LLC_NEWSTATE(linkp, busy);
			action = LLC_DATA_INDICATION;
		} else if (pollfinal == 0) {
			llc_send(linkp, LLCFT_RNR, LLC_CMD, 0);
			LLC_INC(linkp->llcl_vr);
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			LLC_SETFLAG(linkp, DATA, 0);
			action = LLC_DATA_INDICATION;
		}
		break;
	}
	case LLCFT_RR + LLC_CMD:
	case LLCFT_REJ + LLC_CMD:
	case LLCFT_RR + LLC_RSP:
	case LLCFT_REJ + LLC_RSP: {
		register int p = LLC_GETFLAG(linkp, P);
		register int nr = LLCGBITS(frame->llc_control_ext, s_nr);

		if (cmdrsp == LLC_CMD && pollfinal == 1) {
			llc_send(linkp, LLCFT_RNR, LLC_RSP, 1);
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			LLC_CLEAR_REMOTE_BUSY(linkp, action);
		} else if (cmdrsp == LLC_RSP && pollfinal == 1) {
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			linkp->llcl_vs = nr;
			LLC_STOP_P_TIMER(linkp);
			llc_resend(linkp, LLC_CMD, 0);
			LLC_CLEAR_REMOTE_BUSY(linkp, action);
			LLC_NEWSTATE(linkp, busy);
		} else if (pollfinal == 0) {
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			linkp->llcl_vs = nr;
			LLC_STOP_P_TIMER(linkp);
			llc_resend(linkp, LLC_CMD, 0);
			LLC_CLEAR_REMOTE_BUSY(linkp, action);
		}
		break;
	}
	case LLCFT_RNR + LLC_CMD:
	case LLCFT_RNR + LLC_RSP: {
		register int p = LLC_GETFLAG(linkp, P);
		register int nr = LLCGBITS(frame->llc_control_ext, s_nr);

		if (cmdrsp == LLC_CMD && pollfinal == 1) {
			llc_send(linkp, LLCFT_RNR, LLC_RSP, 1);
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			LLC_SET_REMOTE_BUSY(linkp, action);
		} else if (cmdrsp == LLC_RSP && pollfinal == 1) {
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			linkp->llcl_vs = nr;
			LLC_STOP_P_TIMER(linkp);
			LLC_SET_REMOTE_BUSY(linkp, action);
			LLC_NEWSTATE(linkp, busy);
		} else if (pollfinal == 0) {
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			LLC_SET_REMOTE_BUSY(linkp, action);
		}
		break;
	}
	case LLC_P_TIMER_EXPIRED:
		if (linkp->llcl_retry < llc_n2) {
			llc_send(linkp, LLCFT_RNR, LLC_CMD, 1);
			LLC_START_P_TIMER(linkp);
			linkp->llcl_retry++;
			action = 0;
		}
		break;
	}
	if (action == LLC_PASSITON) {
		action = llc_state_core(linkp, frame, frame_kind, cmdrsp, pollfinal);
	}
	return (action);
}

int
llc_state_await_reject(struct llc_linkcb *linkp, struct llc *frame, int frame_kind, int cmdrsp, int pollfinal)
{
	int action, command;

	action = LLC_PASSITON;
	command = LLC_COMMAND(frame_kind, cmdrsp);
	switch (command) {
	case LLC_LOCAL_BUSY_DETECTED:
		llc_send(linkp, LLCFT_RNR, LLC_CMD, 0);
		LLC_SETFLAG(linkp, DATA, 2);
		LLC_NEWSTATE(linkp, await_busy);
		action = 0;
		break;
	case LLC_INVALID_NS + LLC_CMD:
	case LLC_INVALID_NS + LLC_RSP: {
		register int nr = LLCGBITS(frame->llc_control_ext, s_nr);

		if (cmdrsp == LLC_CMD && pollfinal == 1) {
			llc_send(linkp, LLCFT_RR, LLC_RSP, 1);
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			action = 0;
		} else if (cmdrsp == LLC_RSP && pollfinal == 1) {
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			linkp->llcl_vs = nr;
			llc_resend(linkp, LLC_CMD, 1);
			LLC_START_P_TIMER(linkp);
			LLC_CLEAR_REMOTE_BUSY(linkp, action);
			LLC_NEWSTATE(linkp, reject);
		} else if (pollfinal == 0) {
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			action = 0;
		}
		break;
	}
	case LLCFT_INFO + LLC_CMD:
	case LLCFT_INFO + LLC_RSP: {
		register int nr = LLCGBITS(frame->llc_control_ext, s_nr);

		if (cmdrsp == LLC_CMD && pollfinal == 1) {
			LLC_INC(linkp->llcl_vr);
			llc_send(linkp, LLCFT_RR, LLC_RSP, 1);
			LLC_STOP_REJ_TIMER(linkp);
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			LLC_NEWSTATE(linkp, await);
			action = LLC_DATA_INDICATION;
		} else if (cmdrsp == LLC_RSP && pollfinal == 1) {
			LLC_INC(linkp->llcl_vr);
			LLC_STOP_P_TIMER(linkp);
			LLC_STOP_REJ_TIMER(linkp);
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			linkp->llcl_vs = nr;
			llc_resend(linkp, LLC_CMD, 0);
			LLC_CLEAR_REMOTE_BUSY(linkp, action);
			LLC_NEWSTATE(linkp, normal);
			action = LLC_DATA_INDICATION;
		} else if (pollfinal == 0) {
			LLC_INC(linkp->llcl_vr);
			llc_send(linkp, LLCFT_RR, LLC_CMD, 0);
			LLC_STOP_REJ_TIMER(linkp);
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			LLC_NEWSTATE(linkp, await);
			action = LLC_DATA_INDICATION;
		}
		break;
	}
	case LLCFT_RR + LLC_CMD:
	case LLCFT_REJ + LLC_CMD:
	case LLCFT_RR + LLC_RSP:
	case LLCFT_REJ + LLC_RSP: {
		register int nr = LLCGBITS(frame->llc_control_ext, s_nr);

		if (cmdrsp == LLC_CMD && pollfinal == 1) {
			llc_send(linkp, LLCFT_RR, LLC_RSP, 1);
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			LLC_CLEAR_REMOTE_BUSY(linkp, action);
		} else if (cmdrsp == LLC_RSP && pollfinal == 1) {
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			linkp->llcl_vs = nr;
			llc_resend(linkp, LLC_CMD, 1);
			LLC_START_P_TIMER(linkp);
			LLC_CLEAR_REMOTE_BUSY(linkp, action);
			LLC_NEWSTATE(linkp, reject);
		} else if (pollfinal == 0) {
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			LLC_CLEAR_REMOTE_BUSY(linkp, action);
		}
		break;
	}
	case LLCFT_RNR + LLC_CMD:
	case LLCFT_RNR + LLC_RSP: {
		register int nr = LLCGBITS(frame->llc_control_ext, s_nr);

		if (cmdrsp == LLC_CMD && pollfinal == 1) {
			llc_send(linkp, LLCFT_RR, LLC_RSP, 1);
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			LLC_SET_REMOTE_BUSY(linkp, action);
		} else if (cmdrsp == LLC_RSP && pollfinal == 1) {
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			linkp->llcl_vs = nr;
			LLC_STOP_P_TIMER(linkp);
			LLC_SET_REMOTE_BUSY(linkp, action);
			LLC_NEWSTATE(linkp, reject);
		} else if (pollfinal == 0) {
			LLC_UPDATE_NR_RECEIVED(linkp, nr);
			LLC_SET_REMOTE_BUSY(linkp, action);
		}
		break;
	}
	case LLC_P_TIMER_EXPIRED:
		if (linkp->llcl_retry < llc_n2) {
			llc_send(linkp, LLCFT_REJ, LLC_CMD, 1);
			LLC_START_P_TIMER(linkp);
			linkp->llcl_retry++;
			action = 0;
		}
		break;
	}
	if (action == LLC_PASSITON) {
		action = llc_state_core(linkp, frame, frame_kind, cmdrsp, pollfinal);
	}
	return (action);
}

int
llc_statehandler(struct llc_linkcb *linkp, struct llc *frame, int frame_kind, int cmdrsp, int pollfinal)
{
	register int action = 0;

once_more_and_again:
	switch (action) {
	case LLC_CONNECT_INDICATION: {
		int naction;

		LLC_TRACE(linkp, LLCTR_INTERESTING, "CONNECT INDICATION");
		linkp->llcl_nlnext = llc_sapinfo_ctlinput(linkp, PRC_CONNECT_INDICATION, (struct sockaddr *) &linkp->llcl_addr, (caddr_t) linkp);
		if (linkp->llcl_nlnext == 0) {
			naction = NL_DISCONNECT_REQUEST;
		} else {
			naction = NL_CONNECT_RESPONSE;
		}
		action = (*linkp->llcl_statehandler)(linkp, frame, naction, 0, 0);
		goto once_more_and_again;
	}
	case LLC_CONNECT_CONFIRM:
		/* llc_resend(linkp, LLC_CMD, 0); */
		llc_start(linkp);
		break;
	case LLC_DISCONNECT_INDICATION:
		LLC_TRACE(linkp, LLCTR_INTERESTING, "DISCONNECT INDICATION");
		llc_sapinfo_ctlinput(linkp, PRC_DISCONNECT_INDICATION,(struct sockaddr *) &linkp->llcl_addr, linkp->llcl_nlnext);
		break;
	case LLC_RESET_CONFIRM:
	case LLC_RESET_INDICATION_LOCAL:
		break;
	case LLC_RESET_INDICATION_REMOTE:
		LLC_TRACE(linkp, LLCTR_SHOULDKNOW, "RESET INDICATION (REMOTE)");
		action = (*linkp->llcl_statehandler)(linkp, frame, NL_RESET_RESPONSE, 0, 0);
		goto once_more_and_again;
	case LLC_FRMR_SENT:
		LLC_TRACE(linkp, LLCTR_URGENT, "FRMR SENT");
		break;
	case LLC_FRMR_RECEIVED:
		LLC_TRACE(linkp, LLCTR_URGEN, "FRMR RECEIVED");
		action = (*linkp->llcl_statehandler)(linkp, frame, NL_RESET_REQUEST, 0, 0);
		goto once_more_and_again;
	case LLC_REMOTE_BUSY:
		LLC_TRACE(linkp, LLCTR_SHOULDKNOW, "REMOTE BUSY");
		break;
	case LLC_REMOTE_NOT_BUSY:
		LLC_TRACE(linkp, LLCTR_SHOULDKNOW, "REMOTE BUSY CLEARED");
		llc_start(linkp);
		break;
	}
	return (action);
}

int
llc_decode(struct llc* frame, struct llc_linkcb * linkp)
{
	register int ft;

	ft = LLC_BAD_PDU;
	if ((frame->llc_control & 01) == 0) {
		ft = LLCFT_INFO;
	} else {
		switch (frame->llc_control) {
		/* U frames */
		case LLC_UI:
		case LLC_UI_P:
			ft = LLC_UI;
			break;
		case LLC_DM:
		case LLC_DM_P:
			ft = LLCFT_DM;
			break;
		case LLC_DISC:
		case LLC_DISC_P:
			ft = LLCFT_DISC;
			break;
		case LLC_UA:
		case LLC_UA_P:
			ft = LLCFT_UA;
			break;
		case LLC_SABME:
		case LLC_SABME_P:
			ft = LLCFT_SABME;
			break;
		case LLC_FRMR:
		case LLC_FRMR_P:
			ft = LLCFT_FRMR;
			break;
		case LLC_XID:
		case LLC_XID_P:
			ft = LLCFT_XID;
			break;
		case LLC_TEST:
		case LLC_TEST_P:
			ft = LLCFT_TEST;
			break;

		/* S frames */
		case LLC_RR:
			ft = LLCFT_RR;
			break;
		case LLC_RNR:
			ft = LLCFT_RNR;
			break;
		case LLC_REJ:
			ft = LLCFT_REJ;
			break;
		}
	}
	if (linkp) {
		switch (ft) {
		case LLCFT_INFO:
			if (LLCGBITS(frame->llc_control, i_ns) != linkp->llcl_vr) {
				ft = LLC_INVALID_NS;
				break;
			}
			/* fall thru --- yeeeeeee */
		case LLCFT_RR:
		case LLCFT_RNR:
		case LLCFT_REJ:
			/* splash! */
			if (LLC_NR_VALID(linkp, LLCGBITS(frame->llc_control_ext, s_nr))	== 0) {
				ft = LLC_INVALID_NR;
			}
			break;
		}
	}
	return (ft);
}

/*
 * llc_link_dump() - dump link info
 */

#define CHECK(l, s) { 			\
	if (LLC_STATEEQ(l, s)) { 	\
		return (#s);			\
	}							\

char *timer_names[] = { "ACK", "P", "BUSY", "REJ", "AGE" };

char *
llc_getstatename(struct llc_linkcb *linkp)
{
	char *unknown = "UNKNOWN - eh?";

	CHECK(linkp, adm);
	CHECK(linkp, conn);
	CHECK(linkp, reset_wait);
	CHECK(linkp, reset_check);
	CHECK(linkp, setup);
	CHECK(linkp, reset);
	CHECK(linkp, d_conn);
	CHECK(linkp, error);
	CHECK(linkp, normal);
	CHECK(linkp, busy);
	CHECK(linkp, reject);
	CHECK(linkp, await);
	CHECK(linkp, await_busy);
	CHECK(linkp, await_reject);
	return (unknown);
}

void
llc_link_dump(struct llc_linkcb* linkp, const char *message)
{
	register struct ifnet *ifn;
	register struct sockaddr_dl *sdl;
	register int i;
	register char *state;

	ifn = linkp->llcl_if;
	sdl = linkp->llcl_addr;

	/* print interface */
	printf("if %s%d\n", ifn->if_xname, ifn->if_unit);

	/* print message */
	printf(">> %s <<\n", message);

	/* print MAC and LSAP */
	printf("llc addr ");
	for (i = 0; i < (sdl->sdl_alen)-2; i++) {
		printf("%x:", (char)*(LLADDR(sdl)+i) & 0xff);
	}
	printf("%x,", (char)*(LLADDR(sdl)+i) & 0xff);
	printf("%x\n", (char)*(LLADDR(sdl)+i+1) & 0xff);

	/* print state we're in and timers */
	printf("state %s, ", llc_getstatename(linkp));
	for (i = LLC_ACK_SHIFT; i < LLC_AGE_SHIFT; i++) {
		printf("%s-%c %d/", timer_names[i],
				(linkp->llcl_timerflags & (1 << i) ? 'R' : 'S'),
				linkp->llcl_timers[i]);
	}
	printf("%s-%c %d\n", timer_names[i],
			(linkp->llcl_timerflags & (1 << i) ? 'R' : 'S'),
			linkp->llcl_timers[i]);

	/* print flag values */
	printf("flags P %d/F %d/S %d/DATA %d/REMOTE_BUSY %d\n",
			LLC_GETFLAG(linkp, P), LLC_GETFLAG(linkp, S),
			LLC_GETFLAG(linkp, DATA), LLC_GETFLAG(linkp, REMOTE_BUSY));

	/* print send and receive state variables, ack, and window */
	printf("V(R) %d/V(S) %d/N(R) received %d/window %d/freeslot %d\n",
			linkp->llcl_vs, linkp->llcl_vr, linkp->llcl_nr_received,
			linkp->llcl_window, linkp->llcl_freeslot);

	/* further expansions can follow here */
}

void
llc_trace(struct llc_linkcb *linkp, int level, const char *message)
{
	if (linkp->llcl_sapinfo->si_trace && level > llc_tracelevel) {
		llc_link_dump(linkp, message);
	}
	return;
}
