/*
 * Copyright (c) 1987, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Symmetric Computer Systems.
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

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#ifndef lint
static char copyright[] =
"@(#) Copyright (c) 1987, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)disklabel.c	8.4 (Berkeley) 5/4/95";
/* from static char sccsid[] = "@(#)disklabel.c	1.2 (Symmetric) 11/28/85"; */
#endif /* not lint */

#include <sys/param.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>

#define DKTYPENAMES
#define FSTYPENAMES

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <util.h>
//#include <ufs/ufs/dinode.h>
//#include <ufs/ffs/fs.h>

#if HAVE_NBTOOL_CONFIG_H
#include <nbinclude/sys/disklabel.h>
#else
#include <sys/disklabel.h>
#endif /* HAVE_NBTOOL_CONFIG_H */
#include "pathnames.h"
#include "dkcksum.h"

static int		  writelabel(int, char *, struct disklabel *);
static struct disklabel  *readlabel(int);
static void  	          l_perror(char *);
static void              makelabel(const char *, const char *, struct disklabel *);
static struct disklabel  *makebootarea(char *, struct disklabel *, int);
static void	          display(FILE *, struct disklabel *);
static int		  edit(struct disklabel *, int);
static int		  editit(void);
static int               checklabel(struct disklabel *);
static int		  getasciilabel(FILE *, struct disklabel *);
static int		  getasciipartspec(char *, struct disklabel *, int, int);
static void	          	setbootflag(struct disklabel *);
static void  	          Warning(const char *fmt, ...);
static void 	          usage(void);

/*
 * Disklabel: read and write disklabels.
 * The label is usually placed on one of the first sectors of the disk.
 * Many machines also place a bootstrap in the same area,
 * in which case the label is embedded in the bootstrap.
 * The bootstrap source must leave space at the proper offset
 * for the label on such machines.
 */

#ifndef BBSIZE
#define	BBSIZE		BSD_BBSIZE			/* size of boot area, with label */
#endif

#ifndef SBSIZE
#define SBSIZE		BSD_SBSIZE			/* size of super block */
#endif

#define	NUMBOOT	2

#define	DEFEDITOR	_PATH_VI
#define	streq(a,b)	(strcmp(a,b) == 0)

static char	*dkname;
static char	*specname;
static char	tmpfil[] = PATH_TMPFILE;

static struct	disklabel lab;
static char	bootarea[BBSIZE];
static int	rflag;
static int	forceflag;
static u_int32_t start_lba;

#define MAX_PART ('z')
#define MAX_NUM_PARTS (1 + MAX_PART - 'a')
static char    part_size_type[MAX_NUM_PARTS];
static char    part_offset_type[MAX_NUM_PARTS];
static int     part_set[MAX_NUM_PARTS];

#if NUMBOOT > 0
static int		installboot;	/* non-zero if we should install a boot program */
static char	*bootbuf;		/* pointer to buffer with remainder of boot prog */
static int		bootsize;		/* size of remaining boot program */
static char	*xxboot;		/* primary boot */
static char	*bootxx;		/* secondary boot */
static char	boot0[MAXPATHLEN];
static char	boot1[MAXPATHLEN];
#endif

static enum {
	UNSPEC, EDIT, NOWRITE, READ, RESTORE, WRITE, WRITEABLE, WRITEBOOT
} op = UNSPEC;

#ifdef DEBUG
int	debug;
#define OPTIONS	"BNRWb:ders:w"
#else
#define OPTIONS	"BNRWb:ers:w"
#endif

