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

/*
 * TODO:
 * - rename: functions that are layer independent or have commonality between
 * each layer.
 * - most sap_select functions fall into this category.
 */
/*
 * NSAP & TSAP network stack dependent setup/initialization:
 * - will occur from a tp_protosw callback function. (labelled tppw_init)
 * - the tppw_init callback will run from within the tp_init function.
 */

#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/socket.h>
#include <sys/socketvar.h>

#include "if_sap.h"
#include "iso_nsap.h"

/* TSAP's */
static struct tsap_iso *
tsap_create(struct nsap_iso *nsap)
{
	struct tsap_iso *tsap;

	MALLOC(tsap, struct tsap_iso *, sizeof(*tsap), M_IFSAP, M_WAITOK);
	if (tsap == NULL) {
		return (NULL);
	}
	bzero((caddr_t)tsap, sizeof(*tsap));
	bcopy(nsap, tsap->tsi_nsaps, sizeof(tsap->tsi_nsaps));
	return (tsap);
}

static void
tsap_destroy(struct tsap_iso *tsap)
{
	if (tsap != NULL) {
		FREE(tsap, M_IFSAP);
	}
}

void
tsap_attach(struct tsap_iso *tsap, struct nsap_iso *nsap, int sid, int af)
{
	struct tsap_iso *tsiso;

	tsiso = tsap_create(nsap);
	if (tsiso != NULL) {
		sap_select_init(&tsiso->tsi_select, sid, af);
		tsap = tsiso;
	} else {
		tsap = NULL;
	}
}

void
tsap_detach(struct tsap_iso *tsap, struct nsap_iso *nsap, int sid, int af)
{
	if (tsap != NULL) {
		nsap_detach(nsap, sid, af);
		bcopy(nsap, tsap->tsi_nsaps, sizeof(tsap->tsi_nsaps));
		if (tsap->tsi_nsaps == NULL) {
			tsap_destroy(tsap);
		}
	}
}

/*
 * returns an nsap from tsap
 */
struct nsap_iso *
tsap_to_nsap(struct tsap_iso *tsap)
{
	struct nsap_iso *nsap;

	nsap = &tsap->tsi_nsaps;
	if (nsap != NULL) {
		return (nsap);
	}
	return (NULL);
}

void
tsap_iso_attach(struct tsap_iso *tsap, struct nsap_iso *nsap)
{
	tsap_attach(tsap, nsap, SAP_SID_ISO, AF_ISO);
	tsap_attach(tsap, nsap, SAP_SID_INET4, AF_INET);
	tsap_attach(tsap, nsap, SAP_SID_INET6, AF_INET6);
	tsap_attach(tsap, nsap, SAP_SID_NS, AF_NS);
}

void
tsap_iso_detach(struct tsap_iso *tsap, struct nsap_iso *nsap)
{
	tsap_detach(tsap, nsap, SAP_SID_ISO, AF_ISO);
	tsap_detach(tsap, nsap, SAP_SID_INET4, AF_INET);
	tsap_detach(tsap, nsap, SAP_SID_INET6, AF_INET6);
	tsap_detach(tsap, nsap, SAP_SID_NS, AF_NS);
}

/*
 * tsap_iso_compare:
 * - compares nsap_iso and sap_select
 * returns -1 if a, 1 if b and 0 if equal
 */
int
tsap_iso_compare(struct tsap_iso *a, struct tsap_iso *b)
{
	int error;

	if (a != b) {
		error = nsap_iso_compare(&a->tsi_nsaps, &b->tsi_nsaps);
		if (error != 0) {
			return (error);
		}
		error = sap_select_compare(&a->tsi_select, &b->tsi_select);
		if (error != 0) {
			return (error);
		}
	}
	return (0);
}
