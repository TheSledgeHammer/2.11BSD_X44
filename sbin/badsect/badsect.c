/*
 * Copyright (c) 1981, 1983, 1993
 *	The Regents of the University of California.  All rights reserved.
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
#if 0
static char copyright[] =
"@(#) Copyright (c) 1981, 1983, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)badsect.c	8.2 (Berkeley) 5/4/95";
#endif
#endif /* not lint */

/*
 * badsect
 *
 * Badsect takes a list of file-system relative sector numbers
 * and makes files containing the blocks of which these sectors are a part.
 * It can be used to contain sectors which have problems if these sectors
 * are not part of the bad file for the pack (see bad144).  For instance,
 * this program can be used if the driver for the file system in question
 * does not support bad block forwarding.
 */
#include <sys/param.h>
#include <sys/dir.h>
#include <sys/stat.h>

#include <ufs/ufs/dinode.h>
#include <ufs/ffs/fs.h>
//#include <ufs/ffs/ffs_extern.h>

#include <dirent.h>
#include <fcntl.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <err.h>

union {
	struct	fs fs;
	char	fsx[SBSIZE];
} ufs;
#define sblock	ufs.fs
union {
	struct	cg cg;
	char	cgx[MAXBSIZE];
} ucg;
#define	acg	ucg.cg
struct	fs *fs;
int	fso, fsi;
int	errs;
long	dev_bsize = 1;
int is_ufs2;

char buf[MAXBSIZE];

void 	rdfs(daddr_t, int, char *);
int		chkuse(daddr_t, int);
static void usage(void);

const off_t sblock_try[] = SBLOCKSEARCH;

int
main(int argc, char *argv[])
{
	daddr_t number;
	struct stat stbuf, devstat;
	register struct direct *dp;
	int i;
	DIR *dirp;
	char name[BUFSIZ];

	if (argc < 3) {
		usage();
	}
	if (chdir(argv[1]) < 0 || stat(".", &stbuf) < 0) {
		perror(argv[1]);
		exit(2);
	}
	strcpy(name, _PATH_DEV);
	if ((dirp = opendir(name)) == NULL) {
		perror(name);
		exit(3);
	}
	while ((dp = readdir(dirp)) != NULL) {
		strcpy(&name[5], dp->d_name);
		if (stat(name, &devstat) < 0) {
			perror(name);
			exit(4);
		}
		if (stbuf.st_dev == devstat.st_rdev &&
		    (devstat.st_mode & IFMT) == IFBLK)
			break;
	}
	closedir(dirp);
	if (dp == NULL) {
		printf("Cannot find dev 0%lo corresponding to %s\n",
			stbuf.st_rdev, argv[1]);
		exit(5);
	}
	if ((fsi = open(name, 0)) < 0) {
		perror(name);
		exit(6);
	}
	fs = &sblock;
	for (i = 0; ; i++) {
		if (sblock_try[i] == -1)
			errx(1, "%s: bad superblock", name);
		rdfs(sblock_try[i], SBLOCKSIZE,  (char *)ffs);
		switch (fs->fs_magic) {
		case FS_UFS2_MAGIC:
			is_ufs2 = 1;
			/* FALLTHROUGH */
		case FS_UFS1_MAGIC:
			break;
		default:
			continue;
		}
		if (is_ufs2 || (fs->fs_flags & FS_FLAGS_UPDATED)) {
			if (fs->fs_sblockloc != sblock_try[i])
				continue;
		} else {
			if (sblock_try[i] == SBLOCK_UFS2)
				continue;
		}
		break;
	}
	//rdfs(SBOFF, SBSIZE, (char *)fs);
	dev_bsize = fs->fs_fsize / fsbtodb(fs, 1);
	for (argc -= 2, argv += 2; argc > 0; argc--, argv++) {
		number = atoi(*argv);
		if (chkuse(number, 1))
			continue;
		if (mknod(*argv, S_IFMT|S_IRUSR|S_IWUSR, dbtofsb(fs, number)) < 0) {
			perror(*argv);
			errs++;
		}
	}
	printf("Don't forget to run ``fsck %s''\n", name);
	exit(errs);
}

int
chkuse(daddr_t blkno, int cnt)
{
	int cg;
	daddr_t fsbn, bn;
	int32_t fsbe;

	fsbn = dbtofsb(fs, blkno);
	fsbe = (int32_t)(fsbn+cnt);
	if (fsbe > fs->fs_size) {
		printf("block %ld out of range of file system\n", blkno);
		return (1);
	}
	cg = dtog(fs, fsbn);
	if (fsbn < cgdmin(fs, cg)) {
		if (cg == 0 || fsbe > cgsblock(fs, cg)) {
			printf("block %ld in non-data area: cannot attach\n",
				blkno);
			return (1);
		}
	} else {
		if (fsbe > cgbase(fs, cg+1)) {
			printf("block %ld in non-data area: cannot attach\n",
				blkno);
			return (1);
		}
	}
	rdfs(fsbtodb(fs, cgtod(fs, cg)), (int)sblock.fs_cgsize,
	    (char *)&acg);
	if (!cg_chkmagic(&acg)) {
		fprintf(stderr, "cg %d: bad magic number\n", cg);
		errs++;
		return (1);
	}
	bn = dtogd(fs, fsbn);
	if (isclr(cg_blksfree(&acg), bn))
		printf("Warning: sector %ld is in use\n", blkno);
	return (0);
}

/*
 * read a block from the file system
 */
void
rdfs(daddr_t bno, int size, char *bf)
{
	int n;

	if (lseek(fsi, (off_t)bno * dev_bsize, SEEK_SET) < 0) {
		printf("seek error: %ld\n", bno);
		perror("rdfs");
		exit(1);
	}
	n = read(fsi, bf, size);
	if (n != size) {
		printf("read error: %ld\n", bno);
		perror("rdfs");
		exit(1);
	}
}

static void
usage(void)
{
	fprintf(stderr, "usage: badsect bbdir blkno [ blkno ]\n");
	exit(1);
}
