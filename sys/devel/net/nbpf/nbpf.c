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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_mbuf.c,v 1.6.14.3 2013/02/08 19:18:10 riz Exp $");

#include <sys/param.h>
#include <sys/mbuf.h>

#include <net/bpf.h>
#include <net/bpfdesc.h>


#include "nbpf.h"
#include "nbpf_ncode.h"
#include "nbpfdesc.h"

#include <netinet/ip_var.h>
#include <netinet6/ip6_var.h>

int nbpf_setf(struct nbpf_d *, struct nbpf_program *, int *);

struct nbpf_d *
nbpf_d_alloc(size)
	size_t size;
{
	struct nbpf_d *nd;

	nd = (struct nbpf_d *)malloc(size, M_DEVBUF, M_DONTWAIT);
	return (nd);
}

void
nbpf_d_free(nd)
	struct nbpf_d *nd;
{
	if (nd != NULL) {
		free(nd, M_DEVBUF);
	}
}

static void
nbpf_table_init(nd)
	struct nbpf_d *nd;
{
	nbpf_tableset_t *tset;
	nbpf_table_t *t;
	int error;

	nbpf_tableset_init();

	error = nbpf_mktable(&tset, &t, NBPF_TABLE_TID, NBPF_TABLE_TYPE, NBPF_TABLE_HSIZE);
	if (error != 0) {
		return;
	}
	nd->nbd_tableset = tset;
	nd->nbd_table = t;
}

static void
nbpf_init(nd, layer, tag)
	struct nbpf_d *nd;
	int layer, tag;
{
	nbpf_state_t *state;

	state = (nbpf_state_t *)malloc(sizeof(*state), M_DEVBUF, M_DONTWAIT);
	nbpf_set_tag(state, tag);
	nd->nbd_state = state;
	nd->nbd_layer = layer;
}

void
nbpf_attachd(nd)
	struct nbpf_d *nd;
{
	nbpf_init(nd, NBPC_LAYER4, PACKET_TAG_NONE);
	nbpf_table_init(nd);
}

void
nbpf_detachd(nd)
	struct nbpf_d *nd;
{
	nbpf_tableset_fini();

	if (nd->nbd_state != NULL) {
		free(nd->nbd_state, M_DEVBUF);
	}
}

int
nbpfioctl(d, dev, cmd, addr, flag, p)
	struct nbpf_d *d;
	dev_t dev;
	u_long cmd;
	caddr_t addr;
	int flag;
	struct proc *p;
{
    struct nbpf_ioctl_table *nbiot;
    nbpf_tableset_t *tset;
	int error, ret;

	switch (cmd) {
	case BIOCSETF:
		error = nbpf_setf(d, (struct nbpf_program *) addr, &ret);
		if (ret != 0) {
			error = ret;
		}
		break;

	case BIOCSTBLF:
		nbiot = (struct nbpf_ioctl_table *)addr;
		tset = d->nbd_tableset;
		if (tset == NULL) {
			nbpf_table_init(d);
		}
		error = nbpf_table_ioctl(nbiot, tset);
		break;
	}
	return (error);
}

int
nbpf_setf(nd, fp, error)
	struct nbpf_d *nd;
	struct nbpf_program *fp;
	int *error;
{
	struct nbpf_insn *ncode, *old;
	u_int nlen, size;
	int s;

	old = nd->nbd_filter;
	if (fp->nbf_insns == 0) {
		if (fp->nbf_len != 0) {
			return (EINVAL);
		}
		s = splnet();
		nd->nbd_filter = 0;
		//reset_d(d);
		splx(s);
		if (old != 0) {
			free((caddr_t)old, M_DEVBUF);
		}
		return (0);
	}
	nlen = fp->nbf_len;

	size = nlen * sizeof(*fp->nbf_insns);
	ncode = (struct nbpf_insn *)malloc(size, M_DEVBUF, M_WAITOK);
	if (copyin((caddr_t)fp->nbf_insns, (caddr_t)ncode, size) == 0 && nbpf_validate(ncode, (int)nlen, error)) {
		s = splnet();
		nd->nbd_filter = ncode;
		//reset_d(d);
		splx(s);
		if (old != 0) {
			free((caddr_t)old, M_DEVBUF);
		}
		return (0);
	}
	free((caddr_t)ncode, M_DEVBUF);
	return (EINVAL);
}

