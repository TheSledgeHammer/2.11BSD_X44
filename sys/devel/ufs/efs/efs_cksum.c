/*	$NetBSD: efs_subr.c,v 1.14 2021/12/10 20:36:04 andvar Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: efs_subr.c,v 1.14 2021/12/10 20:36:04 andvar Exp $");

#include <sys/param.h>

#include <ufs/efs/efs.h>

/*
 * EFS basic block layout:
 */
#define EFS_BB_UNUSED	0	/* bb 0 is unused */
#define EFS_BB_SB		1	/* bb 1 is superblock */
#define EFS_BB_BITMAP	2	/* bb 2 is bitmap (unless moved by growfs) */
/* bitmap continues, then padding up to first aligned cylinder group */

/*
 * Struct efs_extent limits us to 24 bit offsets, therefore the maximum
 * efs.sb_size is 2**24 blocks (8GB).
 *
 * Trivia: IRIX's mkfs_efs(1M) has claimed the maximum to be 0xfffffe for years.
 */
#define EFS_SIZE_MAX		0x01000000

/*
 * Calculate a checksum for the provided superblock in __host byte order__.
 *
 * At some point SGI changed the checksum algorithm slightly, which can be
 * enabled with the 'new' flag.
 *
 * Presumably this change occurred on or before 24 Oct 1988 (around IRIX 3.1),
 * so we're pretty unlikely to ever actually see an old checksum. Further, it
 * means that EFS_NEWMAGIC filesystems (IRIX >= 3.3) must match the new
 * checksum whereas EFS_MAGIC filesystems could potentially use either
 * algorithm.
 *
 * See comp.sys.sgi <1991Aug9.050838.16876@odin.corp.sgi.com>
 */
int32_t
efs_checksum(struct efs *fs, int new)
{
	int i;
	int32_t cksum;
	uint8_t *sbarray = (uint8_t *)fs;

	KASSERT((EFS_CHECKSUM_SIZE % 2) == 0);

	for (i = cksum = 0; i < EFS_CHECKSUM_SIZE; i += 2) {
		uint16_t v;
		memcpy(&v, &sbarray[i], sizeof(v));
		cksum ^= be16toh(v);
		cksum  = (cksum << 1) | (new && cksum < 0);
	}
	return (cksum);
}

/*
 * Determine if the superblock is valid.
 *
 * Returns 0 if valid, else invalid. If invalid, 'why' is set to an
 * explanation.
 */
int
efs_validate(struct efs *fs, const char **why)
{
	uint32_t ocksum, ncksum;

	*why = NULL;

	if (be32toh(fs->efs_magic) != EFS1_MAGIC &&
	    be32toh(fs->efs_magic) != EFS2_MAGIC) {
		*why = "efs_magic invalid";
		return (1);
	}

	ocksum = htobe32(efs_checksum(fs, 0));
	ncksum = htobe32(efs_checksum(fs, 1));
	if (fs->efs_checksum != ocksum && fs->efs_checksum != ncksum) {
		*why = "efs_checksum invalid";
		return (1);
	}

	if (be32toh(fs->efs_size) > EFS_SIZE_MAX) {
		*why = "efs_size > EFS_SIZE_MAX";
		return (1);
	}

	if (be32toh(fs->efs_firstcg) <= EFS_BB_BITMAP) {
		*why = "efs_firstcg <= EFS_BB_BITMAP";
		return (1);
	}

	/* XXX - add better sb consistency checks here */
	if (fs->efs_cgfsize == 0 ||
			fs->efs_cgisize == 0 ||
			fs->efs_ncg == 0 ||
			fs->efs_bmsize == 0) {
		*why = "something bad happened";
		return (1);
	}

	return (0);
}
