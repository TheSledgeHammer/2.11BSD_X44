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

#ifndef _PUFFS_H_
#define _PUFFS_H_

#include <sys/param.h>
#include <sys/select.h>
#include <sys/queue.h>
#include <sys/lock.h>

#include <fs/puffs/puffs_msgif.h>
#include <fs/puffs/puffs_mount.h>

#define PNODE_INACTIVE		0x01
#define PNODE_LOCKED		0x02
#define PNODE_WANTED		0x04

struct puffs_node {
	void					*pn_cookie;		/* userspace pnode cookie	*/
	struct vnode			*pn_vp;			/* backpointer to vnode		*/
	uint32_t				pn_stat;		/* node status			*/

	LIST_ENTRY(puffs_node) 	pn_entries;
};

#define PUFFS_UPDATEATIME	0x01
#define PUFFS_UPDATECTIME	0x02
#define PUFFS_UPDATEMTIME	0x04
#define PUFFS_UPDATESIZE	0x08

extern struct vnodeops 	puffs_vnodeops;
extern struct vfsops 	puffs_vfsops;

#endif /* _PUFFS_H_ */
