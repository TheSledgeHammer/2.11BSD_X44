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
 */

/* A structure revision of the blockchain */

/*
 * TODO:
 * - inode:
 * 		- mapping to fs inode
 * - blocks:
 * 		- mapping block
 * - see ext2fs mapping of above
 *
 * - map inodes & blocks inside hashchain: silly to map it with a secondary hashlist!
 * 		- store within transaction structure
 *
 * - log_dev_bshift to fs_dev_bsize
 * - hashchain lock
 * - inode lock
 * - revise content marked XXX
 */

#ifndef SYS_BLOCKCHAIN_H_
#define SYS_BLOCKCHAIN_H_

#include <sys/queue.h>
#include <sys/lock.h>

/* blockchain entry */
struct hashchain;
CIRCLEQ_HEAD(hashchain, ht_hchain);
struct ht_hchain {
	CIRCLEQ_ENTRY(ht_hchain)	hc_entry;			/* a list of chain entries */
	struct htree_root			*hc_hroot;			/* htree root per block */
	struct ht_transaction		*hc_trans;			/* pointer to transaction info */

	const char					*hc_name;			/* name of chain entry */
	int							hc_len;

	struct lock_object			hc_lock;			/* lock on chain */
	int							hc_flags;			/* flags */
	int							hc_refcnt;			/* number of entries in this chain */
};

/* blockchain transaction */
struct ht_htransaction {
	int32_t						ht_hash;			/* size of hash for this transaction */
	int32_t						ht_reclen;			/* length of this record */
	uint32_t					ht_flag;			/* flags */
	struct timespec				*ht_timespec;		/* timestamp */

    int32_t						ht_atime;			/* Last access time. */
    int32_t						ht_atimesec;		/* Last access time. */
    int32_t						ht_mtime;			/* Last modified time. */
    int32_t						ht_mtimesec;		/* Last modified time. */
    int32_t						ht_ctime;			/* Last inode change time. */
    int32_t						ht_ctimesec;		/* Last inode change time. */

    /* XXX: */
	uint32_t					ht_type;
	int32_t						ht_len;
	uint32_t					ht_checksum;
	uint32_t					ht_generation;
	uint32_t					ht_version;

	union ht_ino {
		ino_t 					u_ino;
		mode_t					u_imode;
	} ht_ino;
	int							ht_inocnt;

	struct ht_hblocklist		*ht_blocklist;		/* blocks held by this transaction */
	struct ht_hinodelist		*ht_inodelist;		/* inodes held by this transaction */

	int							ht_dev_bshift;
	int							ht_dev_bmask;

};

struct ht_hblocklist {
	u_int32_t 					hc_type;
	int32_t 					hc_len;
	int32_t						hc_blkcount;
	struct {
		int64_t					hc_daddr;
		int32_t					hc_unused;
		int32_t					hc_dlen;
	} hc_blocks[0];
};

struct ht_hinodelist {
	uint32_t					hc_type;
	int32_t						hc_len;
	int32_t						hc_inocnt;
	int32_t						hc_clear; 			/* set if previously listed inodes hould be ignored */
	struct {
		uint32_t 				hc_inumber;
		uint32_t 				hc_imode;
	} hc_inodes[0];
};

/* Macro's between FS's */
#define dev_bshift
#define dev_bmask

#define hashchain_lock(hc)		simple_lock((hc)->hc_lock)
#define hashchain_unlock(hc) 	simple_unlock((hc)->hc_lock)

#endif /* SYS_DEVEL_HTBC_HTBC_NEW_H_ */
