/*	$NetBSD: efs_extent.h,v 1.3 2007/07/04 19:24:09 rumble Exp $	*/

/*
 * Copyright (c) 2006 Stephen M. Rumble <rumble@ephemeral.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _UFS_EFS_EXTENT_H
#define _UFS_EFS_EXTENT_H

/* bitmap number of bytes and words */
#define EFS_BM_NBYTES 		8
#define EFS_BM_NWORDS 		2

/* extent bitmap */
struct efs_bitmap {
    union {
        uint8_t  			bytes[EFS_BM_NBYTES];
        uint32_t 			words[EFS_BM_NWORDS];
    } eb_muddle;
#define eb_bytes 			eb_muddle.bytes
#define eb_words 			eb_muddle.words
};

struct efs_extent_iterator {
	struct efs				*exi_efs;
	off_t					exi_logical;
	off_t 					exi_direct;
	off_t 					exi_indirect;
};

/* on-disk extent */
struct efs_extent {
	uint8_t  				ex_magic;
	uint32_t 				ex_bn;
	uint8_t  				ex_length;
	uint32_t 				ex_offset;
};

#define EFS_EXTENT_MAGIC		0
#define EFS_EXTENT_BN_MASK		0x00ffffff
#define EFS_EXTENT_OFFSET_MASK	0x00ffffff

#define EFS_EXTENTS_PER_BB		(EFS_BB_SIZE / sizeof(struct efs_bitmap))

#endif /* _UFS_EFS_EXTENT_ */
