/*
 * Copyright (c) 1983, 1993
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

#ifndef lint
static char copyright[] =
"@(#) Copyright (c) 1983, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)tunefs.c	8.3 (Berkeley) 5/3/95";
#endif /* not lint */

/*
 * tunefs: change layout parameters to an existing file system.
 */
#include <sys/param.h>
#include <sys/stat.h>

#include <ufs/ufs/dinode.h>
#include <ufs/ffs/fs.h>
#include <ufs/ffs/ffs_extern.h>
#include <ufs/ufs/ufsmount.h>

#include <errno.h>
#include <err.h>
#include <fcntl.h>
#include <fstab.h>
#include <stdio.h>
#include <paths.h>
#include <stdlib.h>
#include <unistd.h>
#include <util.h>

/* the optimization warning string template */
#define	OPTWARN	"should optimize for %s with minfree %s %d%%"

union {
	struct	fs sb;
	char pad[MAXBSIZE];
} sbun;
#define	sblock sbun.sb

int fi;
long dev_bsize = 512;
int	is_ufs2 = 0;
off_t	sblockloc;
off_t sblock_try[] = SBLOCKSEARCH;

void bwrite(daddr_t, char *, int);
void bread(daddr_t, char *, int);
int	 openpartition(char *, int, char *, size_t);
int  isactive(int, struct statfs *);
void getsb(struct fs *, char *);
void usage(void);

