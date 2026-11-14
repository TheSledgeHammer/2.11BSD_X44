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

#include "opt_nbpf.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/ioctl.h>

#include <net/bpf.h>
#include <net/bpfdesc.h>

#include "nbpf.h"
#include "nbpf_ncode.h"
#include "nbpfdesc.h"

#ifndef BPF_BUFSIZE
/*
 * 4096 is too small for FDDI frames. 8192 is too small for gigabit Ethernet
 * jumbos (circa 9k), ATM, or Intel gig/10gig ethernet jumbos (16k).
 */
# define BPF_BUFSIZE 	32768
#endif

extern int bpf_bufsize;// = BPF_BUFSIZE;
extern int bpf_maxbufsize;// = BPF_DFLTBUFSIZE;	/* XXX set dynamically, see above */

static struct nbpf_d *nbpf_d_alloc(size_t);
static void nbpf_d_free(struct nbpf_d *);
static void nbpf_set_d(struct nbpf_d *, nbpf_buf_t *, void *, size_t, size_t);
static void nbpf_reset_d(struct nbpf_d *);
static int nbpf_setf(struct nbpf_d *, struct nbpf_program *, int *);
static void nbpf_catchpacket(struct nbpf_d *, u_char *, u_int, u_int, int, void *(*)(void *, const void *, size_t), int *);
static int nbpf_advcache(nbpf_state_t *, nbpf_buf_t *, void *, u_int, size_t, void *, void *(*)(void *, const void *, size_t));

static struct nbpf_d *
nbpf_d_alloc(size_t size)
{
	struct nbpf_d *nd;

	nd = (struct nbpf_d *)malloc(size, M_DEVBUF, M_DONTWAIT);
	return (nd);
}

static void
nbpf_d_free(struct nbpf_d *nd)
{
	if (nd != NULL) {
		free(nd, M_DEVBUF);
	}
}

static void
nbpf_state_init(struct nbpf_d *nd, int layer, int tag)
{
	nbpf_state_t *state;

	state = (nbpf_state_t *)malloc(sizeof(*state), M_DEVBUF, M_DONTWAIT);
	nbpf_set_tag(state, tag);
	nd->nbd_statehandler = state;
	nd->nbd_layer = layer;
}

static void
nbpf_table_init(struct nbpf_d *nd)
{
	nbpf_tableset_t *tset;
	nbpf_table_t *t;
	int error;

	error = nbpf_mktable(&tset, &t);
	if (error != 0) {
		return;
	}
	nd->nbd_tableset = tset;
	nd->nbd_table = t;
}

void
nbpf_attachd(struct nbpf_d *nd, struct bpf_d *d)
{
	if (nd == NULL) {
		nd = nbpf_d_alloc(sizeof(*nd));
	}
    nd->nbd_bpf = d;
	nbpf_state_init(nd, NBPC_LAYER4, PACKET_TAG_NONE);
	nbpf_table_init(nd);
}

void
nbpf_detachd(struct nbpf_d *nd, struct bpf_d *d)
{
	if (nd->nbd_statehandler != NULL) {
		free(nd->nbd_statehandler, M_DEVBUF);
	}
	if (nd != NULL) {
        nd->nbd_bpf = d;
		nbpf_d_free(nd);
	}
}

int
nbpfioctl(struct nbpf_d *nd, dev_t dev, u_long cmd, caddr_t addr, int flag, struct proc *p)
{
	int s, error = 0, ret;

	switch (cmd) {
	/*
	 * Get buffer len [for read()].
	 */
	case BIOCGNBLEN:
		*(u_int *)addr = nd->nbd_nbufsize;
		break;

	/*
	 * Set buffer length.
	 */
	case BIOCSNBLEN:
		if (nd->nbd_bif != 0) {
			error = EINVAL;
		} else {
			u_int size = *(u_int *)addr;

			if (size > bpf_maxbufsize) {
				*(u_int *)addr = size = bpf_maxbufsize;
			} else if (size < BPF_MINBUFSIZE) {
				*(u_int *)addr = size = BPF_MINBUFSIZE;
			}
			nd->nbd_nbufsize = size;
		}
		break;

	/*
	 * Set link layer read filter.
	 */
	case BIOCSETNF:
		error = nbpf_setf(nd, (struct nbpf_program *)addr, &ret);
		if (ret != 0) {
			error = ret;
		}
		break;

	/*
	 * Flush read packet buffer.
	 */
	case BIOCNBFLUSH:
		s = splnet();
		nbpf_reset_d(nd);
		splx(s);
		break;

	/*
	 * NBPF state table controls.
	 */
	case BIOCSETTBLNF:
		error = nbpf_table_ioctl((struct nbpf_ioctl_table *)addr, nd->nbd_tableset);
		break;

	default:
		error = EINVAL;
		break;
	}
	return (error);
}

