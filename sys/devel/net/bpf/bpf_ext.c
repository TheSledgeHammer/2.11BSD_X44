/*-
 * Copyright (c) 2009-2010 The NetBSD Foundation, Inc.
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
#include <net/bpf.h>
#include <net/bpfdesc.h>

#include "bpf_ncode.h"
#include "bpf_mbuf.h"

int
bpf_ext_filter(bpc, pc, nbuf, buflen, layer)
	bpf_cache_t *bpc;
	struct bpf_insn_ext *pc;
	nbuf_t *nbuf;
	u_int buflen;
	const int layer;
{
	struct mbuf *m;
	size_t pktlen;
	int error;

	m = nbuf_head_mbuf(nbuf);
	pktlen = m_length(m);

	error =	bpf_filter(pc->insn, (unsigned char *)m, pktlen, buflen);
	if (error != 0) {
		return (error);
	}
	error = bpf_ncode_process(bpc, pc, nbuf, layer);
	if (error != 0) {
		return (error);
	}
	return (0);
}

int
bpf_ext_validate(f, len)
	struct bpf_insn_ext *f;
	size_t len;
{
	int error, errat;

	error = bpf_validate(f->insn, (len / sizeof(struct bpf_insn *)));
	if (error != 0) {
		return (error);
	}
	error = bpf_ncode_validate(f, len, &errat);
	if (error != 0) {
		return (error);
	}
	return (0);
}