int
main(int argc, char *argv[])
{
	register struct disklabel *lp;
	FILE *t;
	int ch, f, flag, error;
	char *name;
	extern char *optarg;
	extern int optind;

	f = 0;
	error = 0;
	name = NULL;
	while ((ch = getopt(argc, argv, OPTIONS)) != EOF)
		switch (ch) {
#if NUMBOOT > 0
		case 'B':
			++installboot;
			break;
		case 'b':
			xxboot = optarg;
			break;

		case 'f':
			forceflag = 1;
			start_lba = strtoul(optarg, NULL, 0);
			break;

#if NUMBOOT > 1
		case 's':
			bootxx = optarg;
			break;
#endif
#endif
		case 'N':
			if (op != UNSPEC)
				usage();
			op = NOWRITE;
			break;
		case 'R':
			if (op != UNSPEC)
				usage();
			op = RESTORE;
			break;
		case 'W':
			if (op != UNSPEC)
				usage();
			op = WRITEABLE;
			break;
		case 'e':
			if (op != UNSPEC)
				usage();
			op = EDIT;
			break;
		case 'r':
			++rflag;
			break;
		case 'w':
			if (op != UNSPEC)
				usage();
			op = WRITE;
			break;
#ifdef DEBUG
		case 'd':
			debug++;
			break;
#endif
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;
#if NUMBOOT > 0
	if (installboot) {
		rflag++;
		if (op == UNSPEC)
			op = WRITEBOOT;
	} else {
		if (op == UNSPEC)
			op = READ;
		xxboot = bootxx = 0;
	}
#else
	if (op == UNSPEC)
		op = READ;
#endif
	if (argc < 1)
		usage();

	dkname = argv[0];
	specname = dkname;
	f = open(specname, op == READ ? O_RDONLY : O_RDWR);
	if (f < 0) {
		err(4, "%s", specname);
	}

	switch (op) {

	case UNSPEC:
		break;

	case EDIT:
		if (argc != 1)
			usage();
		lp = readlabel(f);
		error = edit(lp, f);
		break;

	case NOWRITE:
		flag = 0;
		if (ioctl(f, DIOCWLABEL, (char*) &flag) < 0)
			err(4, "ioctl DIOCWLABEL");
		break;

	case READ:
		if (argc != 1)
			usage();
		lp = readlabel(f);
		display(stdout, lp);
		error = checklabel(lp);
		break;

	case RESTORE:
#if NUMBOOT > 0
		if (installboot && argc == 3) {
			makelabel(argv[2], 0, &lab);
			argc--;
			/*
			 * We only called makelabel() for its side effect
			 * of setting the bootstrap file names.  Discard
			 * all changes to `lab' so that all values in the
			 * final label come from the ASCII label.
			 */
			bzero((char *)&lab, sizeof(lab));
		}
#endif
		if (argc != 2)
			usage();
		if (!(t = fopen(argv[1], "r")))
			err(4, "%s", argv[1]);
		if (!getasciilabel(t, &lab))
			exit(1);
		lp = makebootarea(bootarea, &lab, f);
		*lp = lab;
		error = writelabel(f, bootarea, lp);
		break;

	case WRITE:
		if (argc == 3) {
			name = argv[2];
			argc--;
		}
		if (argc != 2)
			usage();
		makelabel(argv[1], name, &lab);
		lp = makebootarea(bootarea, &lab, f);
		*lp = lab;
		if (checklabel(lp) == 0)
			error = writelabel(f, bootarea, lp);
		break;

	case WRITEABLE:
		flag = 1;
		if (ioctl(f, DIOCWLABEL, (char*) &flag) < 0)
			err(4, "ioctl DIOCWLABEL");
		break;

#if NUMBOOT > 0
	case WRITEBOOT: {
		struct disklabel tlab;

		lp = readlabel(f);
		tlab = *lp;
		if (argc == 2)
			makelabel(argv[1], 0, &lab);
		lp = makebootarea(bootarea, &lab, f);
		*lp = tlab;
		if (checklabel(lp) == 0)
			error = writelabel(f, bootarea, lp);
		break;
	}
#endif
	}
	exit(error);
}

static void
fixlabel(f, lp, writeadj)
  struct disklabel *lp;
  int f, writeadj;
{
	struct partition *dp;
	int i;

	for (i = 0; i < lp->d_npartitions; i++) {
		if (i == RAW_PART)
			continue;
		if (lp->d_partitions[i].p_size)
			return;
	}

	dp = &lp->d_partitions[0];
	dp->p_offset = BBSIZE / lp->d_secsize;
	dp->p_size = lp->d_secperunit - dp->p_offset;
}

/*
 * Construct a prototype disklabel from /etc/disktab.  As a side
 * effect, set the names of the primary and secondary boot files
 * if specified.
 */
static void
makelabel(type, name, lp)
	const char *type, *name;
	register struct disklabel *lp;
{
	struct disklabel *dp;

	dp = getdiskbyname(type);
	if (dp == NULL) {
		errx(1, "%s: unknown disk type", type);
	}
	*lp = *dp;
	/* d_packname is union d_boot[01], so zero */
	memset(lp->d_packname, 0, sizeof(lp->d_packname));
	if (name) {
		strncpy(lp->d_packname, name, sizeof(lp->d_packname));
	}
}

static int
writelabel(f, boot, lp)
	int f;
	char *boot;
	register struct disklabel *lp;
{
	int flag;
	int i;

	setbootflag(lp);
	lp->d_magic = DISKMAGIC;
	lp->d_magic2 = DISKMAGIC;
	lp->d_checksum = 0;
	lp->d_checksum = dkcksum(lp);
	if (rflag) {
		/*
		 * First set the kernel disk label,
		 * then write a label to the raw disk.
		 * If the SDINFO ioctl fails because it is unimplemented,
		 * keep going; otherwise, the kernel consistency checks
		 * may prevent us from changing the current (in-core)
		 * label.
		 */
		if (ioctl(f, DIOCSDINFO, lp) < 0 &&
		errno != ENODEV && errno != ENOTTY) {
			l_perror("ioctl DIOCSDINFO");
			return (1);
		}
		lseek(f, (off_t) 0, SEEK_SET);

		/*
		 * write enable label sector before write (if necessary),
		 * disable after writing.
		 */
		flag = 1;
		if (ioctl(f, DIOCWLABEL, &flag) < 0)
			warn("ioctl DIOCWLABEL");
		fixlabel(f, lp, 1);
		i = write(f, boot, lp->d_bbsize);
		fixlabel(f, lp, 0);
		if (i != ((ssize_t)lp->d_bbsize)) {
			warn("write");
			return (1);
		}
#if NUMBOOT > 0
		/*
		 * Output the remainder of the disklabel
		 */
		if (bootbuf) {
			fixlabel(f, lp, 1);
			i = write(f, bootbuf, bootsize);
			fixlabel(f, lp, 0);
			if (i != bootsize) {
				warn("write");
				return (1);
			}
		}
#endif
		flag = 0;
		(void) ioctl(f, DIOCWLABEL, &flag);
	} else if (ioctl(f, DIOCWDINFO, lp) < 0) {
		l_perror("ioctl DIOCWDINFO");
		return (1);
	}
	return (0);
}

static void
l_perror(s)
	char *s;
{
	fprintf(stderr, "disklabel: %s: ", s);

	switch (errno) {

	case ESRCH:
		fprintf(stderr, "No disk label on disk;\n");
		fprintf(stderr, "use \"disklabel -r\" to install initial label\n");
		break;

	case EINVAL:
		fprintf(stderr, "Label magic number or checksum is wrong!\n");
		fprintf(stderr, "(disklabel or kernel is out of date?)\n");
		break;

	case EBUSY:
		fprintf(stderr, "Open partition would move or shrink\n");
		break;

	case EXDEV:
		fprintf(stderr, "Labeled partition or 'a' partition must start at beginning of disk\n");
		break;

	default:
		warn(NULL);
		break;
	}
}

/*
 * Fetch disklabel for disk.
 * Use ioctl to get label unless -r flag is given.
 */
static struct disklabel *
readlabel(f)
	int f;
{
	register struct disklabel *lp;
	int r;

	if (rflag) {
		r = read(f, bootarea, BBSIZE);
		if (r < BBSIZE) {
			err(4, "%s", specname);
		}
		for (lp = (struct disklabel*) bootarea;
				lp <= (struct disklabel*) (bootarea + BBSIZE - sizeof(*lp));
				lp = (struct disklabel*) ((char*) lp + 16)) {
			if (lp->d_magic == DISKMAGIC && lp->d_magic2 == DISKMAGIC) {
				break;
			}
		}
		if (lp > (struct disklabel*) (bootarea + BBSIZE - sizeof(*lp))
				|| lp->d_magic != DISKMAGIC || lp->d_magic2 != DISKMAGIC || dkcksum(lp) != 0) {
			errx(1, "bad pack magic number (label is damaged, or pack is unlabeled)");
		}
		fixlabel(f, lp, 0);
	} else {
		lp = &lab;
		if (ioctl(f, DIOCGDINFO, lp) < 0) {
			l_perror("ioctl DIOCGDINFO");
			exit(4);
		}
	}
	return (lp);
}

/*
 * Construct a bootarea (d_bbsize bytes) in the specified buffer ``boot''
 * Returns a pointer to the disklabel portion of the bootarea.
 */
static struct disklabel *
makebootarea(boot, dp, f)
	char *boot;
	register struct disklabel *dp;
	int f;
{
	struct disklabel *lp;
	register char *p;
	int b;
#if NUMBOOT > 0
	char *dkbasename;
	struct stat sb;
#endif

	/* XXX */
	if (dp->d_secsize == 0) {
		dp->d_secsize = DEV_BSIZE;
		dp->d_bbsize = BBSIZE;
	}
	lp = (struct disklabel*) (boot + (LABELSECTOR * dp->d_secsize) + LABELOFFSET);
	memset(lp, 0, sizeof *lp);
#if NUMBOOT > 0
	/*
	 * If we are not installing a boot program but we are installing a
	 * label on disk then we must read the current bootarea so we don't
	 * clobber the existing boot.
	 */
	if (!installboot) {
		if (rflag) {
			if (read(f, boot, BBSIZE) < BBSIZE)
				err(4, "%s", specname);
			memset(lp, 0, sizeof *lp);
		}
		return (lp);
	}
	/*
	 * We are installing a boot program.  Determine the name(s) and
	 * read them into the appropriate places in the boot area.
	 */
	if (!xxboot || !bootxx) {
		if (!xxboot) {
			sprintf(boot0, "%s/boot1", _PATH_BOOTDIR);
			xxboot = boot0;
		}
#if NUMBOOT > 1
		if (!bootxx) {
			sprintf(boot1, "%s/boot2", _PATH_BOOTDIR);
			bootxx = boot1;
		}
#endif
	}
#ifdef DEBUG
	if (debug)
		fprintf(stderr, "bootstraps: xxboot = %s, bootxx = %s\n", xxboot,
				bootxx ? bootxx : "NONE");
#endif

	/*
	 * Strange rules:
	 * 1. One-piece bootstrap (hp300/hp800)
	 *	up to d_bbsize bytes of ``xxboot'' go in bootarea, the rest
	 *	is remembered and written later following the bootarea.
	 * 2. Two-piece bootstraps (vax/i386?/mips?)
	 *	up to d_secsize bytes of ``xxboot'' go in first d_secsize
	 *	bytes of bootarea, remaining d_bbsize-d_secsize filled
	 *	from ``bootxx''.
	 */
	b = open(xxboot, O_RDONLY);
	if (b < 0)
		err(4, "%s", xxboot);
#if NUMBOOT > 1
	if (read(b, boot, (int) dp->d_secsize) < 0) {
		err(4, "%s", xxboot);
	}
	(void)close(b);
	b = open(bootxx, O_RDONLY);
	if (b < 0)
		err(4, "%s", bootxx);
	if (fstat(b, &sb) != 0) {
		err(4, "%s", bootxx);
	}
	if (dp->d_secsize + sb.st_size > dp->d_bbsize) {
		errx(4, "%s too large", bootxx);
	}
	if (read(b, &boot[dp->d_secsize], (int) (dp->d_bbsize - dp->d_secsize)) < 0) {
		err(4, "%s", bootxx);
	}
#else
	if (read(b, boot, (int) dp->d_bbsize) < 0)
		err(4, "%s", xxboot);
	if (fstat(b, &sb) != 0) {
		err(4, "%s", xxboot);
	}
	bootsize = (int) sb.st_size - dp->d_bbsize;
	if (bootsize > 0) {
		/* XXX assume d_secsize is a power of two */
		//bootsize = (bootsize + dp->d_secsize - 1) & ~(dp->d_secsize - 1);
		bootsize = roundup2(bootsize, dp->d_secsize);
		bootbuf = (char*) malloc((size_t) bootsize);
		if (bootbuf == 0)
			err(4, "%s", xxboot);
		if (read(b, bootbuf, bootsize) < 0) {
			free(bootbuf);
			err(4, "%s", xxboot);
		}
	}
#endif
	close(b);
#endif
	/*
	 * Make sure no part of the bootstrap is written in the area
	 * reserved for the label.
	 */
	for (p = (char*) lp; p < (char*) lp + sizeof(struct disklabel); p++)
		if (*p) {
			errx(2, "Bootstrap doesn't leave room for disk label");
		}
	return (lp);
}

static void
display(f, lp)
	FILE *f;
	register struct disklabel *lp;
{
	register int i, j;
	register struct partition *pp;

	fprintf(f, "# %s:\n", specname);
	if ((unsigned) lp->d_type < DKMAXTYPES)
		fprintf(f, "type: %s\n", dktypenames[lp->d_type]);
	else
		fprintf(f, "type: %d\n", lp->d_type);
	fprintf(f, "disk: %.*s\n", (int)sizeof(lp->d_typename), lp->d_typename);
	fprintf(f, "label: %.*s\n", (int)sizeof(lp->d_packname), lp->d_packname);
	fprintf(f, "flags:");
	if (lp->d_flags & D_REMOVABLE)
		fprintf(f, " removeable");
	if (lp->d_flags & D_ECC)
		fprintf(f, " ecc");
	if (lp->d_flags & D_BADSECT)
		fprintf(f, " badsect");
	fprintf(f, "\n");
	fprintf(f, "bytes/sector: %d\n", lp->d_secsize);
	fprintf(f, "sectors/track: %d\n", lp->d_nsectors);
	fprintf(f, "tracks/cylinder: %d\n", lp->d_ntracks);
	fprintf(f, "sectors/cylinder: %d\n", lp->d_secpercyl);
	fprintf(f, "cylinders: %d\n", lp->d_ncylinders);
	fprintf(f, "rpm: %d\n", lp->d_rpm);
	fprintf(f, "interleave: %d\n", lp->d_interleave);
	fprintf(f, "trackskew: %d\n", lp->d_trackskew);
	fprintf(f, "cylinderskew: %d\n", lp->d_cylskew);
	fprintf(f, "headswitch: %d\t\t# milliseconds\n", lp->d_headswitch);
	fprintf(f, "track-to-track seek: %d\t# milliseconds\n", lp->d_trkseek);
	fprintf(f, "drivedata: ");
	for (i = NDDATA - 1; i >= 0; i--)
		if (lp->d_drivedata[i])
			break;
	if (i < 0)
		i = 0;
	for (j = 0; j <= i; j++)
		fprintf(f, "%d ", lp->d_drivedata[j]);
	fprintf(f, "\n\n%d partitions:\n", lp->d_npartitions);
	fprintf(f, "#        size   offset    fstype   [fsize bsize   cpg]\n");
	pp = lp->d_partitions;
	for (i = 0; i < lp->d_npartitions; i++, pp++) {
		if (pp->p_size) {
			fprintf(f, "  %c: %8d %8d  ", 'a' + i, pp->p_size, pp->p_offset);
			if ((unsigned) pp->p_fstype < FSMAXTYPES)
				fprintf(f, "%8.8s", fstypenames[pp->p_fstype]);
			else
				fprintf(f, "%8d", pp->p_fstype);
			switch (pp->p_fstype) {

			case FS_UNUSED: /* XXX */
				fprintf(f, "    %5d %5d %5.5s ", pp->p_fsize,
						pp->p_fsize * pp->p_frag, "");
				break;

			case FS_BSDFFS:
				fprintf(f, "    %5d %5d %5d ", pp->p_fsize,
						pp->p_fsize * pp->p_frag, pp->p_cpg);
				break;

			case FS_BSDLFS:
				fprintf(f, "    %5lu %5lu %5d",
				    (u_long)pp->p_fsize,
				    (u_long)(pp->p_fsize * pp->p_frag),
				    pp->p_cpg);
				break;

			default:
				fprintf(f, "%20.20s", "");
				break;
			}
			fprintf(f, "\t# (Cyl. %4d", pp->p_offset / lp->d_secpercyl);
			if (pp->p_offset % lp->d_secpercyl)
				putc('*', f);
			else
				putc(' ', f);
			fprintf(f, "- %d",
					(pp->p_offset + pp->p_size + lp->d_secpercyl - 1)
							/ lp->d_secpercyl - 1);
			if (pp->p_size % lp->d_secpercyl)
				putc('*', f);
			fprintf(f, ")\n");
		}
	}
	fflush(f);
}

static int
edit(lp, f)
	struct disklabel *lp;
	int f;
{
	register int c, fp;
	struct disklabel label;
	FILE *fd;

	if ((fp = mkstemp(tmpfil)) == -1 ||
	    (fd = fdopen(fp, "w")) == NULL) {
		warnx("can't create %s", tmpfil);
		return (1);
	}

	display(fd, lp);
	fclose(fd);
	for (;;) {
		if (!editit())
			break;
		fd = fopen(tmpfil, "r");
		if (fd == NULL) {
			warnx("can't reopen %s for reading", tmpfil);
			break;
		}
		memset(&label, 0, sizeof(label));
		if (getasciilabel(fd, &label)) {
			*lp = label;
			if (writelabel(f, bootarea, lp) == 0) {
				fclose(fd);
				unlink(tmpfil);
				return (0);
			}
		}
		fclose(fd);
		printf("re-edit the label? [y]: "); 
		fflush(stdout);
		c = getchar();
		if (c != EOF && c != (int) '\n')
			while (getchar() != (int) '\n')
				;
		if (c == (int) 'n')
			break;
	}
	unlink(tmpfil);
	return (1);
}

static int
editit(void)
{
	register int pid, xpid;
	int stat, retval;
	register char *ed;
	sigset_t set, oset;

	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGHUP);
	sigaddset(&set, SIGQUIT);
	sigprocmask(SIG_BLOCK, &set, &oset);
	while ((pid = fork()) < 0) {
		if (errno != EAGAIN) {
			sigprocmask(SIG_SETMASK, &oset, (sigset_t *)0);
			warn("fork");
			return(0);
		}
		sleep(1);
	}
	if (pid == 0) {
		sigprocmask(SIG_SETMASK, &oset, NULL);
		setgid(getgid());
		setuid(getuid());
		if ((ed = getenv("EDITOR")) == (char*) 0)
			ed = DEFEDITOR;
		retval = execlp(ed, ed, tmpfil, (void *)0);
		if (retval == -1) {
		  err(1, "%s", ed);
		}
	}
	while ((xpid = wait(&stat)) >= 0)
		if (xpid == pid)
			break;
	sigprocmask(SIG_SETMASK, &oset, NULL);
	return (!stat);
}

