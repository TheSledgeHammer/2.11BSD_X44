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

#include "iso_nsap.h"

struct tp_protosw {
	void (*tp_xsap_attach)(struct tp_xsap_router *, int);
	void (*tp_xsap_detach)(struct tp_xsap_router *, int);
};

struct tp_xsap_router tp_xsap_router;

void
tp_xsap_router_attach(struct tp_xsap_router *router, int af)
{
	nsap_attach(&router->txr_nsap, af);
	tsap_attach(&router->txr_tsap, &router->txr_nsap, af);
	ssap_attach(&router->txr_ssap, &router->txr_tsap, af);
	psap_attach(&router->txr_psap, &router->txr_ssap, af);
}

void
tp_xsap_router_detach(struct tp_xsap_router *router, int af)
{
	nsap_detach(&router->txr_nsap, af);
	tsap_detach(&router->txr_tsap, &router->txr_nsap, af);
	ssap_detach(&router->txr_ssap, &router->txr_tsap, af);
	psap_detach(&router->txr_psap, &router->txr_ssap, af);
}

void
tp_protosw_attach(struct tp_xsap_router *router, int af)
{
	int i;
	int len;

	len = (sizeof(tp_protosw)/sizeof(tp_protosw[0]));
	for (i = 0; i < len; i++) {
		(*tp_protosw[i].tp_xsap_attach)(router, af);
	}
}

void
tp_protosw_detach(struct tp_xsap_router *router, int af)
{
	int i;
	int len;

	len = (sizeof(tp_protosw)/sizeof(tp_protosw[0]));
	for (i = 0; i < len; i++) {
		(*tp_protosw[i].tp_xsap_detach)(router, af);
	}
}

/* init function for the new attach and detach callback functions */
void
tp_protosw_init(void)
{
	int sid, af;

	for (sid = 0; sid < SAP_TABLE_MAX; sid++) {
		af = sap_select_sid_to_af(sid);
		if (af != 0) {
			tp_protosw_attach(&tp_xsap_router, af);
		} else {
			tp_protosw_detach(&tp_xsap_router, af);
		}
	}
}

/*
 * tp protosw:
 * - new init function
 */
void
tpin_init(struct tsap_iso *tsap)
{
	tp_xsap_router_attach(tsap, AF_INET);
}

void
tpin6_init(struct tsap_iso *tsap)
{
	tp_xsap_router_attach(tsap, AF_INET6);
}

void
tpiso_init(struct tsap_iso *tsap)
{
	tp_xsap_router_attach(tsap, AF_ISO);
}

void
tpns_init(struct tsap_iso *tsap)
{
	tp_xsap_router_attach(tsap, AF_NS);
}

struct tp_protosw tp_protosw[] = {
		{
				&tpin4_protosw
		},
		{
				&tpin6_protosw
		},
		{
				&tpiso_protosw
		},
		{
				&tpns_protosw
		},
};
