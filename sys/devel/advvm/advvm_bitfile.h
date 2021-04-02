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
 *	@(#)advvm_bitfile.h 1.0 	7/01/21
 */

#ifndef _ADVVM_BITFILE_H_
#define _ADVVM_BITFILE_H_

#include <sys/queue.h>

/* bitfile tag */
struct advvm_bittag {
	uint32_t						abt_number;		 /* tag number, 1 based */
	uint32_t						abt_sequence;	 /* sequence number */
};
typedef struct advvm_bittag			advvm_bittag_t;

/* bitfile tag macros */
#define MAX_ADVVM_TAGS 				0x7fffffff
#define BFTAG_EQUALS(tag1, tag2)	(((tag1).abt_number == (tag2).abt_number) && ((tag1).abt_sequence == (tag2).abt_sequence))
#define BFTAG_INDEX(tag)			(tag).abt_number
#define BFTAG_RSVD(tag) 			(((tag).abt_number) < 0)
#define BFTAG_REG(tag) 				(((tag).abt_number) > 0)
#define BFTAG_SEQ(tag) 				((tag).abt_sequence)
#define BFTAG_NULL(tag) 			((tag.abt_number) == 0)

/* bitfile set id */
struct advvm_bfsid {
	struct advvm_bittag				*abd_dirtag;	/* tag of bitfile-set's tag directory */
	uint32_t						abd_domid;		/* bitfile-set's domain's ID */
};
typedef struct advvm_bfsid			advvm_bfsid_t;

struct advvm_global_bitag {
	struct advvm_bfsid  			gpt_bfsid;
	struct advvm_bittag 			gbt_tag;
};
typedef struct advvm_global_bitag	advvm_global_bitag_t;

/* bitfile set */
struct bfs_head;
TAILQ_HEAD(bfs_head, advvm_bfs);
struct advvm_bfs {
	struct bfs_head					*abs_head;		/* bitfile set list */
	TAILQ_ENTRY(advvm_bfs)			abs_entry;		/* bitfile set list entry */
	struct advvm_bfsid  			abs_bfsid; 		/* bitfile-set's ID */
	u_int 							abs_magic;		/* magic number: structure validation */
	advvm_domain_t					*abs_domain;	/* pointer to set's domain structure */
	dev_t							abs_dev;		/* set's dev_t */
	struct advvm_bittag				abs_bftag;		/* tag of bitfile-set's tag directory */
};
typedef struct advvm_bfs			advvm_bfs_t;

#endif /* _ADVVM_BITFILE_H_ */