static char *
skip(cp)
	register char *cp;
{
	while (*cp != '\0' && isspace(*cp))
		cp++;
	if (*cp == '\0' || *cp == '#')
		return ((char*) NULL);
	return (cp);
}

static char *
word(cp)
	register char *cp;
{
	register char c;

	while (*cp != '\0' && !isspace(*cp) && *cp != '#')
		cp++;
	if ((c = *cp) != '\0') {
		*cp++ = '\0';
		if (c != '#')
			return (skip(cp));
	}
	return ((char*) NULL);
}

#define NXTNUM(n) do { 		                                            \
	if (tp == NULL) { 		                                            \
		fprintf(stderr, "line %d: too few numeric fields\n", lineno);   \
		return (1); 		                                            \
	} else { 				                                            \
		cp = tp, tp = word(cp);                                         \
		(n) = atoi(cp);                                                 \
	} 						                                            \
} while (0)


/* retain 1 character following number */
#define NXTWORD(w,n) do {                                               \
	if (tp == NULL) {                                                   \
		fprintf(stderr, "line %d: too few numeric fields\n", lineno);   \
		return (1);                                                     \
	} else {                                                            \
		char *tmp;                                                      \
		cp = tp, tp = word(cp);                                         \
		(n) = atoi(cp);                                                 \
		if (tmp) (w) = *tmp;                                            \
	}                                                                   \
} while (0)

