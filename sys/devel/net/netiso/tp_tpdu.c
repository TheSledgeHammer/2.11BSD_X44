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

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/queue.h>

#include <netiso/tp_events.h>
#include <netiso/tp_param.h>
#include <netiso/tp_tpdu.h>

struct tpdu_info_head tp_info_list = LIST_HEAD_INITIALIZER(tp_info_list);

/*
 * tpdu info add
 */
/*
 * Derived from tpdu_info[][4] in tp_input.c
 */
void
tpdu_info_init(struct tpdu *tpdu)
{
	tpdu_info_add(tpdu, XPD_TPDU_type, 0x5, 0x8, 0x0, TP_MAX_XPD_DATA);
	tpdu_info_add(tpdu, XAK_TPDU_type, 0x5, 0x8, 0x0, 0x0);
	tpdu_info_add(tpdu, GR_TPDU_type, 0x0, 0x0, 0x0, 0x0);
	tpdu_info_add(tpdu, AK_TPDU_type, 0x5, 0xa, 0x0, 0x0);
	tpdu_info_add(tpdu, ER_TPDU_type, 0x5, 0x5, 0x0, 0x0);
	tpdu_info_add(tpdu, DR_TPDU_type, 0x7, 0x7, 0x7, TP_MAX_DR_DATA);
	tpdu_info_add(tpdu, DC_TPDU_type, 0x6, 0x6, 0x0, 0x0);
	tpdu_info_add(tpdu, CC_TPDU_type, 0x7, 0x7, 0x7, TP_MAX_CC_DATA);
	tpdu_info_add(tpdu, CR_TPDU_type, 0x7, 0x7, 0x7, TP_MAX_CR_DATA);
	tpdu_info_add(tpdu, DT_TPDU_type, 0x5, 0x8, 0x3, 0x0);
	/*
	 * Missing RJ_TPDU_TYPE. As this was not originally available.
	 * As well as not being present in tp_driver.
	 */
}

/*
 * tpdu info add:
 * Parameters:
 * - tpdu: the tpdu in question
 * - type: tpdu type
 * - reglen: regular format length
 * - xtdlen: extended format length
 * - class: tpdu class (currently: 0 or 4)
 * - maxlength: max length
 */
void
tpdu_info_add(struct tpdu *tpdu, unsigned char type, unsigned char reglen, unsigned char xtdlen, unsigned char class, unsigned char maxlength)
{
	struct tpdu_info *ti;

	ti = (struct tpdu_info *)malloc(sizeof(*ti), M_PCB, M_WAITOK);
	if (ti == NULL) {
		return;
	}
	ti->ti_tpdu = tpdu;
	ti->ti_type = type;
	ti->ti_class = class;
	ti->ti_rf_length = reglen;
	ti->ti_xf_length = xtdlen;
	ti->ti_max_length = maxlength;
	LIST_INSERT_HEAD(&tp_info_list, ti, ti_link);
}

/*
 * tpdu info remove
 */
void
tpdu_info_remove(struct tpdu *tpdu, unsigned char type)
{
	struct tpdu_info *ti;

	ti = tpdu_info_lookup(tpdu, type);
	if (ti != NULL) {
		LIST_REMOVE(ti, ti_link);
	}
}

/*
 * tpdu info lookup
 */
struct tpdu_info *
tpdu_info_lookup(struct tpdu *tpdu, unsigned char type)
{
	struct tpdu_info *ti;

	LIST_FOREACH(ti, &tp_info_list, ti_link) {
		if ((ti->ti_tpdu == tpdu) && (ti->ti_type == type)) {
			return (ti);
		}
	}
	return (NULL);
}

#ifdef notyet

#define CHECK(Phrase, Erval, Stat, Whattodo, Loc) \
	if (Phrase) { \
		error = (Erval); \
		errlen = (int)(Loc); \
		IncStat(Stat); \
		goto Whattodo; \
	}


#define WHILE_OPTIONS(P, hdr, format) \
		caddr_t P; \
		caddr_t PLIM; \
		struct tpdu_info *info; \
		info = tpdu_info_lookup((hdr), (hdr)->tpdu_type); \
		if (info != NULL) { \
			P = info + (hdr); \
			PLIM = 1 + (hdr)->tpdu_li + (hdr); \
			struct tp_vbp *tpv = (struct tp_vbp *)P; \
			for (;; P += 2 + tpv->tpv_len) { \
				CHECK((P > PLIM), E_TP_LENGTH_INVAL, ts_inv_length, respond, P - (caddr_t)hdr); \
				if (P == PLIM) { \
					break; \
				} \
			} \
		}

#endif
