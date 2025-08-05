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

	nd = (struct nbpf_d*) malloc(size, M_DEVBUF, M_DONTWAIT);
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
nbpf_state_init(nd, layer, tag)
	struct nbpf_d *nd;
	int layer, tag;
{
	nbpf_state_t *state;

	state = (nbpf_state_t*) malloc(sizeof(*state), M_DEVBUF, M_DONTWAIT);
	nbpf_set_tag(state, tag);
	nd->nbd_state = state;
	nd->nbd_layer = layer;
}

static void
nbpf_table_init(nd)
	struct nbpf_d *nd;
{
	nbpf_tableset_t *tset;
	nbpf_table_t *t;
	int error;

	error = nbpf_mktable(&tset, &t, NBPF_TABLE_TID, NBPF_TABLE_TYPE,
	NBPF_TABLE_HSIZE);
	if (error != 0) {
		return;
	}
	nd->nbd_tableset = tset;
	nd->nbd_table = t;
}

void
nbpf_attachd(nd)
	struct nbpf_d *nd;
{
	if (nd == NULL) {
		nd = nbpf_d_alloc(sizeof(*nd));
	}
	nbpf_state_init(nd, NBPC_LAYER4, PACKET_TAG_NONE);
	nbpf_tableset_init();
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
	if (nd != NULL) {
		nbpf_d_free(nd);
	}
}

int
nbpfioctl(nd, dev, cmd, addr, flag, p)
	struct nbpf_d *nd;
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
		error = nbpf_setf(nd, (struct nbpf_program*) addr, &ret);
		if (ret != 0) {
			error = ret;
		}
		break;

	case BIOCSTBLF:
		nbiot = (struct nbpf_ioctl_table*) addr;
		tset = nd->nbd_tableset;
		if (tset == NULL) {
			nbpf_table_init(nd);
		}
		error = nbpf_table_ioctl(nbiot, tset);
		break;
	}
	return (error);
}

static void
nbpf_set_d(nd, nbuf, nptr, size, len)
	struct nbpf_d *nd;
	nbpf_buf_t *nbuf;
	void *nptr;
	size_t size, len;
{
	nd->nbd_nbuf = nbuf;
	nd->nbd_nptr = nptr;
	nd->nbd_nsize = size;
	nd->nbd_nlen = len;
}

static void
nbpf_reset_d(nd)
	struct nbpf_d *nd;
{
	nd->nbd_nbuf = NULL;
	nd->nbd_nptr = NULL;
	nd->nbd_nsize = 0;
	nd->nbd_nlen = 0;
}

int
nbpf_allocbufs(nd)
	struct nbpf_d *nd;
{
	nd->nbd_nbuf = malloc(nd->nbd_bufsize, M_DEVBUF, M_NOWAIT);
	if (!nd->nbd_nbuf) {
		free(nd->nbd_nbuf, M_DEVBUF);
		return (ENOBUFS);
	}
	nd->nbd_nlen = 0;
	return (0);
}

void
nbpf_freed(nd)
	struct nbpf_d *nd;
{
	if (nd->nbd_nbuf != NULL) {
		free(nd->nbd_nbuf, M_DEVBUF);
	}
	if (nd->nbd_filter != NULL) {
		nbpf_nc_free(nd->nbd_filter);
	}
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
		nbpf_reset_d(nd);
		splx(s);
		if (old != 0) {
			nbpf_nc_free(old);
		}
		return (0);
	}
	nlen = fp->nbf_len;
	size = nlen * sizeof(*fp->nbf_insns);
	ncode = nbpf_nc_alloc(size);
	if (copyin((caddr_t) fp->nbf_insns, (caddr_t) ncode, size) == 0
			&& nbpf_validate(ncode, (int) nlen, error)) {
		s = splnet();
		nd->nbd_filter = ncode;
		nbpf_reset_d(nd);
		splx(s);
		if (old != 0) {
			nbpf_nc_free(old);
		}
		return (0);
	}
	nbpf_nc_free(ncode);
	return (EINVAL);
}