/*
 * Read an ascii label in from fd f,
 * in the same format as that put out by display(),
 * and fill in lp.
 */
static int
getasciilabel(f, lp)
	FILE	*f;
	register struct disklabel *lp;
{
	const char *const *cpp, *s; 
    register char *cp;
	register struct partition *pp;
	char *tp, line[BUFSIZ];
	int v, lineno = 0, errors = 0;
	u_int part;

	lp->d_bbsize = BBSIZE; /* XXX */
	lp->d_sbsize = SBSIZE; /* XXX */
	while (fgets(line, sizeof(line) - 1, f)) {
		lineno++;
		if (cp == strchr(line, '\n'))
			*cp = '\0';
		cp = skip(line);
		if (cp == NULL)
			continue;
		tp = strchr(cp, ':');
		if (tp == NULL) {
			fprintf(stderr, "line %d: syntax error\n", lineno);
			errors++;
			continue;
		}
		*tp++ = '\0', tp = skip(tp);
		if (streq(cp, "type")) {
			if (tp == NULL)
				tp = "unknown";
			cpp = dktypenames;
			for (; cpp < &dktypenames[DKMAXTYPES]; cpp++)
				if ((s = *cpp) && streq(s, tp)) {
					lp->d_type = cpp - dktypenames;
					break;
				}
			v = atoi(tp);
			if ((unsigned) v >= DKMAXTYPES)
				fprintf(stderr, "line %d:%s %d\n", lineno,
						"Warning, unknown disk type", v);
			lp->d_type = v;
			continue;
		}
		if (streq(cp, "flags")) {
			for (v = 0; (cp = tp) && *cp != '\0';) {
				tp = word(cp);
				if (streq(cp, "removeable"))
					v |= D_REMOVABLE;
				else if (streq(cp, "ecc"))
					v |= D_ECC;
				else if (streq(cp, "badsect"))
					v |= D_BADSECT;
				else {
					fprintf(stderr, "line %d: %s: bad flag\n", lineno, cp);
					errors++;
				}
			}
			lp->d_flags = v;
			continue;
		}
		if (streq(cp, "drivedata")) {
			register int i;

			for (i = 0; (cp = tp) && *cp != '\0' && i < NDDATA;) {
				lp->d_drivedata[i++] = atoi(cp);
				tp = word(cp);
			}
			continue;
		}
		if (sscanf(cp, "%d partitions", &v) == 1) {
			if (v == 0 || (unsigned) v > MAXPARTITIONS) {
				fprintf(stderr, "line %d: bad # of partitions\n", lineno);
				lp->d_npartitions = MAXPARTITIONS;
				errors++;
			} else
				lp->d_npartitions = v;
			continue;
		}
		if (tp == NULL)
			tp = "";
		if (streq(cp, "disk")) {
			strncpy(lp->d_typename, tp, sizeof(lp->d_typename));
			continue;
		}
		if (streq(cp, "label")) {
			strncpy(lp->d_packname, tp, sizeof(lp->d_packname));
			continue;
		}
		if (streq(cp, "bytes/sector")) {
			v = atoi(tp);
			if (v <= 0 || (v % 512) != 0) {
				fprintf(stderr, "line %d: %s: bad sector size\n", lineno, tp);
				errors++;
			} else
				lp->d_secsize = v;
			continue;
		}
		if (streq(cp, "sectors/track")) {
			v = atoi(tp);
			if (v <= 0) {
				fprintf(stderr, "line %d: %s: bad %s\n", lineno, tp, cp);
				errors++;
			} else
				lp->d_nsectors = v;
			continue;
		}
		if (streq(cp, "sectors/cylinder")) {
			v = atoi(tp);
			if (v <= 0) {
				fprintf(stderr, "line %d: %s: bad %s\n", lineno, tp, cp);
				errors++;
			} else
				lp->d_secpercyl = v;
			continue;
		}
		if (streq(cp, "tracks/cylinder")) {
			v = atoi(tp);
			if (v <= 0) {
				fprintf(stderr, "line %d: %s: bad %s\n", lineno, tp, cp);
				errors++;
			} else
				lp->d_ntracks = v;
			continue;
		}
		if (streq(cp, "cylinders")) {
			v = atoi(tp);
			if (v <= 0) {
				fprintf(stderr, "line %d: %s: bad %s\n", lineno, tp, cp);
				errors++;
			} else
				lp->d_ncylinders = v;
			continue;
		}
		if (streq(cp, "rpm")) {
			v = atoi(tp);
			if (v <= 0) {
				fprintf(stderr, "line %d: %s: bad %s\n", lineno, tp, cp);
				errors++;
			} else
				lp->d_rpm = v;
			continue;
		}
		if (streq(cp, "interleave")) {
			v = atoi(tp);
			if (v <= 0) {
				fprintf(stderr, "line %d: %s: bad %s\n", lineno, tp, cp);
				errors++;
			} else
				lp->d_interleave = v;
			continue;
		}
		if (streq(cp, "trackskew")) {
			v = atoi(tp);
			if (v < 0) {
				fprintf(stderr, "line %d: %s: bad %s\n", lineno, tp, cp);
				errors++;
			} else
				lp->d_trackskew = v;
			continue;
		}
		if (streq(cp, "cylinderskew")) {
			v = atoi(tp);
			if (v < 0) {
				fprintf(stderr, "line %d: %s: bad %s\n", lineno, tp, cp);
				errors++;
			} else
				lp->d_cylskew = v;
			continue;
		}
		if (streq(cp, "headswitch")) {
			v = atoi(tp);
			if (v < 0) {
				fprintf(stderr, "line %d: %s: bad %s\n", lineno, tp, cp);
				errors++;
			} else
				lp->d_headswitch = v;
			continue;
		}
		if (streq(cp, "track-to-track seek")) {
			v = atoi(tp);
			if (v < 0) {
				fprintf(stderr, "line %d: %s: bad %s\n", lineno, tp, cp);
				errors++;
			} else
				lp->d_trkseek = v;
			continue;
		}
		/* the ':' was removed above */
		if (*cp < 'a' || *cp > MAX_PART || cp[1] != '\0') {
			fprintf(stderr, "line %d: %s: Unknown disklabel field\n", lineno, cp);
			errors++;
			continue;
		}

		/* Process a partition specification line. */
		part = *cp - 'a';
		if (part > lp->d_npartitions) {
			fprintf(stderr, "line %d: partition name out of range a-%c: %s\n", lineno, 'a' + lp->d_npartitions - 1, cp);
			errors++;
			continue;
		}
		part_set[part] = 1;

		if (getasciipartspec(tp, lp, part, lineno) != 0) {
			errors++;
			break;
		}
	}
	errors += checklabel(lp);
	return (errors == 0);
}