int
main(argc, argv)
	int argc;
	char *argv[];
{
	int	ch, Aflag, Fflag, Nflag, openflags;
	char *special, *name;
	int i;
	bool active;
	char *chg[2], device[MAXPATHLEN];
	struct statfs	sfs;

	Aflag = Fflag = Nflag = 0;
	chg[FS_OPTSPACE] = "space";
	chg[FS_OPTTIME] = "time";

	while ((ch = getopt(argc, argv, "AFNademot:")) != -1) {
		switch (ch) {
		case 'A':
			Aflag++;
			break;

		case 'F':
			Fflag++;
			break;

		case 'N':
			Nflag++;
			break;

		case 'a':
			name = "maximum contiguous block count";
			if (argc < 1)
				errx(10, "-a: missing %s", name);
			argc--, argv++;
			i = atoi(*argv);
			if (i < 1)
				errx(10, "%s must be >= 1 (was %s)", name, *argv);
			warnx("%s changes from %d to %d", name, sblock.fs_maxcontig, i);
			sblock.fs_maxcontig = i;
			break;

		case 'd':
			name = "rotational delay between contiguous blocks";
			if (argc < 1)
				errx(10, "-d: missing %s", name);
			argc--, argv++;
			i = atoi(*argv);
			warnx("%s changes from %dms to %dms", name, sblock.fs_rotdelay, i);
			sblock.fs_rotdelay = i;
			break;

		case 'e':
			name = "maximum blocks per file in a cylinder group";
			if (argc < 1)
				errx(10, "-e: missing %s", name);
			argc--, argv++;
			i = atoi(*argv);
			if (i < 1)
				errx(10, "%s must be >= 1 (was %s)", name, *argv);
			warnx("%s changes from %d to %d", name, sblock.fs_maxbpg, i);
			sblock.fs_maxbpg = i;
			break;

		case 'm':
			name = "minimum percentage of free space";
			if (argc < 1)
				errx(10, "-m: missing %s", name);
			argc--, argv++;
			i = atoi(*argv);
			if (i < 0 || i > 99)
				errx(10, "bad %s (%s)", name, *argv);
			warnx("%s changes from %d%% to %d%%", name, sblock.fs_minfree, i);
			sblock.fs_minfree = i;
			if (i >= MINFREE &&
			sblock.fs_optim == FS_OPTSPACE)
				warnx(OPTWARN, "time", ">=", MINFREE);
			if (i < MINFREE &&
			sblock.fs_optim == FS_OPTTIME)
				warnx(OPTWARN, "space", "<", MINFREE);
			break;

		case 'o':
			name = "optimization preference";
			if (argc < 1)
				errx(10, "-o: missing %s", name);
			argc--, argv++;
			if (strcmp(*argv, chg[FS_OPTSPACE]) == 0)
				i = FS_OPTSPACE;
			else if (strcmp(*argv, chg[FS_OPTTIME]) == 0)
				i = FS_OPTTIME;
			else
				errx(10, "bad %s (options are `space' or `time')", name);
			if (sblock.fs_optim == i) {
				warnx("%s remains unchanged as %s", name, chg[i]);
				break;
			}
			warnx("%s changes from %s to %s", name, chg[sblock.fs_optim],
					chg[i]);
			sblock.fs_optim = i;
			if (sblock.fs_minfree >= MINFREE && i == FS_OPTSPACE)
				warnx(OPTWARN, "time", ">=", MINFREE);
			if (sblock.fs_minfree < MINFREE && i == FS_OPTTIME)
				warnx(OPTWARN, "space", "<", MINFREE);
			break;

		case 't':
			name = "track skew in sectors";
			if (argc < 1)
				errx(10, "-t: missing %s", name);
			argc--, argv++;
			i = atoi(*argv);
			if (i < 0)
				errx(10, "%s: %s must be >= 0", *argv, name);
			warnx("%s changes from %d to %d", name, sblock.fs_trackskew, i);
			sblock.fs_trackskew = i;
			break;

		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;
	if (argc != 1) {
		usage();
	}

	special = argv[0];
	openflags = Nflag ? O_RDONLY : O_RDWR;
	if (Fflag) {
		fi = open(special, openflags);
	} else {
		fi = openpartition(special, openflags, device, sizeof(device));
		special = device;
	}
	if (fi < 0) {
		err(3, "cannot open %s for writing", special);
	}
	active = !Fflag && isactive(fi, &sfs);
	getsb(&sblock, special);

	if (Nflag) {
		fprintf(stdout, "tunefs: current settings\n");
		fprintf(stdout, "\tmaximum contiguous block count %d\n",
		sblock.fs_maxcontig);
		fprintf(stdout, "\trotational delay between contiguous blocks %dms\n",
		sblock.fs_rotdelay);
		fprintf(stdout, "\tmaximum blocks per file in a cylinder group %d\n",
		sblock.fs_maxbpg);
		fprintf(stdout, "\tminimum percentage of free space %d%%\n",
		sblock.fs_minfree);
		fprintf(stdout, "\toptimization preference: %s\n",
				chg[sblock.fs_optim]);
		fprintf(stdout, "\ttrack skew %d sectors\n",
		sblock.fs_trackskew);
		fprintf(stdout, "tunefs: no changes made\n");
		exit(0);
	}

	/* write superblock to original coordinates (use old dev_bsize!) */
	bwrite(sblockloc, (char *)&sblock, SBLOCKSIZE);

	if (active) {
		struct ufs_args args;
		args.fspec = sfs.f_mntfromname;
		if (mount(MOUNT_FFS | MOUNT_UFS, sfs.f_mntonname,
				sfs.f_flags | MNT_UPDATE, &args, sizeof args) == -1) {
			warn("mount");
		} else {
			printf("%s: mount of %s on %s updated\n", getprogname(),
					sfs.f_mntfromname, sfs.f_mntonname);
		}
	}

	dev_bsize = sblock.fs_fsize / fsbtodb(&sblock, 1);

	if (Aflag) {
		for (i = 0; i < sblock.fs_ncg; i++) {
			bwrite(fsbtodb(&sblock, cgsblock(&sblock, i)), (char *)&sblock, SBLOCKSIZE);
		}
	}

	close(fi);
	exit(0);
}

void
usage()
{

	fprintf(stderr, "Usage: tunefs [-AN] tuneup-options special-device\n");
	fprintf(stderr, "where tuneup-options are:\n");
	fprintf(stderr, "\t-d rotational delay between contiguous blocks\n");
	fprintf(stderr, "\t-a maximum contiguous blocks\n");
	fprintf(stderr, "\t-e maximum blocks per file in a cylinder group\n");
	fprintf(stderr, "\t-m minimum percentage of free space\n");
	fprintf(stderr, "\t-o optimization preference (`space' or `time')\n");
	fprintf(stderr, "\t-t track skew in sectors\n");
	exit(2);
}

int
isactive(fd, rsfs)
	int fd;
	struct statfs *rsfs;
{
	struct stat st0, st;
	struct statfs *sfs;

	if (fstat(fd, &st0) == -1) {
		warn("stat");
		return 0;
	}

	if ((n = getmntinfo(&sfs, 0)) == -1) {
		warn("getmntinfo");
		return 0;
	}

	for (int i = 0; i < n; i++) {
		if (stat(sfs[i].f_mntfromname, &st) == -1)
			continue;
		if (st.st_rdev != st0.st_rdev)
			continue;
		*rsfs = sfs[i];
		return 1;
	}
	return 0;
}

void
getsb(fs, file)
	register struct fs *fs;
	char *file;
{
	int i;

	for (i = 0;; i++) {
		if (sblock_try[i] == -1) {
			errx(5, "cannot find filesystem superblock");
		}
		bread(sblock_try[i] / dev_bsize, (char*) fs, SBLOCKSIZE);
		switch (fs->fs_magic) {
		case FS_UFS2_MAGIC:
			is_ufs2 = 1;
			/*FALLTHROUGH*/
		case FS_UFS1_MAGIC:
			break;
		default:
			continue;
		}
		if (!is_ufs2 && sblock_try[i] == SBLOCK_UFS2)
			continue;
		if ((is_ufs2 || (fs->fs_flags & FS_FLAGS_UPDATED))
				&& fs->fs_sblockloc != sblock_try[i])
			continue;
		break;
	}
	dev_bsize = fs->fs_fsize / fsbtodb(fs, 1);
	sblockloc = sblock_try[i] / dev_bsize;
}

void
bwrite(blk, buf, size)
	daddr_t blk;
	char *buf;
	int size;
{

	if (lseek(fi, (off_t)blk * dev_bsize, SEEK_SET) < 0) {
		err(6, "FS SEEK");
	}
	if (write(fi, buf, size) != size) {
		err(7, "FS WRITE");
	}
}

void
bread(bno, buf, cnt)
	daddr_t bno;
	char *buf;
	int cnt;
{
	int i;

	if (lseek(fi, (off_t)bno * dev_bsize, SEEK_SET) < 0) {
		err(6, "FS SEEK");
	}
	if ((i = read(fi, buf, cnt)) != cnt) {
		err(5, "FS READ");
	}
}

int
openpartition(name, flags, device, devicelen)
	char *name;
	int flags;
	char *device;
	size_t devicelen;
{
	struct fstab *fs;
	struct stat st;
	char *special;
	int fd, oerrno;

	fs = getfsfile(name);
	special = fs ? fs->fs_spec : name;
	if (stat(special, &st) < 0) {
		err(1, "%s", special);
	}
	if ((st.st_mode & S_IFMT) != S_IFBLK && (st.st_mode & S_IFMT) != S_IFCHR) {
		errx(10, "%s: not a block or character device", special);
	}
	fd = opendisk(special, flags, device, devicelen, 0);
	if (fd == -1 && errno == ENOENT) {
		oerrno = errno;
		strlcpy(device, special, devicelen);
		errno = oerrno;
	}
	return (fd);
}
