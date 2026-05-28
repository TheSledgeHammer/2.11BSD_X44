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

#include <sys/queue.h>
#include "devel/net/nettpi/tpi_events.h"
#include "devel/net/nettpi/tpi_tpkt.h"

struct tsap_ref {
	struct nsap_iso tf_nsap;
	present;
	selectlen;
};

struct tsap_packet {
	struct tpkt *tsp_pkt;
	tsp_code
};

#define TPDU_CODE(tpdu) ((tpdu)->_tpduv.fd_pcode & DT_TPDU)

struct tsap_blk {
	CIRCLEQ_ENTRY(tsap_blk) tb_queue;

	struct tsap_iso *tb_called;
	struct tsap_iso *tb_calling;

	struct tsap_ref tb_initiator;
	struct tsap_ref tb_responder;

	//socket??, sockaddr??

	struct tpkt *tb_pkt;
	struct tp_event *tb_event;

	//reason, cdt, seq, data, datalen, class
};

struct tsap_blkqueue;
CIRCLEQ_HEAD(tsap_blkqueue, tsap_blk);

struct tsap_blkqueue tsapblkqueue;

void
tsap_blk_init(struct tsap_blkqueue *tblkqueue)
{
	CIRCLEQ_INIT(tblkqueue);
}

struct tsap_blk *
tsap_blk_create(size_t size)
{
	struct tsap_blk *tblk;

	tblk = (struct tsap_blk *)malloc(size, M_ISOSAP, M_WAITOK);
	if (tblk == NULL) {
		return (NULL);
	}

	CIRCLEQ_INSERT_HEAD(&tsapblkqueue, tblk, tb_queue);
	return (tblk);
}

tsap_blk_lookup()
{
	struct tsap_blk *tblk;

	CIRCLEQ_FOREACH(tblk, &tsapblkqueue, tb_queue) {

	}
}

tsap_connect_request(struct tsap_iso *calling, struct tsap_iso *called)
{
	struct tsap_blk *tblk;

	tblk = tsap_blk_create(sizeof(struct tsap_blk *));
	if (tblk == NULL) {
		tsap_disconnect();
	}
	if (calling == NULL) {
		struct tsap_iso *tiso;
		struct nsap_iso *niso;
		nsap_iso_attach(niso);
		tsap_iso_attach(tiso, niso);
		calling = tiso;
	}
	tblk->tb_calling = calling;

	tsap_iso_compare(calling, called);
	tsap_connect();

}

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