/*
 * Read a partition line into partition `part' in the specified disklabel.
 * Return 0 on success, 1 on failure.
 */
static int
getasciipartspec(tp, lp, part, lineno)
	char *tp;
	struct disklabel *lp;
	int part, lineno;
{
	struct partition *pp;
	char *cp, *endp;
	const char *const *cpp;
	u_long v;

	pp = &lp->d_partitions[part];
	cp = NULL;

	v = 0;
	NXTWORD(part_size_type[part], v);
	if (v == 0 && part_size_type[part] != '*') {
		fprintf(stderr, "line %d: %s: bad partition size\n", lineno, cp);
		return (1);
	}
	pp->p_size = v;

	v = 0;
	NXTWORD(part_offset_type[part], v);
	if (v == 0 && part_offset_type[part] != '*'
			&& part_offset_type[part] != '\0') {
		fprintf(stderr, "line %d: %s: bad partition offset\n", lineno, cp);
		return (1);
	}
	pp->p_offset = v;
	if (tp == NULL) {
		fprintf(stderr, "line %d: missing file system type\n", lineno);
		return (1);
	}
	cp = tp, tp = word(cp);
	for (cpp = fstypenames; cpp < &fstypenames[FSMAXTYPES]; cpp++)
		if (*cpp && !strcmp(*cpp, cp))
			break;
	if (*cpp != NULL) {
		pp->p_fstype = cpp - fstypenames;
	} else {
		if (isdigit(*cp)) {
			errno = 0;
			v = strtoul(cp, &endp, 10);
			if (errno != 0 || *endp != '\0')
				v = FSMAXTYPES;
		} else
			v = FSMAXTYPES;
		if (v >= FSMAXTYPES) {
			fprintf(stderr, "line %d: Warning, unknown file system type %s\n",
					lineno, cp);
			v = FS_UNUSED;
		}
		pp->p_fstype = v;
	}

	switch (pp->p_fstype) {
	case FS_UNUSED:
	case FS_BSDFFS:
	case FS_BSDLFS:
		/* accept defaults for fsize/frag/cpg */
		if (tp) {
			NXTNUM(pp->p_fsize);
			if (pp->p_fsize == 0)
				break;
			NXTNUM(v);
			pp->p_frag = v / pp->p_fsize;
			if (tp != NULL)
				NXTNUM(pp->p_cpg);
		}
		/* else default to 0's */
		break;
	default:
		break;
	}
	return (0);
}

