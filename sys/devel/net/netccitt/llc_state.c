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

#include <sys/cdefs.h>
#include <sys/queue.h>
#include <sys/types.h>

#include <net/if_dl.h>
#include <net/if_llc.h>

#include <netccitt/dll.h>
#include <netccitt/llc_var.h>

#define LLC_CMDMASK(frame_kind, cmdrsp) (((frame_kind) + (cmdrsp)) - (cmdrsp))

int
llc_command(int frame_kind, int cmdrsp)
{
	int cmdmask;

	cmdmask = LLC_CMDMASK(frame_kind, cmdrsp);
	switch (cmdrsp) {
	case LLC_CMD:
		switch (frame_kind) {
		case LLCFT_INFO:
			cmdmask = LLC_CMDMASK(frame_kind, cmdrsp);
			break;
		case LLCFT_RR:
			cmdmask = LLC_CMDMASK(frame_kind, cmdrsp);
			break;
		case LLCFT_RNR:
			cmdmask = LLC_CMDMASK(frame_kind, cmdrsp);
			break;
		case LLCFT_REJ:
			cmdmask = LLC_CMDMASK(frame_kind, cmdrsp);
			break;
		case LLCFT_SABME:
			cmdmask = LLC_CMDMASK(frame_kind, cmdrsp);
			break;
		case LLCFT_DISC:
			cmdmask = LLC_CMDMASK(frame_kind, cmdrsp);
			break;
		case LLC_INVALID_NR:
			cmdmask = LLC_CMDMASK(frame_kind, cmdrsp);
			break;
		case LLC_INVALID_NS:
			cmdmask = LLC_CMDMASK(frame_kind, cmdrsp);
			break;
		}
		break;
	case LLC_RSP:
		switch (frame_kind) {
		case LLCFT_INFO:
			cmdmask = LLC_CMDMASK(frame_kind, cmdrsp);
			break;
		case LLCFT_RR:
			cmdmask = LLC_CMDMASK(frame_kind, cmdrsp);
			break;
		case LLCFT_RNR:
			cmdmask = LLC_CMDMASK(frame_kind, cmdrsp);
			break;
		case LLCFT_REJ:
			cmdmask = LLC_CMDMASK(frame_kind, cmdrsp);
			break;
		case LLCFT_DM:
			cmdmask = LLC_CMDMASK(frame_kind, cmdrsp);
			break;
		case LLCFT_UA:
			cmdmask = LLC_CMDMASK(frame_kind, cmdrsp);
			break;
		case LLCFT_FRMR:
			cmdmask = LLC_CMDMASK(frame_kind, cmdrsp);
			break;
		case LLC_INVALID_NR:
			cmdmask = LLC_CMDMASK(frame_kind, cmdrsp);
			break;
		case LLC_INVALID_NS:
			cmdmask = LLC_CMDMASK(frame_kind, cmdrsp);
			break;
		}
		break;
	}
	return (cmdmask);
}