void
nbpf_filtncatch(d, nd, pkt, pktlen, slen, cpfn)
    struct bpf_d *d;
	struct nbpf_d *nd;
	u_char *pkt;
	u_int pktlen;
	u_int slen;
	void *(*cpfn)(void *, const void *, size_t);
{
	slen = nbpf_filter(nd->nbd_state, nd->nbd_filter, (nbpf_buf_t *)pktlen, nd->nbd_layer);
	if (slen != 0) {
		//catchpacket(d, pkt, pktlen, slen, cpfn);
	}
}

static int
nbpf_check_cache(nbpf_state_t *state, nbpf_buf_t *nbuf, void *nptr)
{
	if (!nbpf_iscached(state, NBPC_IP46) &&
			!nbpf_fetch_ipv4(state, &state->nbs_ip4, nbuf, nptr) &&
			!nbpf_fetch_ipv6(state, &state->nbs_ip6, nbuf, nptr)) {
		return (state->nbs_info);
	}
	if (nbpf_iscached(state, NBPC_IPFRAG)) {
		return (state->nbs_info);
	}

	switch (nbpf_cache_ipproto(state)) {
	case IPPROTO_TCP:
		(void)nbpf_fetch_tcp(state, &state->nbs_port, nbuf, nptr);
		break;
	case IPPROTO_UDP:
		(void)nbpf_fetch_udp(state, &state->nbs_port, nbuf, nptr);
		break;
	case IPPROTO_ICMP:
        (void)nbpf_fetch_icmp(state, &state->nbs_icmp, nbuf, nptr);
        break;
	case IPPROTO_ICMPV6:
		(void)nbpf_fetch_icmp(state, &state->nbs_icmp, nbuf, nptr);
		break;
	}
	return (state->nbs_info);
}

/*
 * npf_cache_all: general routine to cache all relevant IP (v4 or v6)
 * and TCP, UDP or ICMP headers.
 */
int
nbpf_cache_all(nbpf_state_t *state, nbpf_buf_t *nbuf)
{
	void *nptr;

	nptr = nbpf_dataptr(nbuf);
	return (nbpf_check_cache(state, nbuf, nptr));
}

void
nbpf_recache(nbpf_state_t *state, nbpf_buf_t *nbuf)
{
	int mflags, flags;

	mflags = state->nbs_info & (NBPC_IP46 | NBPC_LAYER4);
	state->nbs_info = 0;
	flags = nbpf_cache_all(state, nbuf);
	KASSERT((flags & mflags) == mflags);
}

int
nbpf_reassembly(nbpf_state_t *state, nbpf_buf_t *nbuf, struct mbuf **mp)
{
	int error;

	if (nbpf_iscached(state, NBPC_IP4)) {
		struct ip *ip = nbpf_dataptr(nbuf);
		error = ip_reass_packet(mp, ip);
	} else if (nbpf_iscached(state, NBPC_IP6)) {
		error = ip6_reass_packet(mp, state->nbs_hlen);
		if (error && *mp == NULL) {
			memset(nbuf, 0, sizeof(nbpf_buf_t));
		}
	}
	if (error) {
		return (error);
	}
	if (*mp == NULL) {
		return (0);
	}
	state->nbs_info = 0;
	if (nbpf_cache_all(state, nbuf) & NBPC_IPFRAG) {
		return (EINVAL);
	}
	return (0);
}

int
nbpf_packet_handler(d, buf, mp)
	struct nbpf_d *d;
	void *buf;
	struct mbuf **mp;
{
	int error;

	if (nbpf_cache_all(d->nbd_state, buf) & NBPC_IPFRAG) {
		error = nbpf_reassembly(d->nbd_state, buf, mp);
		if (error) {
			goto out;
		}
		if (*mp == NULL) {
			return (0);
		}
	}
out:
	if (!error) {
		error = ENETUNREACH;
	}
	if (*mp) {
		m_freem(*mp);
		*mp = NULL;
	}
	return (error);
}
