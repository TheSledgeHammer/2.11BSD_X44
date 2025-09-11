/*	$NetBSD: puffs_sys.h,v 1.11 2006/12/01 12:48:31 pooka Exp $	*/

/*
 * Copyright (c) 2005, 2006  Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by the
 * Google Summer of Code program and the Ulla Tuominen Foundation.
 * The Google SoC project was mentored by Bill Studenmund.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef PUFFS_MOUNT_H_
#define PUFFS_MOUNT_H_

#include <sys/queue.h>

/*
 * While a request is going to userspace, park the caller within the
 * kernel.  This is the kernel counterpart of "struct puffs_req".
 */
struct puffs_park {
	struct puffs_req 		park_preq;		/* the relevant preq		*/

	void					*park_kernbuf;	/* kernel buffer address	*/
	size_t					park_buflen;	/* buffer length		*/
	size_t					park_copylen;	/* length to copy to userspace  */

	uint32_t 				park_flags;

	TAILQ_ENTRY(puffs_park) park_entries;
};
#define park_id				park_preq.preq_id
#define park_opclass		park_preq.preq_opclass
#define park_optype			park_preq.preq_optype
#define park_cookie			park_preq.preq_cookie
#define park_rv				park_preq.preq_rv


#define PUFFS_SIZEOPREQ_UIO_IN 	1
#define PUFFS_SIZEOPREQ_UIO_OUT 2
#define PUFFS_SIZEOPREQ_BUF_IN 	3
#define PUFFS_SIZEOPREQ_BUF_OUT 4

#define PUFFS_SIZEOP_UIO(a)	\
	(((a) == PUFFS_SIZEOPREQ_UIO_IN)||(a) == PUFFS_SIZEOPREQ_UIO_OUT)
#define PUFFS_SIZEOP_BUF(a)	\
	(((a) == PUFFS_SIZEOPREQ_BUF_IN)||(a) == PUFFS_SIZEOPREQ_BUF_OUT)

/* XXX: alignment-optimization */
struct puffs_sizepark {
	uint64_t				pkso_reqid;
	uint8_t					pkso_reqtype;

	struct uio				*pkso_uio;
	void					*pkso_copybuf;
	size_t					pkso_bufsize;

	TAILQ_ENTRY(puffs_sizepark) pkso_entries;
};

#define DPRINTF(x)
#define DPRINTF_VERBOSE(x)

#define MPTOPUFFSMP(mp) ((struct puffs_mount *)((mp)->mnt_data))
#define PMPTOMP(pmp) 	(pmp->pmp_mp)
#define VPTOPP(vp) 		((struct puffs_node *)(vp)->v_data)
#define VPTOPNC(vp) 	(((struct puffs_node *)(vp)->v_data)->pn_cookie)
#define VPTOPUFFSMP(vp) ((struct puffs_mount*)((struct puffs_node*)vp->v_data))
#define FPTOPMP(fp) 	(((struct puffs_instance *)fp->f_data)->pi_pmp)
#define FPTOPI(fp) 		((struct puffs_instance *)fp->f_data)

#define EXISTSOP(pmp, op) \
	(((pmp)->pmp_flags & PUFFS_KFLAG_ALLOPS) || ((pmp)->pmp_vnopmask[PUFFS_VN_##op]))

TAILQ_HEAD(puffs_wq, puffs_park);
struct puffs_mount {
	struct lock_object				pmp_lock;

	struct puffs_args				pmp_args;
#define pmp_flags 		pmp_args.pa_flags
#define pmp_vnopmask 	pmp_args.pa_vnopmask

	struct puffs_wq					pmp_req_touser;
	size_t							pmp_req_touser_waiters;
	size_t							pmp_req_maxsize;

	struct puffs_wq					pmp_req_replywait;
	TAILQ_HEAD(, puffs_sizepark)	pmp_req_sizepark;

	LIST_HEAD(, puffs_node)			pmp_pnodelist;

	struct mount					*pmp_mp;
	struct vnode					*pmp_root;
	void							*pmp_rootcookie;
	struct selinfo					*pmp_sel;				/* in puffs_instance */

	unsigned int					pmp_nextreq;
	uint8_t							pmp_status;
};

#define PUFFSTAT_BEFOREINIT	0
#define PUFFSTAT_MOUNTING	1
#define PUFFSTAT_RUNNING	2
#define PUFFSTAT_DYING		3

#endif /* PUFFS_MOUNT_H_ */
