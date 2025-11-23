/*
 * The 3-Clause BSD License:
 * Copyright (c) 2025 Martin Kelly
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/mbuf.h>
#include <sys/queue.h>
#include <sys/null.h>

#include <netiso/tp_events.h>
#include <netiso/tp_param.h>

#include "tpi_tpdu.h"
#include "tpdu_var.h"

void
tpduv_variable_add(struct tpdu *tpdu, struct mbuf *m, unsigned char type, unsigned char reglen, unsigned char xtdlen, unsigned char class, unsigned char maxlength)
{
	tpdu->_tpduv = mtod(m, struct tpdu_variable *);
	tpdu->_tpduv->ve_type = type;
	tpdu->_tpduv->ve_class = class;
	tpdu->_tpduv->ve_rf_length = reglen;
	tpdu->_tpduv->ve_xf_length = xtdlen;
	tpdu->_tpduv->ve_max_length = maxlength;
    LIST_INSERT_HEAD(&tpdu->_tpduvh, tpdu->_tpduv, ve_link);
}

/*
 * Derived from tpdu_info[][4] in tp_input.c
 */
void
tpdu_variable_init(struct tpdu *tpdu, struct mbuf *m)
{
	LIST_INIT(&tpdu->_tpduvh);

	tpduv_variable_add(tpdu, m, XPD_TPDU_type, 0x5, 0x8, 0x0, TP_MAX_XPD_DATA);
	tpduv_variable_add(tpdu, m, XAK_TPDU_type, 0x5, 0x8, 0x0, 0x0);
	tpduv_variable_add(tpdu, m, GR_TPDU_type, 0x0, 0x0, 0x0, 0x0);
	tpduv_variable_add(tpdu, m, AK_TPDU_type, 0x5, 0xa, 0x0, 0x0);
	tpduv_variable_add(tpdu, m, ER_TPDU_type, 0x5, 0x5, 0x0, 0x0);
	tpduv_variable_add(tpdu, m, DR_TPDU_type, 0x7, 0x7, 0x7, TP_MAX_DR_DATA);
	tpduv_variable_add(tpdu, m, DC_TPDU_type, 0x6, 0x6, 0x0, 0x0);
	tpduv_variable_add(tpdu, m, CC_TPDU_type, 0x7, 0x7, 0x7, TP_MAX_CC_DATA);
	tpduv_variable_add(tpdu, m, CR_TPDU_type, 0x7, 0x7, 0x7, TP_MAX_CR_DATA);
	tpduv_variable_add(tpdu, m, DT_TPDU_type, 0x5, 0x8, 0x3, 0x0);
	/*
	 * Missing RJ_TPDU_TYPE. As this was not originally available.
	 * As well as not being present into tp_driver.
	 */
}

/*
 * Search for tpdu variables by tpdu_type
 */
struct tpdu_variable *
tpdu_variable_lookup(struct tpdu *tpdu, unsigned char type)
{
	LIST_FOREACH(tpdu->_tpduv, &tpdu->_tpduvh, ve_link) {
		if (tpdu->_tpduv->ve_type == type) {
			return (tpdu->_tpduv);
		}
	}
	return (NULL);
}

#define TPDU_COMMAND(tpdu_kind, cmd) ((tpdu_kind) + (cmd))

/*
 * State Handler Methods:
 * - adm, conn, reset, setup, d_conn, error, busy, reject, await
 */

int
tpdu_statehandler(struct tpipcb *tp, struct tp_event *evnt, int tpdu_kind, int cmdrsp, int poll)
{
    register int action = 0;

	switch (action) {
	case TPDU_CONNECT_INDICATION:

	case TPDU_CONNECT_CONFIRM:

	case TPDU_DISCONNECT_INDICATION:

	case TPDU_DATA_INDICATION:

	case TPDU_XPD_DATA_INDICATION:

	case TPDU_RESET_CONFIRM:

	case TPDU_DATA_RECEIVED:

	case TPDU_XPD_DATA_RECEIVED:

	case TPDU_ACCEPT_REQUEST:
	}
	return (action);
}


tpdu_state_timer(int tpdu_timer)
{
	switch (tpdu_timer) {
	case TPDU_INACT:
	case TPDU_RETRANS:
	case TPDU_SENDACK:
	case TPDU_NOTUSED:
	case TPDU_REFERENCE:
	case TPDU_DATA_RETRANS:
	}
}

tpdu_state_types(int tpdu_kind)
{
	switch (tpdu_kind) {
	case TPDUT_CC:
	case TPDUT_CR:
	case TPDUT_ER:
	case TPDUT_DR:
	case TPDUT_DC:
	case TPDUT_AK:
	case TPDUT_DT:
	case TPDUT_XPD:
	case TPDUT_XAK:
	case TPDUT_RJ:
	}
}


tpdu_state_class(int tpdu_class, int tpdu_kind)
{
	switch (tpdu_class) {
	case TPDU_CLASS0:
		/* Not valid in TP0 */
		if ((tpdu_kind == (TPDUT_DC | TPDUT_AK | TPDUT_XPD | TPDUT_XAK | TPDUT_RJ))) {
			break;
		}
		break;
	case TPDU_CLASS1:
		/* Not valid in TP1:
		 * - with receipt confirmation option enabled
		 */
		int confirm = 0;
		if ((tpdu_kind == (TPDUT_AK) && confirm == 0)) {
			break;
		}
		break;
	case TPDU_CLASS2:
		/* Not valid in TP2:
		 * - with explicit flow control option enabled
		 */
		int flow_control = 0;
		if ((tpdu_kind == TPDUT_RJ) || (tpdu_kind == (TPDUT_AK | TPDUT_XPD | TPDUT_XAK) && (flow_control == 0))) {
			break;
		}
		break;
	case TPDU_CLASS3:
	case TPDU_CLASS4:
		/* Not valid in TP4: */
		if ((tpdu_kind == TPDUT_RJ)) {
			break;
		}
		break;
	}
}
