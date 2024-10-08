/*
 * The 3-Clause BSD License:
 * Copyright (c) 2020 Martin Kelly
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @(#)vfs_htbc.c	1.00
 */

#ifndef SYS_DEVEL_HTBC_HTREE_H_
#define SYS_DEVEL_HTBC_HTREE_H_

#include <sys/queue.h>
#include <sys/tree.h>

/* Red-Black HashTree */

/* hashtree entries  */
struct hashtree_entry {
	RB_ENTRY(hashtree_entry) 	h_node;
	uint32_t 			 		h_hash;
	uint32_t 					h_block;
	uint16_t					h_max;
	uint16_t					h_num;
};

/* hashtree root */
struct hashtree_rbtree;
RB_HEAD(hashtree_rbtree, hashtree_entry);
struct hashtree_root {
	struct hashtree_rbtree 		h_root[0];

};

/* hashtree node:  */
struct hashtree_node {
	struct hashtree_entry 		h_entries[0];
};

/* Blockchain: */
struct hashhead;
CIRCLEQ_HEAD(hashhead, hashchain);
struct hashchain {
	CIRCLEQ_ENTRY(hashchain)	hc_entry;			/* a list of chain entries */
	struct htree_root			*hc_hroot;			/* htree root per block */
	const char					*hc_name;			/* name of chain entry */
	int							hc_length;
	int							hc_refcnt;
};

/* VFS and Filesystem Access */

/* represents as vfs for filesystem purposes */
struct hashnode {
	struct vnode 				*hn_vnode;
	struct mount				*hn_mount;
};

/* Block level Manipulation */
struct hashblock {

	void 						*hb_data;
};

/* File level Manipulation */
struct hashfile {

};

extern struct hashhead *blockchain;

#endif /* SYS_DEVEL_HTBC_HTREE_H_ */
