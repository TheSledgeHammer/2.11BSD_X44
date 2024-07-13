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

/* TODO:
 * - catchpacket??
 */

struct nbpf_program {
	u_int 				nbf_len;
	struct nbpf_insn 	*nbf_insns; /* ncode */
};

struct nbpf_d {
	struct nbpf_d		*nbd_next;			/* Linked list of descriptors */

	/* nbpf tables */
	nbpf_tableset_t		*nbd_tableset;
	nbpf_table_t 		*nbd_table;

	u_long				nbd_rcount;			/* number of packets received */
	int					nbd_seesent;		/* true if bpf should see sent packets */
	/* nbpf ncode */
	nbpf_state_t		*nbd_state;
	struct nbpf_insn 	*nbd_filter;
	int 				nbd_layer;
};

struct nbpf_if {
	struct nbpf_if		*nbif_next;			/* list of all interfaces */
	struct nbpf_d 		*nbif_dlist;		/* descriptor list */
	struct nbpf_if 		**nbif_driverp;		/* pointer into softc */
	u_int 				nbif_dlt;			/* link layer type */
	u_int 				nbif_hdrlen;		/* length of header (with padding) */
	struct ifnet 		*nbif_ifp;			/* correspoding interface */
};

struct nbpf_d	nbpf_dtab[NNBPFILTER];

dev_type_open(nbpfopen);
dev_type_close(nbpfclose);
dev_type_read(nbpfread);
dev_type_write(nbpfwrite);
dev_type_ioctl(nbpfioctl);
dev_type_poll(nbpfpoll);
dev_type_kqfilter(nbpfkqfilter);

const struct cdevsw nbpf_cdevsw = {
		.d_open = nbpfopen,
		.d_close = nbpfclose,
		.d_read = nbpfread,
		.d_write = nbpfwrite,
		.d_ioctl = nbpfioctl,
		.d_stop = nostop,
		.d_tty = notty,
		.d_poll = nbpfpoll,
		.d_mmap = nommap,
		.d_kqfilter = nbpfkqfilter,
		.d_discard = nodiscard,
		.d_type = D_OTHER
};

void
nbpf_table_init(nd)
	struct nbpf_d *nd;
{
	nbpf_tableset_t tset;
	nbpf_table_t t;
	int error;

	nbpf_tableset_init();

	error = nbpf_mktable(&tset, &t, NBPF_TABLE_TID, NBPF_TABLE_TYPE, NBPF_TABLE_HSIZE);
	if (error != 0) {
		return;
	}
	nd->nbd_tableset = &tset;
	nd->nbd_table = &t;
}

void
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

static void
nbpf_attachd(nd)
	struct nbpf_d *nd;
{
	nbpf_init(nd, NBPC_LAYER4, PACKET_TAG_NONE);
}

static void
nbpf_detachd(nd)
	struct nbpf_d *nd;
{
	free(nd->nbd_state, M_DEVBUF);
}

int
nbpfopen(dev, flag, mode, p)
	dev_t dev;
	int flag;
	int mode;
	struct proc *p;
{
	return (0);
}

int
nbpfclose(dev, flag, mode, p)
	dev_t dev;
	int flag;
	int mode;
	struct proc *p;
{
	int error;
	return (error);
}

int
nbpfread(dev, uio, ioflag)
	dev_t dev;
	struct uio *uio;
	int ioflag;
{
	int error;
	return (error);
}

int
nbpfwrite(dev, uio, ioflag)
	dev_t dev;
	struct uio *uio;
	int ioflag;
{
	int error;
	return (error);
}

#define BIOCSTBLF	_IOW('B', 115, struct nbpf_program) /* nbpf tblset */

int
nbpfioctl(dev, cmd, addr, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t addr;
	int flag;
	struct proc *p;
{
	struct nbpf_d *d;
	int error;

	d = &nbpf_dtab[minor(dev)];

	switch (cmd) {
	case BIOCSETF:
		int ret = 0;
		error = nbpf_setf(d, (struct nbpf_program *)addr, &ret);
		if (ret != 0) {
			error = ret;
		}
		break;

	case BIOCSTBLF:
		struct nbpf_ioctl_table *nbiot = (struct nbpf_ioctl_table *)arg;
		nbpf_tableset_t *tset = d->nbd_tableset;
		if (tset == NULL) {
			nbpf_table_init(d);
		}
		error = nbpf_table_ioctl(nbiot, tset);
		break;
	}
	return (error);
}

int
nbpfpoll(dev, events, p)
{
	int revents;

	return (revents);
}

int
nbpfkqfilter(dev, kn)
{
	return (0);
}

int
nbpf_setf(d, fp, error)
	struct nbpf_d *d;
	struct nbpf_program *fp;
	int *error;
{
	struct nbpf_insn *ncode, *old;
	u_int nlen, size;
	int s;

	old = d->nbd_filter;
	if (fp->nbf_insns == 0) {
		if (fp->nbf_len != 0) {
			return (EINVAL);
		}
		s = splnet();
		d->nbd_filter = 0;
		reset_d(d);
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
		d->nbd_filter = ncode;
		reset_d(d);
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
nbpf_filtncatch(d, pkt, pktlen, slen, cpfn)
	struct nbpf_d *d;
	u_char *pkt;
	u_int pktlen;
	u_int slen;
	void *(*cpfn)(void *, const void *, size_t);
{
	slen = nbpf_filter(d->nbd_state, d->nbd_filter, pktlen, d->nbd_layer);
	if (slen != 0) {
		catchpacket(d, pkt, pktlen, slen, cpfn);
	}
}

void
nbpf_tap(arg, pkt, pktlen)
	caddr_t arg;
	u_char *pkt;
	u_int pktlen;
{
	struct nbpf_if *bp;
	struct nbpf_d *d;
	u_int slen;

	bp = (struct nbpf_if *)arg;
	for (d = bp->nbif_dlist; d != 0; d = d->nbd_next) {
		++d->nbd_rcount;

		nbpf_filtncatch(d, pkt, pktlen, memcpy);
	}
}

void
nbpf_mtap(arg, m)
	caddr_t arg;
	struct mbuf *m;
{
	void *(*cpfn)(void *, const void *, size_t);
	struct nbpf_if *bp = (struct nbpf_if *)arg;
	struct nbpf_d *d;
	u_int pktlen, slen, buflen;
	struct mbuf *m0;
	void *marg;

	pktlen = 0;
	for (m0 = m; m0 != 0; m0 = m0->m_next)
		pktlen += m0->m_len;

	if (pktlen == m->m_len) {
		cpfn = memcpy;
		marg = mtod(m, void *);
		buflen = pktlen;
	} else {
		cpfn = bpf_mcpy;
		marg = m;
		buflen = 0;
	}

	for (d = bp->nbif_dlist; d != 0; d = d->nbd_next) {
		if (!d->nbd_seesent && (m->m_pkthdr.rcvif == NULL))
			continue;
		++d->nbd_rcount;

		nbpf_filtncatch(d, marg, pktlen, slen, cpfn);
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
