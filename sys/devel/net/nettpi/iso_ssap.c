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
 * Work in Progress:
 * - psap's and ssap's will eventually be in seperate source files.
 */

#include "iso_nsap.h"

/* SSAP's */
void
ssap_init(struct ssap_iso *ssap, struct tsap_iso tsap[ISOLEN], int sid, int af)
{
	ssap->ssi_tsaps = tsap;
	ssap->ssi_sid = sid;
	ssap->ssi_af = af;
	sap_select_init(&ssap->ssi_select, sid, af);
}

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

/* PSAP's */
void
psap_init(struct psap_iso *psap, struct ssap_iso ssap[ISOLEN], int sid, int af)
{
	psap->psi_ssaps = ssap;
	psap->psi_sid = sid;
	psap->psi_af = af;
	sap_select_init(&psap->psi_select, sid, af);
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