static void
nbpf_set_d(struct nbpf_d *nd, nbpf_buf_t *nbuf, void *nptr, size_t size, size_t len)
{
	nd->nbd_nbuf = nbuf;
	nd->nbd_nptr = nptr;
	nd->nbd_nsize = size;
	nd->nbd_nlen = len;
}

static void
nbpf_reset_d(struct nbpf_d *nd)
{
	nd->nbd_nbuf = NULL;
	nd->nbd_nptr = NULL;
	nd->nbd_nsize = 0;
	nd->nbd_nlen = 0;
}

int
nbpf_allocbufs(struct nbpf_d *nd, size_t nbufsize)
{
	if (nd->nbd_nbufsize == 0) {
		nd->nbd_nbufsize = nbufsize;
	}
	nd->nbd_nbuf = malloc(nbufsize, M_DEVBUF, M_NOWAIT);
	if (!nd->nbd_nbuf) {
		free(nd->nbd_nbuf, M_DEVBUF);
		return (ENOBUFS);
	}
	nd->nbd_nlen = 0;
	return (0);
}

void
nbpf_freed(struct nbpf_d *nd)
{
	if (nd->nbd_nbuf != NULL) {
		free(nd->nbd_nbuf, M_DEVBUF);
	}
	if (nd->nbd_filter != NULL) {
		nbpf_nc_free(nd->nbd_filter);
	}
}

static int
nbpf_setf(struct nbpf_d *nd, struct nbpf_program *fp, int *error)
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
	if (copyin((caddr_t)fp->nbf_insns, (caddr_t)ncode, size) == 0
			&& nbpf_validate(ncode, (int)nlen, error)) {
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
nbpf_filtncatch(struct nbpf_d *nd, u_char *pkt, u_int pktlen, void *(*cpfn)(void *, const void *, size_t), int *do_wakeup)
{
	u_int slen;

	slen = nbpf_filter(nd->nbd_statehandler, nd->nbd_filter, nd->nbd_nbuf, nd->nbd_layer);
	if (slen != 0) {
		nbpf_catchpacket(nd, pkt, pktlen, slen, (NBPC_IP46 | NBPC_LAYER4), cpfn, do_wakeup);
	}
}

#define BPF_ROTATE_BUFFERS(nd) 			\
	(nd)->nbd_hbuf = (nd)->nbd_sbuf; 	\
	(nd)->nbd_hlen = (nd)->nbd_slen; 	\
	(nd)->nbd_sbuf = (nd)->nbd_fbuf; 	\
	(nd)->nbd_slen = 0; 				\
	(nd)->nbd_fbuf = 0;

/*
 * GOAL:
 * - To provide a secondary filter on buffer overflows in catchpacket.
 *
 * TODO:
 * - fix up: to work better within bpf's catchpacket.
 * - perform check on nbuf, if overflow increment bpf dcount and return
 *
 */
static void
nbpf_catchpacket(struct nbpf_d *nd, u_char *pkt, u_int pktlen, u_int snaplen, int flags, void *(*cpfn)(void *, const void *, size_t), int *do_wakeup)
{
	void *nptr;
	int totlen, hdrlen, curlen;
	int mflags, error;
	struct bpf_hdr *hp;

	++nd->nbd_ccount;

	mflags = (nd->nbd_statehandler->nbs_info & flags);
	nptr = nbpf_dataptr(pkt);
	hdrlen = nd->nbd_bif->bif_hdrlen;
	totlen = hdrlen + min(snaplen, pktlen);
	if (totlen > nd->nbd_nbufsize) {
		totlen = nd->nbd_nbufsize;
	}
	curlen = BPF_WORDALIGN(nd->nbd_slen);
	if (curlen + totlen > nd->nbd_nbufsize) {
		if (nd->nbd_fbuf == 0) {
			++nd->nbd_dcount;
			return;
		}
		BPF_ROTATE_BUFFERS(nd);
		*do_wakeup = 1;
		curlen = 0;
	} else if (nd->nbd_immediate || nd->nbd_state == BPF_TIMED_OUT) {
		*do_wakeup = 1;
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
	nbpf_set_d(nd, (u_char *)(hp + hdrlen), nptr, hp->bh_caplen, hp->bh_hdrlen);
	error = nbpf_advcache(nd->nbd_statehandler, nd->nbd_nbuf, nd->nbd_nptr, nd->nbd_nlen, nd->nbd_nsize, pkt, cpfn);
	if (error == mflags) {
		nd->nbd_statehandler->nbs_info = 0;
	}
	nd->nbd_bpf->bd_slen = (curlen + totlen);
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
		(*cpfn)(nbuf, buf, hlen);
	} else {
		error = nbpf_advstore(&nbuf, &nptr, hdrlen, size, buf);
		if (error) {
			return (error);
		}
		(*cpfn)(nbuf, buf, hdrlen);
	}
	return (flags);
}