/*
 * Check disklabel for errors and fill in
 * derived fields according to supplied values.
 */
static int
checklabel(lp)
	register struct disklabel *lp;
{
	register struct partition *pp, *pp2;
	int i, j, errors = 0;
	char part;
	unsigned long base_offset, needed, total_size, total_percent, current_offset;
	int seen_default_offset;
	long free_space;
	int hog_part;

	if (lp->d_secsize == 0) {
		fprintf(stderr, "sector size %d\n", lp->d_secsize);
		return (1);
	}
	if (lp->d_nsectors == 0) {
		fprintf(stderr, "sectors/track %d\n", lp->d_nsectors);
		return (1);
	}
	if (lp->d_ntracks == 0) {
		fprintf(stderr, "tracks/cylinder %d\n", lp->d_ntracks);
		return (1);
	}
	if (lp->d_ncylinders == 0) {
		fprintf(stderr, "cylinders/unit %d\n", lp->d_ncylinders);
		errors++;
	}
	if (lp->d_rpm == 0)
		Warning("revolutions/minute %d", lp->d_rpm);
	if (lp->d_secpercyl == 0)
		lp->d_secpercyl = lp->d_nsectors * lp->d_ntracks;
	if (lp->d_secperunit == 0)
		lp->d_secperunit = lp->d_secpercyl * lp->d_ncylinders;
	if (lp->d_bbsize == 0) {
		fprintf(stderr, "boot block size %d\n", lp->d_bbsize);
		errors++;
	} else if (lp->d_bbsize % lp->d_secsize)
		Warning("boot block size %% sector-size != 0");
	if (lp->d_sbsize == 0) {
		fprintf(stderr, "super block size %d\n", lp->d_sbsize);
		errors++;
	} else if (lp->d_sbsize % lp->d_secsize)
		Warning("super block size %% sector-size != 0");
	if (lp->d_npartitions > MAXPARTITIONS)
		Warning("number of partitions (%d) > MAXPARTITIONS (%d)",
				lp->d_npartitions, MAXPARTITIONS);

	/* first allocate space to the partitions, then offsets */
	total_percent = 0; /* in percent */
	hog_part = -1;
	for (i = 0; i < lp->d_npartitions; i++) {
		pp = &lp->d_partitions[i];
		if (part_set[i]) {
			if (part_size_type[i] == '*') {
				if (i == RAW_PART) {
					pp->p_size = lp->d_secperunit;
				} else {
					if (part_offset_type[i] != '*') {
						if (total_size < pp->p_offset) {
							total_size = pp->p_offset;
						}
					}
					if (hog_part != -1) {
						Warning("Too many '*' partitions (%c and %c)",
								hog_part + 'a', i + 'a');
					} else {
						hog_part = i;
					}
				}
			} else {
				off_t size;

				size = pp->p_size;
				switch (part_size_type[i]) {
				case '%':
					total_percent += size;
					break;
				case 't':
				case 'T':
					size *= 1024ULL;
					/* FALLTHROUGH */
				case 'g':
				case 'G':
					size *= 1024ULL;
					/* FALLTHROUGH */
				case 'm':
				case 'M':
					size *= 1024ULL;
					/* FALLTHROUGH */
				case 'k':
				case 'K':
					size *= 1024ULL;
					break;
				case '\0':
					break;
				default:
					Warning("unknown size specifier '%c' (K/M/G/T are valid)",
							part_size_type[i]);
					break;
				}
				/* don't count %'s yet */
				if (part_size_type[i] != '%') {
					/*
					 * for all not in sectors, convert to
					 * sectors
					 */
					if (part_size_type[i] != '\0') {
						if (size % lp->d_secsize != 0)
							Warning(
									"partition %c not an integer number of sectors",
									i + 'a');
						size /= lp->d_secsize;
						pp->p_size = size;
					}
					/* else already in sectors */
					if (i != RAW_PART) {
						total_size += size;
					}
				}
			}
		}
	}

	/* Find out the total free space, excluding the boot block area. */
	base_offset = BBSIZE / lp->d_secsize;
	free_space = 0;
	for (i = 0; i < lp->d_npartitions; i++) {
		pp = &lp->d_partitions[i];
		if (!part_set[i] || i == RAW_PART || part_size_type[i] == '%'
				|| part_size_type[i] == '*') {
			continue;
		}
		if (pp->p_offset > base_offset) {
			free_space += pp->p_offset - base_offset;
		}
		if (pp->p_offset + pp->p_size > base_offset) {
			base_offset = pp->p_offset + pp->p_size;
		}
	}

	if (base_offset < lp->d_secperunit)
		free_space += lp->d_secperunit - base_offset;

	/* handle % partitions - note %'s don't need to add up to 100! */
	if (total_percent != 0) {
		if (total_percent > 100) {
			fprintf(stderr, "total percentage %lu is greater than 100\n",
					total_percent);
			errors++;
		}

		if (free_space > 0) {
			for (i = 0; i < lp->d_npartitions; i++) {
				pp = &lp->d_partitions[i];
				if (part_set[i] && part_size_type[i] == '%') {
					/* careful of overflows! and integer roundoff */
					pp->p_size = ((double) pp->p_size / 100) * free_space;
					total_size += pp->p_size;

					/* FIX we can lose a sector or so due to roundoff per
					 partition.  A more complex algorithm could avoid that */
				}
			}
		} else {
			fprintf(stderr,
					"%ld sectors available to give to '*' and '%%' partitions\n",
					free_space);
			errors++;
			/* fix?  set all % partitions to size 0? */
		}
	}

	/* give anything remaining to the hog partition */
	if (hog_part != -1) {
		/*
		 * Find the range of offsets usable by '*' partitions around
		 * the hog partition and how much space they need.
		 */
		needed = 0;
		base_offset = BBSIZE / lp->d_secsize;
		for (i = hog_part - 1; i >= 0; i--) {
			pp = &lp->d_partitions[i];
			if (!part_set[i] || i == RAW_PART)
				continue;
			if (part_offset_type[i] == '*') {
				needed += pp->p_size;
				continue;
			}
			base_offset = pp->p_offset + pp->p_size;
			break;
		}
		current_offset = lp->d_secperunit;
		for (i = lp->d_npartitions - 1; i > hog_part; i--) {
			pp = &lp->d_partitions[i];
			if (!part_set[i] || i == RAW_PART)
				continue;
			if (part_offset_type[i] == '*') {
				needed += pp->p_size;
				continue;
			}
			current_offset = pp->p_offset;
		}

		if (current_offset - base_offset <= needed) {
			fprintf(stderr, "Cannot find space for partition %c\n",
					hog_part + 'a');
			fprintf(stderr, "Need more than %lu sectors between %lu and %lu\n",
					needed, base_offset, current_offset);
			errors++;
			lp->d_partitions[hog_part].p_size = 0;
		} else {
			lp->d_partitions[hog_part].p_size = current_offset - base_offset
					- needed;
			total_size += lp->d_partitions[hog_part].p_size;
		}
	}

	/* Now set the offsets for each partition */
	current_offset = BBSIZE / lp->d_secsize; /* in sectors */
	seen_default_offset = 0;
	for (i = 0; i < lp->d_npartitions; i++) {
		part = 'a' + i;
		pp = &lp->d_partitions[i];
		if (part_set[i]) {
			if (part_offset_type[i] == '*') {
				if (i == RAW_PART) {
					pp->p_offset = 0;
				} else {
					pp->p_offset = current_offset;
					seen_default_offset = 1;
				}
			} else {
				/* allow them to be out of order for old-style tables */
				if (pp->p_offset < current_offset&&
				seen_default_offset && i != RAW_PART &&
				pp->p_fstype != FS_VINUM) {
					fprintf(stderr,
							"Offset %ld for partition %c overlaps previous partition which ends at %lu\n",
							(long) pp->p_offset, i + 'a', current_offset);
					fprintf(stderr,
							"Labels with any *'s for offset must be in ascending order by sector\n");
					errors++;
				} else if (pp->p_offset != current_offset && i != RAW_PART
						&& seen_default_offset) {
					/*
					 * this may give unneeded warnings if
					 * partitions are out-of-order
					 */
					Warning(
							"Offset %ld for partition %c doesn't match expected value %ld",
							(long) pp->p_offset, i + 'a', current_offset);
				}
			}
			if (i != RAW_PART)
				current_offset = pp->p_offset + pp->p_size;
		}
	}

	for (i = 0; i < lp->d_npartitions; i++) {
		part = 'a' + i;
		pp = &lp->d_partitions[i];
		if (pp->p_size == 0 && pp->p_offset != 0)
			Warning("partition %c: size 0, but offset %lu", part,
					(u_long) pp->p_offset);
#ifdef notdef
		if (pp->p_size % lp->d_secpercyl)
			Warning("partition %c: size %% cylinder-size != 0", part);
		if (pp->p_offset % lp->d_secpercyl)
			Warning("partition %c: offset %% cylinder-size != 0", part);
#endif
		if (pp->p_offset > lp->d_secperunit) {
			fprintf(stderr, "partition %c: offset past end of unit\n", part);
			errors++;
		}
		if (pp->p_offset + pp->p_size > lp->d_secperunit) {
			fprintf(stderr,
					"partition %c: partition extends past end of unit\n", part);
			errors++;
		}
		if (i == RAW_PART) {
			if (pp->p_fstype != FS_UNUSED)
				Warning("partition %c is not marked as unused!", part);
			if (pp->p_offset != 0)
				Warning("partition %c doesn't start at 0!", part);
			if (pp->p_size != lp->d_secperunit)
				Warning("partition %c doesn't cover the whole unit!", part);

			if ((pp->p_fstype != FS_UNUSED) || (pp->p_offset != 0)
					|| (pp->p_size != lp->d_secperunit)) {
				Warning("An incorrect partition %c may cause problems for "
						"standard system utilities", part);
			}
		}

		/* check for overlaps */
		/* this will check for all possible overlaps once and only once */
		for (j = 0; j < i; j++) {
			pp2 = &lp->d_partitions[j];
			if (j != RAW_PART && i != RAW_PART && pp->p_fstype != FS_VINUM
					&& pp2->p_fstype != FS_VINUM && part_set[i]
					&& part_set[j]) {
				if (pp2->p_offset < pp->p_offset + pp->p_size
						&& (pp2->p_offset + pp2->p_size > pp->p_offset
								|| pp2->p_offset >= pp->p_offset)) {
					fprintf(stderr, "partitions %c and %c overlap!\n", j + 'a',
							i + 'a');
					errors++;
				}
			}
		}
	}
	for (; i < 8 || i < lp->d_npartitions; i++) {
		part = 'a' + i;
		pp = &lp->d_partitions[i];
		if (pp->p_size || pp->p_offset)
			Warning("unused partition %c: size %d offset %lu", 'a' + i,
					pp->p_size, (u_long) pp->p_offset);
	}

	return (errors);
}

