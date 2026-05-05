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

/* PSAP's */

struct psap_iso *
psap_create(struct ssap_iso *ssap)
{
	struct psap_iso *psap;

	MALLOC(psap, struct psap_iso *, sizeof(*psap), M_ISOSAP, M_WAITOK);
	if (psap == NULL) {
		return (NULL);
	}
	bzero((caddr_t)psap, sizeof(*psap));
	bcopy(ssap, psap->psi_ssaps, sizeof(psap->psi_ssaps));
	return (psap);
}

void
psap_destroy(struct psap_iso *psap)
{
	if (psap != NULL) {
		FREE(psap, M_ISOSAP);
	}
}

void
psap_attach(struct psap_iso *psap, struct ssap_iso ssap[ISOLEN], int af)
{
	struct psap_iso *psiso;
	int sid;

	psiso = psap_create(ssap);
	if (psiso != NULL) {
		sid = sap_select_af_to_sid(af);
		sap_select_init(&psiso->psi_select, sid, af);
		psap = psiso;
	}
	psap = NULL;
}

void
psap_detach(struct psap_iso *psap, long type, long subnet, int af)
{
	if (psap != NULL) {
		ssap_detach(&psap->psi_ssaps, type, subnet, af);
		if (psap->psi_ssaps == NULL) {
			psap_destroy(psap);
		}
	}
}

struct ssap_iso *
psap_to_ssap(struct psap_iso *psap)
{
	struct ssap_iso *ssap;

	ssap = &psap->psi_ssaps;
	if (ssap != NULL) {
		return (ssap);
	}
	return (NULL);
}

/*
 * psap_iso_compare:
 * - compares ssap_iso and sap_select
 * returns -1 if a, 1 if b and 0 if equal
 */
int
psap_iso_compare(struct psap_iso *a, struct psap_iso *b)
{
	int error;

	if (a != b) {
		error = ssap_iso_compare(&a->psi_ssaps, &b->psi_ssaps);
		if (error != 0) {
			return (error);
		}
		error = sap_select_compare(&a->psi_select, &b->psi_select);
		if (error != 0) {
			return (error);
		}
	}
	return (0);
}
