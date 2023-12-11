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

/*
 * NPF network buffer management interface.
 *
 * Network buffer in NetBSD is mbuf.  Internal mbuf structures are
 * abstracted within this source.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_mbuf.c,v 1.6.14.3 2013/02/08 19:18:10 riz Exp $");

#include <sys/param.h>
#include <sys/mbuf.h>

#include "nbpf.h"

size_t
nbpf_offset(struct mbuf *m)
{
	const u_int off;
	const int poff;

	off = (uintptr_t)mtod(m, uintptr_t);
	poff = m_length(m) + off;
	return (poff);
}

/*
 * nbpf_ensure_contig: check whether the specified length from the current
 * point in the nbuf is contiguous.  If not, rearrange the chain to be so.
 *
 * => Returns pointer to the data at the current offset in the buffer.
 * => Returns NULL on failure and nbuf becomes invalid.
 */
uint32_t
nbpf_ensure_contig(struct mbuf *m, size_t len)
{
	struct mbuf *m0;
	uint32_t ret;
	size_t off = mtod(m, uintptr_t);

	if (__predict_false(m->m_len < (off + len))) {
		size_t foff;
		size_t mlen;
		mlen = m->m_len;
		KASSERT((m->m_flags & M_PKTHDR) != 0);
		m0 = m_pullup(m, len);
		foff = m_length(m0) - m_length(m) + off;
		KASSERT(m != NULL);
		if (m == m0 && m->m_len == mlen) {
			return (0);
		}
		KASSERT((m->m_flags & M_PKTHDR) != 0);
		m0 = m;
		ret = mtod(m, u_char *) + foff;
	}
	return (ret);
}

/*
 * nbpf_advance: advance in nbuf or chain by specified amount of bytes and,
 * if requested, ensure that the area *after* advance is contiguous.
 *
 * => Returns new pointer to data in nbuf or NULL if offset is invalid.
 * => Current nbuf and the offset is stored in the nbuf metadata.
 */
uint32_t
nbpf_advance(struct mbuf *m, size_t len)
{
	uint32_t ret;
	u_int off, wmark;

	off = mtod(m, u_char *) + len;
	wmark = m->m_len;

	while (__predict_false(wmark <= off)) {
		m = m->m_next;
		if (__predict_false(m == NULL)) {
			return (0);
		}
		wmark += m->m_len;
	}
	KASSERT(off < m_length(m));
	ret = mtod(m, u_char *);
	KASSERT(off >= (wmark - m->m_len));
	ret += (off - (wmark - m->m_len));
	ret = nbpf_ensure_contig(m, len);
	return (ret);
}

bool_t
nbpf_cksum_barrier(struct mbuf *m, int di)
{
    if (di != PFIL_OUT) {
		return false;
	}
	KASSERT((m->m_flags & M_PKTHDR) != 0);

	if (m->m_pkthdr.csum_flags & (M_CSUM_TCPv4 | M_CSUM_UDPv4)) {
		in_delayed_cksum(m);
		m->m_pkthdr.csum_flags &= ~(M_CSUM_TCPv4 | M_CSUM_UDPv4);
		return true;
	}
	return false;
}

/*
 * nbuf_add_tag: add a tag to specified network buffer.
 *
 * => Returns 0 on success or errno on failure.
 */
int
nbpf_add_tag(struct mbuf *m, uint32_t key, uint32_t val)
{
    struct m_tag *mt;
    uint32_t *dat;

    KASSERT((m->m_flags & M_PKTHDR) != 0);

    mt = m_tag_get(PACKET_TAG_NONE, sizeof(uint32_t), M_NOWAIT);
    if (mt == NULL) {
		return ENOMEM;
	}
	dat = (uint32_t *)(mt + 1);
	*dat = val;
	m_tag_prepend(m, mt);
    return (0);
}

/*
 * nbuf_find_tag: find a tag in specified network buffer.
 *
 * => Returns 0 on success or errno on failure.
 */
int
nbpf_find_tag(struct mbuf *m, int tag, uint32_t key, void **data)
{
    struct m_tag *mt;

	KASSERT((m->m_flags & M_PKTHDR) != 0);

    mt = m_tag_find(m, tag, NULL);
	if (mt == NULL) {
		return EINVAL;
	}
	*data = (void *)(mt + 1);
	return 0;
}

/*
 * npf_addr_mask: apply the mask to a given address and store the result.
 */
void
nbpf_addr_mask(const nbpf_addr_t *addr, const nbpf_netmask_t mask, const int alen, nbpf_addr_t *out)
{
	const int nwords = alen >> 2;
	uint_fast8_t length = mask;

	/* Note: maximum length is 32 for IPv4 and 128 for IPv6. */
	KASSERT(length <= NPF_MAX_NETMASK);

	for (int i = 0; i < nwords; i++) {
		uint32_t wordmask;

		if (length >= 32) {
			wordmask = htonl(0xffffffff);
			length -= 32;
		} else if (length) {
			wordmask = htonl(0xffffffff << (32 - length));
			length = 0;
		} else {
			wordmask = 0;
		}
		out->s6_addr32[i] = addr->s6_addr32[i] & wordmask;
	}
}

/*
 * npf_addr_cmp: compare two addresses, either IPv4 or IPv6.
 *
 * => Return 0 if equal and negative/positive if less/greater accordingly.
 * => Ignore the mask, if NPF_NO_NETMASK is specified.
 */
int
nbpf_addr_cmp(const nbpf_addr_t *addr1, const nbpf_netmask_t mask1, const nbpf_addr_t *addr2, const nbpf_netmask_t mask2, const int alen)
{
	nbpf_addr_t realaddr1, realaddr2;

	if (mask1 != NBPF_NO_NETMASK) {
		nbpf_addr_mask(addr1, mask1, alen, &realaddr1);
		addr1 = &realaddr1;
	}
	if (mask2 != NBPF_NO_NETMASK) {
		nbpf_addr_mask(addr2, mask2, alen, &realaddr2);
		addr2 = &realaddr2;
	}
	return memcmp(addr1, addr2, alen);
}