/*
 * If we are installing a boot program that doesn't fit in d_bbsize
 * we need to mark those partitions that the boot overflows into.
 * This allows newfs to prevent creation of a filesystem where it might
 * clobber bootstrap code.
 */
static void
setbootflag(lp)
	register struct disklabel *lp;
{
	register struct partition *pp;
	int i, errors = 0;
	char part;
	u_long boffset;

	if (bootbuf == 0)
		return;
	boffset = bootsize / lp->d_secsize;
	for (i = 0; i < lp->d_npartitions; i++) {
		part = 'a' + i;
		pp = &lp->d_partitions[i];
		if (pp->p_size == 0)
			continue;
		if (boffset <= pp->p_offset) {
			if (pp->p_fstype == FS_BOOT)
				pp->p_fstype = FS_UNUSED;
		} else if (pp->p_fstype != FS_BOOT) {
			if (pp->p_fstype != FS_UNUSED) {
				fprintf(stderr, "boot overlaps used partition %c\n", part);
				errors++;
			} else {
				pp->p_fstype = FS_BOOT;
				Warning("boot overlaps partition %c, %s", part,
						"marked as FS_BOOT");
			}
		}
	}
	if (errors) {
		errx(4, "cannot install boot program");
	}
}

/*VARARGS1*/
static void
Warning(const char *fmt, ...)
{
	va_list ap;

	fprintf(stderr, "Warning, ");
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	va_end(ap);
}

