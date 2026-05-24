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
 *	@(#)tp_pcb.h	8.2 (Berkeley) 9/22/94
 */

#include <netiso/tp_protosw.h>

#include <netiso/tp_proto/tp_ip.h>
#include <netiso/tp_proto/tp_ip6.h>
#include <netiso/tp_proto/tp_iso.h>
#include <netiso/tp_proto/tp_ns.h>

struct tp_protosw tp_protosw[] = {
		{	/* INET 0 */
				&tpin4_protosw
		},
		{	/* INET6 1 */
				&tpin6_protosw
		},
		{	/* ISO 2 */
				&tpiso_protosw
		},
		{	/* XNS 3 */
				&tpns_protosw
		},
		{	/* ISO TPCONS 4 */
				&tpcons_protosw
		},
};

struct tp_xsap tp_xsap;

static void tp_protosw_attach(struct tp_xsap *);
static void tp_protosw_detach(struct tp_xsap *);

/* init function for the new attach and detach callback functions */
void
tp_protosw_init(void)
{
	int sid, af;

	for (sid = 0; sid < SAP_TABLE_MAX; sid++) {
		af = sap_select_sid_to_af(sid);
		if (af != 0) {
			tp_protosw_attach(&tp_xsap);
		} else {
			tp_protosw_detach(&tp_xsap);
		}
	}
}

static void
tp_protosw_attach(struct tp_xsap *xsap)
{
	int i;
	int len;

	len = (sizeof(tp_protosw)/sizeof(tp_protosw[0]));
	for (i = 0; i < len; i++) {
		(*tp_protosw[i].tp_xsap_attach)(xsap);
	}
}

static void
tp_protosw_detach(struct tp_xsap *xsap)
{
	int i;
	int len;

	len = (sizeof(tp_protosw)/sizeof(tp_protosw[0]));
	for (i = 0; i < len; i++) {
		(*tp_protosw[i].tp_xsap_detach)(xsap);
	}
}

void
tp_xsap_attach(struct tp_xsap *xsap, int af)
{
	nsap_attach(&xsap->tx_nsap, af);
	tsap_attach(&xsap->tx_tsap, &xsap->tx_nsap, af);
	ssap_attach(&xsap->tx_ssap, &xsap->tx_tsap, af);
	psap_attach(&xsap->tx_psap, &xsap->tx_ssap, af);
}

void
tp_xsap_detach(struct tp_xsap *xsap, int af)
{
	nsap_detach(&xsap->tx_nsap, af);
	tsap_detach(&xsap->tx_tsap, &xsap->tx_nsap, af);
	ssap_detach(&xsap->tx_ssap, &xsap->tx_tsap, af);
	psap_detach(&xsap->tx_psap, &xsap->tx_ssap, af);
}
