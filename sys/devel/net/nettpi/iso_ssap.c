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

#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/socket.h>
#include <sys/socketvar.h>

#include "iso_nsap.h"

/* SSAP's */
struct ssap_iso *
ssap_create(struct tsap_iso *tsap)
{
	struct ssap_iso *ssap;

	MALLOC(ssap, struct ssap_iso *, sizeof(*ssap), M_ISOSAP, M_WAITOK);
	if (ssap == NULL) {
		return (NULL);
	}
	bzero((caddr_t)ssap, sizeof(*ssap));
	bcopy(tsap, ssap->ssi_tsaps, sizeof(ssap->ssi_tsaps));
	return (ssap);
}

void
ssap_destroy(struct ssap_iso *ssap)
{
	if (ssap != NULL) {
		FREE(ssap, M_ISOSAP);
	}
}

void
ssap_attach(struct ssap_iso *ssap, struct tsap_iso *tsap, int af)
{
	struct ssap_iso *ssiso;
	int sid;

	ssiso = ssap_create(tsap);
	if (ssiso != NULL) {
		sid = sap_select_af_to_sid(af);
		sap_select_init(&ssiso->ssi_select, sid, af);
		ssap = ssiso;
	}
	ssap = NULL;
}

void
ssap_detach(struct ssap_iso *ssap, struct tsap_iso *tsap, int af)
{
	if (ssap != NULL) {
		tsap_detach(tsap, &tsap->tsi_nsaps, af);
		bcopy(tsap, ssap->ssi_tsaps, sizeof(ssap->ssi_tsaps));
		if (ssap->ssi_tsaps == NULL) {
			ssap_destroy(ssap);
		}
	}
}

/*
 * returns an tsap from ssap
 */
struct tsap_iso *
ssap_to_tsap(struct ssap_iso *ssap)
{
	struct tsap_iso *tsap;

	tsap = &ssap->ssi_tsaps;
	if (tsap != NULL) {
		return (tsap);
	}
	return (NULL);
}

/*
 * ssap_iso_compare:
 * - compares tsap_iso and sap_select
 * returns -1 if a, 1 if b and 0 if equal
 */
int
ssap_iso_compare(struct ssap_iso *a, struct ssap_iso *b)
{
	int error;

	if (a != b) {
		error = tsap_iso_compare(&a->ssi_tsaps, &b->ssi_tsaps);
		if (error != 0) {
			return (error);
		}
		error = sap_select_compare(&a->ssi_select, &b->ssi_select);
		if (error != 0) {
			return (error);
		}
	}
	return (0);
}

int
ssap_connect(struct ssap_iso *ssap, void *arg, int af)
{
	struct tsap_iso *tsap;
	int error;

	tsap = ssap_to_tsap(ssap);
	if (tsap == NULL) {
		return (EINVAL);
	}
	return (tsap_connect(tsap, arg, af));
}

int
ssap_disconnect(struct ssap_iso *ssap, void *arg, int af)
{
	struct tsap_iso *tsap;
	int error;

	tsap = ssap_to_tsap(ssap);
	if (tsap == NULL) {
		return (EINVAL);
	}
	return (tsap_disconnect(tsap, arg, af));
}