static void
usage(void)
{
#if NUMBOOT > 0
	fprintf(stderr,
			"%s\n\t%s\n%s\n\t%s\n%s\n\t%s\n%s\n\t%s\n%s\n\t%s\n%s\n\t%s\n%s\n\t%s\n%s\n\t%s\n",
			"usage: disklabel [-r] disk", "(to read label)",
			"or disklabel -w [-r] disk type [ packid ]",
			"(to write label with existing boot program)",
			"or disklabel -e [-r] disk", "(to edit label)",
			"or disklabel -R [-r] disk protofile",
			"(to restore label with existing boot program)",
#if NUMBOOT > 1
			"or disklabel -B [ -b boot1 [ -s boot2 ] ] disk [ type ]",
			"(to install boot program with existing label)",
			"or disklabel -w -B [ -b boot1 [ -s boot2 ] ] disk type [ packid ]",
			"(to write label and boot program)",
			"or disklabel -R -B [ -b boot1 [ -s boot2 ] ] disk protofile [ type ]",
			"(to restore label and boot program)",
#else
			"or disklabel -B [ -b bootprog ] disk [ type ]",
			"(to install boot program with existing on-disk label)",
			"or disklabel -w -B [ -b bootprog ] disk type [ packid ]",
			"(to write label and install boot program)",
			"or disklabel -R -B [ -b bootprog ] disk protofile [ type ]",
			"(to restore label and install boot program)",
#endif
			"or disklabel [-NW] disk", "(to write disable/enable label)");
#else
	fprintf(stderr, "%-43s%s\n%-43s%s\n%-43s%s\n%-43s%s\n%-43s%s\n",
			"usage: disklabel [-r] disk", "(to read label)",
			"or disklabel -w [-r] disk type [ packid ]", "(to write label)",
			"or disklabel -e [-r] disk", "(to edit label)",
			"or disklabel -R [-r] disk protofile", "(to restore label)",
			"or disklabel [-NW] disk", "(to write disable/enable label)");
#endif
	exit(1);
}
