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

#ifndef _NBPF_NBPFDESC_H_
#define _NBPF_NBPFDESC_H_

/* nbpf program: */
struct nbpf_program {
	u_int 				nbf_len;
	struct nbpf_insn 	*nbf_insns; /* ncode */
};

/* nbpf descriptor: */
struct nbpf_d {
	/* nbpf tables */
	nbpf_tableset_t		*nbd_tableset;
	nbpf_table_t 		*nbd_table;

	/* nbpf ncode */
	nbpf_state_t		*nbd_state;
	struct nbpf_insn 	*nbd_filter;
	int 			nbd_layer;

	/* nbpf buffer */
	void 			*nbd_nptr;			/* nbuf pointer to object */
	nbpf_buf_t 		*nbd_nbuf;			/* nbuf store */
	size_t			nbd_nlen;			/* nbuf length */
	size_t			nbd_nsize; 			/* nbuf size */

	/* bpf information: needed by the nbpf */
	struct bpf_d		*nbd_bpf; 			/* bpf back-pointer */
#define nbd_sbuf		nbd_bpf->bd_sbuf
#define nbd_hbuf		nbd_bpf->bd_hbuf
#define nbd_fbuf		nbd_bpf->bd_fbuf
#define nbd_slen		nbd_bpf->bd_slen
#define nbd_hlen		nbd_bpf->bd_hlen

#define nbd_bufsize		nbd_bpf->bd_bufsize /* absolute length of buffers */

#define	nbd_bif			nbd_bpf->bd_bif		/* interface descriptor */
#define nbd_rcount		nbd_bpf->bd_rcount
#define nbd_dcount		nbd_bpf->bd_dcount
#define nbd_ccount		nbd_bpf->bd_ccount
};

#define BIOCSTBLF	_IOW('B', 115, struct nbpf_program) /* nbpf tblset */

struct nbpf_d 	*nbpf_d_alloc(size_t);
void	nbpf_d_free(struct nbpf_d *);

void	nbpf_attachd(struct nbpf_d *);
void	nbpf_detachd(struct nbpf_d *);
int	nbpfioctl(struct nbpf_d *, dev_t, u_long, caddr_t, int, struct proc *);
void	nbpf_filtncatch(struct bpf_d *, struct nbpf_d *, u_char *, u_int, u_int, void *(*)(void *, const void *, size_t));

#endif /* _NBPF_NBPFDESC_H_ */