int
llc_state_adm(struct llc_linkcb *linkp, struct llc *frame, int frame_kind, int cmdrsp, int pollfinal)
{
	int action, cmdmask;

	action = 0;
	cmdmask = llc_command(frame_kind, cmdrsp);
	switch (cmdmask) {
	case NL_CONNECT_REQUEST:
		llc_send(linkp, LLCFT_SABME, LLC_CMD, pollfinal);
		LLC_SETFLAG(linkp, P, pollfinal);
		LLC_SETFLAG(linkp, S, 0);
		linkp->llcl_retry = 0;
		LLC_NEWSTATE(linkp, setup);
		break;
	case LLCFT_SABME:
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
	case LLCFT_DISC:
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
	int action, cmdmask;

	action = 0;
	cmdmask = llc_command(frame_kind, cmdrsp);
	switch (cmdmask) {
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
	case LLCFT_SABME:
		LLC_SETFLAG(linkp, F, pollfinal);
		break;
	case LLCFT_DM:
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
	int action, cmdmask;

	action = 0;
	cmdmask = llc_command(frame_kind, cmdrsp);
	switch (cmdmask) {
	case NL_RESET_REQUEST:
	case NL_DISCONNECT_REQUEST:
	case LLCFT_DM:
	case LLCFT_SABME:
	case LLCFT_DISC:
	}

	return (action);
}

int
llc_state_reset_check(struct llc_linkcb *linkp, struct llc *frame, int frame_kind, int cmdrsp, int pollfinal)
{
	int action, cmdmask;

	action = 0;
	cmdmask = llc_command(frame_kind, cmdrsp);
	switch (cmdmask) {
	case NL_RESET_RESPONSE:
	case NL_DISCONNECT_REQUEST:
	case LLCFT_DM:
	case LLCFT_SABME:
	case LLCFT_DISC:
	}
	return (action);
}

int
llc_state_setup(struct llc_linkcb *linkp, struct llc *frame, int frame_kind, int cmdrsp, int pollfinal)
{
	int action, cmdmask;

	action = 0;
	cmdmask = llc_command(frame_kind, cmdrsp);
	switch (cmdmask) {
	case LLCFT_SABME:
	case LLCFT_UA:
	case LLC_ACK_TIMER_EXPIRED:
	case LLCFT_DISC:
	case LLCFT_DM:
	}
	return (action);
}

int
llc_state_reset(struct llc_linkcb *linkp, struct llc *frame, int frame_kind, int cmdrsp, int pollfinal)
{
	int action, cmdmask;

	action = 0;
	cmdmask = llc_command(frame_kind, cmdrsp);
	switch (cmdmask) {
	case LLCFT_SABME:
	case LLCFT_UA:
	case LLC_ACK_TIMER_EXPIRED:
	case LLCFT_DISC:
	case LLCFT_DM:
	}
	return (action);
}

int
llc_state_dconn(struct llc_linkcb *linkp, struct llc *frame, int frame_kind, int cmdrsp, int pollfinal)
{
	int action, cmdmask;

	action = 0;
	cmdmask = llc_command(frame_kind, cmdrsp);
	switch (cmdmask) {
	case LLCFT_SABME:
	case LLCFT_UA:
	case LLCFT_DISC:
	case LLCFT_DM:
	case LLC_ACK_TIMER_EXPIRED:
	}
	return (action);
}

int
llc_state_error(struct llc_linkcb *linkp, struct llc *frame, int frame_kind, int cmdrsp, int pollfinal)
{
	int action, cmdmask;

	action = 0;
	cmdmask = llc_command(frame_kind, cmdrsp);
	switch (cmdmask) {
	case LLCFT_SABME:
	case LLCFT_DISC:
	case LLCFT_DM:
	case LLCFT_FRMR:
	case LLC_ACK_TIMER_EXPIRED:
	default:
	}
	return (action);
}

int
llc_state_core(struct llc_linkcb *linkp, struct llc *frame, int frame_kind, int cmdrsp, int pollfinal)
{
	int action, cmdmask;

	action = 0;
	cmdmask = llc_command(frame_kind, cmdrsp);
	switch (cmdmask) {
	case NL_DISCONNECT_REQUEST:
	case NL_RESET_REQUEST:
	case LLCFT_SABME:
		action = LLC_RESET_INDICATION_REMOTE;
		break;
	case LLCFT_DISC:
		action = LLC_DISCONNECT_INDICATION;
		break;
	case LLCFT_FRMR:
		action =  LLC_FRMR_RECEIVED;
		break;
	case LLCFT_DM:
		action = LLC_DISCONNECT_INDICATION;
		break;
	case LLC_INVALID_NR:
	case LLC_INVALID_NS:
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
	case LLCFT_UA:
	case LLC_BAD_PDU:
		action = LLC_FRMR_SENT;
		break;
	case LLC_P_TIMER_EXPIRED:
	case LLC_ACK_TIMER_EXPIRED:
	case LLC_REJ_TIMER_EXPIRED:
	case LLC_BUSY_TIMER_EXPIRED:
	default:
		action = LLC_FRMR_SENT;
		break;
	}
	return (action);
}

int
llc_state_normal(struct llc_linkcb *linkp, struct llc *frame, int frame_kind, int cmdrsp, int pollfinal)
{
	int action, cmdmask;

	action = LLC_PASSITON;
	cmdmask = llc_command(frame_kind, cmdrsp);
	switch (cmdmask) {
	case NL_DATA_REQUEST:
	case NL_INITIATE_PF_CYCLE:
	case LLC_LOCAL_BUSY_DETECTED:
	case LLC_INVALID_NS:
	case LLCFT_INFO:
	case LLCFT_RR:
	case LLCFT_RNR:
	case LLCFT_REJ:
	case LLC_P_TIMER_EXPIRED:
	case LLC_ACK_TIMER_EXPIRED:
	case LLC_BUSY_TIMER_EXPIRED:
	}
	if (action == LLC_PASSITON) {
		action = llc_state_core(linkp, frame, frame_kind, cmdrsp, pollfinal);
	}
	return (action);
}

int
llc_state_busy(struct llc_linkcb *linkp, struct llc *frame, int frame_kind, int cmdrsp, int pollfinal)
{
	int action, cmdmask;

	action = LLC_PASSITON;
	cmdmask = llc_command(frame_kind, cmdrsp);
	switch (cmdmask) {
	case NL_DATA_REQUEST:
	case LLC_LOCAL_BUSY_CLEARED:
	case LLC_INVALID_NS:
	case LLCFT_INFO:
	case LLCFT_RR:
	case LLCFT_RNR:
	case LLCFT_REJ:
	case NL_INITIATE_PF_CYCLE:
	case LLC_P_TIMER_EXPIRED:
	case LLC_ACK_TIMER_EXPIRED:
	case LLC_BUSY_TIMER_EXPIRED:
	case LLC_REJ_TIMER_EXPIRED:
	}
	if (action == LLC_PASSITON) {
		action = llc_state_core(linkp, frame, frame_kind, cmdrsp, pollfinal);
	}
	return (action);
}

int
llc_state_reject(struct llc_linkcb *linkp, struct llc *frame, int frame_kind, int cmdrsp, int pollfinal)
{
	int action, cmdmask;

	action = 0;
	cmdmask = llc_command(frame_kind, cmdrsp);
	switch (cmdmask) {
	case NL_DATA_REQUEST:
	case NL_LOCAL_BUSY_DETECTED:
	case LLC_INVALID_NS:
	case LLCFT_INFO:
	case LLCFT_RR:
	case LLCFT_RNR:
	case LLCFT_REJ:
	case NL_INITIATE_PF_CYCLE:
	case LLC_REJ_TIMER_EXPIRED:
	case LLC_ACK_TIMER_EXPIRED:
	case LLC_BUSY_TIMER_EXPIRED:
	}
	return (action);
}

int
llc_state_await(struct llc_linkcb *linkp, struct llc *frame, int frame_kind, int cmdrsp, int pollfinal)
{
	int action, cmdmask;

	action = 0;
	cmdmask = llc_command(frame_kind, cmdrsp);
	switch (cmdmask) {
	case LLC_LOCAL_BUSY_DETECTED:
	case LLC_INVALID_NS:
	case LLCFT_INFO:
	case LLCFT_RR:
	case LLCFT_REJ:
	case LLCFT_RNR:
	case LLC_P_TIMER_EXPIRED:
	}
	return (action);
}

int
llc_state_await_busy(struct llc_linkcb *linkp, struct llc *frame, int frame_kind, int cmdrsp, int pollfinal)
{
	int action, cmdmask;

	action = 0;
	cmdmask = llc_command(frame_kind, cmdrsp);
	switch (cmdmask) {
	case LLC_LOCAL_BUSY_CLEARED:
	case LLC_INVALID_NS:
	case LLCFT_INFO:
	case LLCFT_RR:
	case LLCFT_REJ:
	case LLCFT_RNR:
	case LLC_P_TIMER_EXPIRED:
	}
	return (action);
}

int
llc_state_await_reject(struct llc_linkcb *linkp, struct llc *frame, int frame_kind, int cmdrsp, int pollfinal)
{
	int action, cmdmask;

	action = LLC_PASSITON;
	cmdmask = llc_command(frame_kind, cmdrsp);
	switch (cmdmask) {
	case LLC_LOCAL_BUSY_DETECTED:
	case LLC_INVALID_NS:
	case LLCFT_INFO:
	case LLCFT_RR:
	case LLCFT_REJ:
	case LLCFT_RNR:
	case LLC_P_TIMER_EXPIRED:
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

char *timer_names[] = {"ACK", "P", "BUSY", "REJ", "AGE"};

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
	CHECK(linkp, dconn);
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
