/*
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Rich $alz of BBN Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>

#ifndef lint
static char copyright[] =
"@(#) Copyright (c) 1990, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)clri.c	8.3 (Berkeley) 4/28/95";
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>

#include <ufs/ufs/dinode.h>
#include <ufs/ffs/fs.h>
#include <ufs/ffs/ffs_extern.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

/*
 * Possible superblock locations ordered from most to least likely.
 */
static off_t sblock_try[] = SBLOCKSEARCH;
off_t sblockloc;

int
main(int argc, char *argv[])
{
	register struct fs *sbp;
	register struct ufs1_dinode *ip1;
	register struct ufs2_dinode *ip2;
	register int fd;
	void *ibuf;
	long generation, bsize;
	off_t offset;
	int inonum;
	int is_ufs2 = 0;
	int i;
	char *fs, sblock[SBSIZE];
	long dev_bsize;

	if (argc < 3) {
		(void)fprintf(stderr, "usage: clri filesystem inode ...\n");
		exit(1);
	}

	fs = *++argv;

	/* get the superblock. */
	if ((fd = open(fs, O_RDWR, 0)) < 0)
		err(1, "%s", fs);

	for (i = 0;; i++) {
		sblockloc = sblock_try[i];
		if (sblockloc == -1)
			errx(1, "%s: can't find superblock", fs);
		if (pread(fd, sblock, sizeof(sblock), sblockloc) != sizeof(sblock))
			errx(1, "%s: can't read superblock", fs);

		sbp = (struct fs*) sblock;
		switch (sbp->fs_magic) {
		case FS_UFS2_MAGIC:
			is_ufs2 = 1;
			/*FALLTHROUGH*/
		case FS_UFS1_MAGIC:
			break;
		default:
			continue;
		}

		/* check we haven't found an alternate */
		if (is_ufs2 || (sbp->fs_flags & FS_FLAGS_UPDATED)) {
			if ((uint64_t) sblockloc != sbp->fs_sblockloc)
				continue;
		} else {
			if (sblockloc == SBLOCK_UFS2)
				continue;
		}

		break;
	}
	if (lseek(fd, sblockloc, SEEK_SET) < 0)
		err(1, "%s", fs);
	if (read(fd, sblock, sizeof(sblock)) != sizeof(sblock))
		errx(1, "%s: can't read superblock", fs);
	(void)fsync(fd);

	dev_bsize = sbp->fs_fsize / fsbtodb(sbp, 1);
	bsize = sbp->fs_bsize;
	ibuf = malloc(bsize);
	if (ibuf == NULL) {
		err(1, "malloc");
	}

	/* remaining arguments are inode numbers. */
	while (*++argv) {
		/* get the inode number. */
		if ((inonum = atoi(*argv)) <= 0)
			errx(1, "%s is not a valid inode number", *argv);
		(void)printf("clearing %d\n", inonum);

		/* read in the appropriate block. */
		offset = ino_to_fsba(sbp, inonum);	/* inode to fs blk */
		offset = fsbtodb(sbp, offset);		/* fs blk disk blk */
		offset *= dev_bsize;			/* disk blk to bytes */

		/* seek and read the block */
		if (lseek(fd, offset, SEEK_SET) < 0)
			err(1, "%s", fs);
		if (read(fd, ibuf, bsize) != bsize)
			err(1, "%s", fs);

		/* get the inode within the block. */
		if (is_ufs2) {
			ip2 = &((struct ufs2_dinode *)ibuf)[ino_to_fsbo(sbp, inonum)];
			/* clear the inode, and bump the generation count. */
			generation = ip2->di_gen + 1;
			memset(ip2, 0, sizeof(*ip2));
			ip2->di_gen = generation;
		} else {
			ip1 = &((struct ufs1_dinode *)ibuf)[ino_to_fsbo(sbp, inonum)];
			/* clear the inode, and bump the generation count. */
			generation = ip1->di_gen + 1;
			memset(ip1, 0, sizeof(*ip1));
			ip1->di_gen = generation;
		}

		/* backup and write the block */
		if (lseek(fd, (off_t)-bsize, SEEK_CUR) < 0)
			err(1, "%s", fs);
		if (write(fd, ibuf, bsize) != bsize)
			err(1, "%s", fs);
		(void)fsync(fd);
	}
	free(ibuf);
	(void)close(fd);
	exit(0);
}