void
nbpf_filtncatch(nd, nd, pkt, pktlen, slen, cpfn)
	struct nbpf_d *nd;
	u_char *pkt;
	u_int pktlen;
	u_int slen;
	void *(*cpfn)(void *, const void *, size_t);
{
	slen = nbpf_filter(nd->nbd_state, nd->nbd_filter, nd->nbd_nbuf, nd->nbd_layer);
	if (slen != 0) {
		nbpf_catchpacket(nd, pkt, pktlen, slen, (NBPC_IP46 | NBPC_LAYER4), cpfn);
	}
}

#define ROTATE_BUFFERS(nd) 				\
	(nd)->nbd_hbuf = (nd)->nbd_sbuf; 	\
	(nd)->nbd_hlen = (nd)->nbd_slen; 	\
	(nd)->nbd_sbuf = (nd)->nbd_fbuf; 	\
	(nd)->nbd_slen = 0; 				\
	(nd)->nbd_fbuf = 0;

/*
 * TODO:
 * - updating the bpf
 */
static void
nbpf_catchpacket(nd, pkt, pktlen, snaplen, flags, cpfn)
	struct nbpf_d *nd;
	u_char *pkt;
	u_int pktlen, snaplen;
	int flags;
	void *(*cpfn)(void *, const void *, size_t);
{
	void *nptr;
	int totlen, hdrlen, curlen;
	int mflags, error;
	struct bpf_hdr *hp;

	mflags = (nd->nbd_state->nbs_info & flags);
	nptr = nbpf_dataptr(pkt);
	hdrlen = nd->nbd_bif->bif_hdrlen;
	totlen = hdrlen + min(snaplen, pktlen);
	if (totlen > nd->nbd_bufsize) {
		totlen = nd->nbd_bufsize;
	}
	curlen = BPF_WORDALIGN(nd->nbd_slen);
	if (curlen + totlen > nd->nbd_bufsize) {
		if (nd->nbd_fbuf == 0) {
			++nd->nbd_dcount;
			return;
		}
		ROTATE_BUFFERS(nd);
		curlen = 0;
	}

	/*
	 * Append the bpf header.
	 */
	hp = (struct bpf_hdr *)(nd->nbd_sbuf + curlen);
	microtime(&hp->bh_tstamp);
	hp->bh_datalen = pktlen;
	hp->bh_hdrlen = hdrlen;
	hp->bh_caplen = (totlen - hdrlen);

	/*
	 * Append the nbpf buffer
	 */
	nbpf_set_d(nd, pkt, (u_char *)(hp + hdrlen), hp->bh_caplen, hp->bh_hdrlen);
	error = nbpf_advcache(nd->nbd_state, nd->nbd_nbuf, nd->nbd_nptr, nd->nbd_nlen, nd->nbd_nsize, pkt, cpfn);
	if (error == mflags) {
		nd->nbd_state->nbs_info = 0;
	}
	nd->nbd_slen = (curlen + totlen);
}

static int
nbpf_advcache(nbpf_state_t *state, nbpf_buf_t *nbuf, void *nptr, u_int hdrlen, size_t size, void *buf, void *(*cpfn)(void *, const void *, size_t))
{
	int flags, error;
	u_int hlen;

	flags = nbpf_cache_all(state, nbuf, nptr);
	if ((flags & (NBPC_IP46 | NBPC_LAYER4)) == 0 || (flags & NBPC_IPFRAG) != 0) {
		state->nbs_info |= flags;
	}

	hlen = state->nbs_hlen;
	if (hlen != hdrlen) {
		error = nbpf_advstore(&nbuf, &nptr, hlen, size, buf);
		if (error) {
			return (error);
		}
		cpfn(nbuf, buf, hlen);
	} else {
		error = nbpf_advstore(&nbuf, &nptr, hdrlen, size, buf);
		if (error) {
			return (error);
		}
		cpfn(nbuf, buf, hdrlen);
	}
	return (flags);
}
