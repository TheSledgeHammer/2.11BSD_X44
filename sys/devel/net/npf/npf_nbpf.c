/*	$NetBSD: npf_mbuf.c,v 1.6.14.3 2013/02/08 19:18:10 riz Exp $	*/

/*-
 * Copyright (c) 2009-2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This material is based upon work partially supported by The
 * NetBSD Foundation under a contract with Mindaugas Rasiukevicius.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/mbuf.h>

#include <devel/net/nbpf/nbpf.h>
#include <devel/net/nbpf/nbpf_ncode.h>

#include "npf_impl.h"

/*
 * NPF compatibility routines with NBPF
 */

void *
nbuf_advance(nbuf_t *nbuf, size_t len, size_t ensure)
{
	struct mbuf *m = nbuf->nb_mbuf;
	uint8_t *d;

	d = nbpf_advance(&nbuf, nbuf->nb_nptr, len);
	if (ensure) {
		/* Ensure contiguousness (may change nbuf chain). */
		d = nbuf_ensure_contig(nbuf, ensure);
	}
	return (d);
}

void *
nbuf_ensure_contig(nbuf_t *nbuf, size_t len)
{
	const struct mbuf * const n = nbuf->nb_mbuf;
	const size_t off = (uintptr_t)nbuf->nb_nptr - mtod(n, uintptr_t);
	int error;

	if (nbpf_fetch_datum(n, nbuf->nb_nptr, len, off)) {
		KASSERT(off < n->m_len);
		if (__predict_false(n->m_len < (off + len))) {
			struct mbuf *m, *m0;
			const size_t foff;
			const size_t plen;
			const size_t mlen;
			size_t target;
			bool success;

			//npf_stats_inc(NPF_STAT_NBUF_NONCONTIG);

			/* Attempt to round-up to NBUF_ENSURE_ALIGN bytes. */
			if ((target = NBUF_ENSURE_ROUNDUP(foff + len)) > plen) {
				target = foff + len;
			}

			m0 = nbuf->nb_mbuf0;
			foff = nbuf_offset(nbuf);
			plen = m_length(m0);
			mlen = m0->m_len;

			/* Rearrange the chain to be contiguous. */
			KASSERT((m0->m_flags & M_PKTHDR) != 0);
			m = m_pullup(&m0, target);
			if (m != NULL && m0 != NULL) {
				if (m != m0 && m->m_len != mlen) {
					success = TRUE;
				} else {
					success = FALSE;
				}
			} else {
				success = FALSE;
			}
			KASSERT(m0 != NULL);

			/* If no change in the chain: return what we have. */
			if (m == m0 && m->m_len == mlen) {
				return success ? nbuf->nb_nptr : NULL;
			}

			/*
			 * The mbuf chain was re-arranged.  Update the pointers
			 * accordingly and indicate that the references to the data
			 * might need a reset.
			 */
			KASSERT((m->m_flags & M_PKTHDR) != 0);
			nbuf->nb_mbuf0 = m;
			nbuf->nb_mbuf = m;

			KASSERT(foff < m->m_len && foff < m_length(m));
			nbuf->nb_nptr = mtod(m, uint8_t *) + foff;
			nbuf->nb_flags |= NBUF_DATAREF_RESET;

			if (!success) {
				//npf_stats_inc(NPF_STAT_NBUF_CONTIG_FAIL);
				return NULL;
			}
		}
	}
	return (nbuf->nb_nptr);
}

/*
 * npf_cache_all: general routine to cache all relevant IP (v4 or v6)
 * and TCP, UDP or ICMP headers.
 *
 * => nbuf offset shall be set accordingly.
 */
int
npf_cache_all(nbpf_cache_t *npc, nbuf_t *nbuf)
{
	nbpf_state_t *state = (nbpf_state_t *)npc;

	return (nbpf_cache_all(state, nbuf));
}

void
npf_recache(nbpf_cache_t *npc, nbuf_t *nbuf)
{
	nbpf_state_t *state = (nbpf_state_t *)npc;

	nbpf_recache(state, nbuf);
}

/*
 * nbuf_add_tag: add a tag to specified network buffer.
 *
 * => Returns 0 on success or errno on failure.
 */
int
nbuf_add_tag(nbuf_t *nbuf, uint32_t key, uint32_t val)
{
	nbpf_state_t *state = (nbpf_state_t *)nbuf;

	nbpf_set_tag(state, PACKET_TAG_NPF);
	return (nbpf_add_tag(state, nbuf, key, val));
}

/*
 * nbuf_find_tag: find a tag in specified network buffer.
 *
 * => Returns 0 on success or errno on failure.
 */
int
nbuf_find_tag(nbuf_t *nbuf, uint32_t key, void **data)
{
	nbpf_state_t *state = (nbpf_state_t *)nbuf;

	return (nbpf_find_tag(state, nbuf, key, data));
}

int
npf_filter(nbpf_cache_t *npc, struct nbpf_insn *pc, nbuf_t *nbuf, int layer)
{
	nbpf_state_t *state = (nbpf_state_t *)npc;

	return (nbpf_filter(state, pc, nbuf, layer));
}

int
npf_validate(struct nbpf_insn *f, size_t len, int ret)
{
	return (nbpf_validate(f, len, ret));
}
