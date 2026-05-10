/*
 * The 3-Clause BSD License:
 * Copyright (c) 2026 Martin Kelly
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

#include "devel/net/nettpi/tpi_events.h"

void
tp_event_set_tpdu(struct tpi_event *event, int tpdu_event_kind,
		struct mbuf *edata, int edatalen, unsigned int ecdt,
		unsigned short esref, SeqNum eseq, SeqNum esubseq, unsigned char efcc,
		unsigned char ereason) {

	switch (tpdu_event_kind) {
	case ER_TPDU:
		EV_SET(event, ER_TPDU, reason, ereason);
		break;

	case CR_TPDU:
		EV_SET(event, CR_TPDU, data, edata);
		EV_SET(event, CR_TPDU, datalen, edatalen);
		EV_SET(event, CR_TPDU, cdt, ecdt);
		break;

	case CC_TPDU:
		EV_SET(event, CC_TPDU, data, edata);
		EV_SET(event, CC_TPDU, datalen, edatalen);
		EV_SET(event, CC_TPDU, sref, esref);
		EV_SET(event, CC_TPDU, cdt, ecdt);
		break;

	case DR_TPDU:
		EV_SET(event, DR_TPDU, data, edata);
		EV_SET(event, DR_TPDU, datalen, edatalen);
		EV_SET(event, DR_TPDU, sref, esref);
		EV_SET(event, DR_TPDU, reason, ereason);
		break;

	case DC_TPDU:
		EV_SET(event, DC_TPDU, data, edata);
		EV_SET(event, DC_TPDU, datalen, edatalen);
		EV_SET(event, DC_TPDU, sref, esref);
		EV_SET(event, DC_TPDU, reason, ereason);
		break;

	case AK_TPDU:
		EV_SET(event, AK_TPDU, cdt, ecdt);
		EV_SET(event, AK_TPDU, seq, eseq);
		EV_SET(event, AK_TPDU, subseq, esubseq);
		EV_SET(event, AK_TPDU, fcc_present, efcc);
		break;

	case DT_TPDU:
		EV_SET(event, DT_TPDU, data, edata);
		EV_SET(event, DT_TPDU, datalen, edatalen);
		EV_SET(event, DT_TPDU, eot, ecdt);
		EV_SET(event, DT_TPDU, seq, eseq);
		break;

	case XPD_TPDU:
		EV_SET(event, XPD_TPDU, data, edata);
		EV_SET(event, XPD_TPDU, datalen, edatalen);
		EV_SET(event, XPD_TPDU, seq, eseq);
		break;

	case XAK_TPDU:
		EV_SET(event, AK_TPDU, seq, eseq);
		break;

	case RJ_TPDU:
		EV_SET(event, AK_TPDU, seq, eseq);
		break;
	}
}

void
tp_event_set_other(struct tpi_event *event, int tpdu_event_kind,
		SeqNum elow, SeqNum ehigh, int eretrans, unsigned char ereason)
{
	switch (tpdu_event_kind) {
	case TM_reference:
		EV_SET(event, TM_reference, low, elow);
		EV_SET(event, TM_reference, high, ehigh);
		EV_SET(event, TM_reference, retrans, eretrans);
		break;
	case TM_data_retrans:
		EV_SET(event, TM_data_retrans, low, elow);
		EV_SET(event, TM_data_retrans, high, ehigh);
		EV_SET(event, TM_data_retrans, retrans, eretrans);
		break;
	case T_DISC_req:
		EV_SET(event, T_DISC_req, reason, ereason);
		break;
	}
}
