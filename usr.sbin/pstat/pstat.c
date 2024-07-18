/*-
 * Copyright (c) 1980, 1991, 1993, 1994
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
"@(#) Copyright (c) 1980, 1991, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)pstat.c	8.16 (Berkeley) 5/9/95";
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <sys/map.h>
#include <sys/ucred.h>
#define _KERNEL
#include <sys/file.h>
#include <ufs/ufs/quota.h>
#include <ufs/ufs/inode.h>
#define NFS
#include <sys/mount.h>
#undef NFS
#include <sys/uio.h>
#include <sys/namei.h>
#include <miscfs/lofs/lofs.h>
#include <miscfs/union/union.h>
#undef _KERNEL
#include <sys/stat.h>
#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfs.h>
#include <nfs/nfsnode.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/sysctl.h>

#include <err.h>
#include <kvm.h>
#include <limits.h>
#include <nlist.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct nlist nl[] = {
#define VM_SWAPMAP	0
	{ .n_name = "_swapmap" },	/* list of free swap areas */
#define VM_NSWAPMAP	1
	{ .n_name = "_nswapmap" },	/* size of the swap map */
#define VM_SWDEVT	2
	{ .n_name = "_swdevt" },	/* list of swap devices and sizes */
#define VM_NSWAP	3
	{ .n_name = "_nswap" },		/* size of largest swap device */
#define VM_NSWDEV	4
	{ .n_name = "_nswdev" },	/* number of swap devices */
#define VM_DMMAX	5
	{ .n_name = "_dmmax" },		/* maximum size of a swap block */
#define VM_TEXT		6
	{ .n_name = "_text" },		/* list of free text */
#define VM_NTEXT	7
	{ .n_name = "_ntext"},		/* number of text */
#define	V_MOUNTLIST	8
	{ .n_name = "_mountlist" },	/* address of head of mount list. */
#define V_NUMV		9
	{ .n_name = "_numvnodes" },
#define	FNL_NFILE	10
	{ .n_name = "_nfiles"},
#define FNL_MAXFILE	11
	{ .n_name = "_maxfiles"},

#define NLMANDATORY 	FNL_MAXFILE	/* names up to here are mandatory */
#define NLMANNUM(val) 	((val) + FNL_MAXFILE)

#define VM_NISWAP	NLMANNUM(1)
	{ .n_name = "_niswap" },
#define VM_NISWDEV	NLMANNUM(2)
	{ .n_name = "_niswdev" },
#define	SCONS		NLMANNUM(3)
	{ .n_name = "_cons" },
#define	SPTY		NLMANNUM(4)
	{ .n_name = "_pt_tty" },
#define	SNPTY		NLMANNUM(5)
	{ .n_name = "_npty" },
	{ .n_name = NULL },
};

int	usenumflag;
int	totalflag;
char	*nlistf	= NULL;
char	*memf	= NULL;
kvm_t	*kd;

struct {
	int m_flag;
	const char *m_name;
} mnt_flags[] = {
	{ MNT_RDONLY, "rdonly" },
	{ MNT_SYNCHRONOUS, "sync" },
	{ MNT_NOEXEC, "noexec" },
	{ MNT_NOSUID, "nosuid" },
	{ MNT_NODEV, "nodev" },
	{ MNT_UNION, "union" },
	{ MNT_ASYNC, "async" },
	{ MNT_EXRDONLY, "exrdonly" },
	{ MNT_EXPORTED, "exported" },
	{ MNT_DEFEXPORTED, "defexported" },
	{ MNT_EXPORTANON, "exportanon" },
	{ MNT_EXKERB, "exkerb" },
	{ MNT_LOCAL, "local" },
	{ MNT_QUOTA, "quota" },
	{ MNT_ROOTFS, "rootfs" },
	{ MNT_UPDATE, "update" },
	{ MNT_DELEXPORT },
	{ MNT_UPDATE, "update" },
	{ MNT_DELEXPORT, "delexport" },
	{ MNT_RELOAD, "reload" },
	{ MNT_FORCE, "force" },
	{ MNT_MLOCK, "mlock" },
	{ MNT_WAIT, "wait" },
	{ MNT_MPBUSY, "mpbusy" },
	{ MNT_MPWANT, "mpwant" },
	{ MNT_UNMOUNT, "unmount" },
	{ MNT_WANTRDWR, "wantrdwr" },
	{ 0 }
};

#define	SVAR(var) __STRING(var)	/* to force expansion */
#define	KGET(idx, var)							\
	KGET1(idx, &var, sizeof(var), SVAR(var))
#define	KGET1(idx, p, s, msg)						\
	KGET2(nl[idx].n_value, p, s, msg)
#define	KGET2(addr, p, s, msg)						\
	if (kvm_read(kd, (u_long)(addr), p, s) != s) {			\
		warnx("cannot read %s: %s", msg, kvm_geterr(kd)); \
	}
#define	KGETRET(addr, p, s, msg)					\
	if (kvm_read(kd, (u_long)(addr), p, s) != s) {			\
		warnx("cannot read %s: %s", msg, kvm_geterr(kd));	\
		return (0);						\
	}

#define KGETPROC(kp, op, arg, cnt, msg) 	\
	if (((kp) = kvm_getprocs(kd, op, arg, cnt)) == 0) { \
		warnx("cannot read %s: %s", msg, kvm_geterr(kd));	\
	}

struct e_vnode {
	struct vnode *avnode;
	struct vnode vnode;
};

struct e_proc {
	struct kinfo_proc *kinfo;
	struct proc 	*aproc;
	struct proc		proc;
};

struct e_text {
	struct vm_text atext;
	struct vm_text *text;
};

struct {
	int flag;
	char val;
} ttystates[] = {
		{ TS_WOPEN,	'W'},
		{ TS_ISOPEN,	'O'},
		{ TS_CARR_ON,	'C'},
		{ TS_TIMEOUT,	'T'},
		{ TS_FLUSH,	'F'},
		{ TS_BUSY,	'B'},
		{ TS_ASLEEP,	'A'},
		{ TS_XCLUDE,	'X'},
		{ TS_TTSTOP,	'S'},
		{ TS_TBLOCK,	'K'},
		{ TS_ASYNC,	'Y'},
		{ TS_BKSL,	'D'},
		{ TS_ERASE,	'E'},
		{ TS_LNCH,	'L'},
		{ TS_TYPEN,	'P'},
		{ TS_CNTTB,	'N'},
		{ 0,	       '\0'},
};

void	filemode(void);
int	getfiles(char **, int *);
struct mount *getmnt(struct mount *);
struct e_vnode *kinfo_vnodes(int *);
struct e_vnode *loadvnodes(int *);
void	mount_print(struct mount *);
void	nfs_header(void);
int	nfs_print(struct vnode *);
void	swapmode(void);
void	ttymode(void);
void	ttyprt(struct tty *, int);
void	ttytype(struct tty *, char *, int, int);
void	ufs_header(void);
int	ufs_print(struct vnode *);
void	union_header(void);
int	union_print(struct vnode *);
void	usage(void);
void	vnode_header(void);
void	vnode_print(struct vnode *, struct vnode *);
void	vnodemode(void);
void 	procmode(void);
void 	proc_print(struct proc *, struct proc *, int);
struct e_proc *loadprocs(int *);
struct e_proc *kinfo_procs(int *);
struct e_text *loadtexts(int *);
struct e_text *vminfo_texts(int *);
void	textmode(void);
void	putf(long, char);

int
main(argc, argv)
	int argc;
	char *argv[];
{
	extern char *optarg;
	extern int optind;
	int ch, i, quit, ret;
	int fileflag, swapflag, ttyflag, vnodeflag, procflag, textflag;
	char buf[_POSIX2_LINE_MAX];

	fileflag = swapflag = ttyflag = vnodeflag = procflag = textflag = 0;
	while ((ch = getopt(argc, argv, "TM:N:finpstvx")) != EOF)
		switch (ch) {
		case 'f':
			fileflag = 1;
			break;
		case 'M':
			memf = optarg;
			break;
		case 'N':
			nlistf = optarg;
			break;
		case 'n':
			usenumflag = 1;
			break;
		case 'p':
			procflag = 1;
			break;
		case 's':
			swapflag = 1;
			break;
		case 'T':
			totalflag = 1;
			break;
		case 't':
			ttyflag = 1;
			break;
		case 'v':
		case 'i':		/* Backward compatibility. */
			vnodeflag = 1;
			break;
		case 'x':
			textflag = 1;
			break;
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	/*
	 * Discard setgid privileges if not the running kernel so that bad
	 * guys can't print interesting stuff from kernel memory.
	 */
	if (nlistf != NULL || memf != NULL)
		(void)setgid(getgid());

	if ((kd = kvm_openfiles(nlistf, memf, NULL, O_RDONLY, buf)) == 0)
		errx(1, "kvm_openfiles: %s", buf);
	if ((ret = kvm_nlist(kd, nl)) != 0) {
		if (ret == -1)
			errx(1, "kvm_nlist: %s", kvm_geterr(kd));
		for (i = quit = 0; i <= NLMANDATORY; i++)
			if (!nl[i].n_value) {
				quit = 1;
				warnx("undefined symbol: %s\n", nl[i].n_name);
			}
		if (quit)
			exit(1);
	}
	if (!(fileflag | vnodeflag | ttyflag | procflag | swapflag | textflag | totalflag))
		usage();
	if (fileflag || totalflag)
		filemode();
	if (vnodeflag || totalflag)
		vnodemode();
	if (procflag || totalflag)
		procmode();
	if (textflag || totalflag)
		textmode();
	if (ttyflag)
		ttymode();
	if (swapflag || totalflag)
		swapmode();
	exit (0);
}

void
vnodemode(void)
{
	struct e_vnode *e_vnodebase, *endvnode, *evp;
	struct vnode *vp;
	struct mount *maddr, *mp;
	int numvnodes;

	e_vnodebase = loadvnodes(&numvnodes);
	if (totalflag) {
		(void)printf("%7d vnodes\n", numvnodes);
		return;
	}
	endvnode = e_vnodebase + numvnodes;
	(void)printf("%d active vnodes\n", numvnodes);

#define FSTYPE_IS(mp, name)	(strcmp(mp->mnt_stat.f_fstypename, (name)))

	maddr = NULL;
	for (evp = e_vnodebase; evp < endvnode; evp++) {
		vp = &evp->vnode;
		if (vp->v_mount != maddr) {
			/*
			 * New filesystem
			 */
			if ((mp = getmnt(vp->v_mount)) == NULL)
				continue;
			maddr = vp->v_mount;
			mount_print(mp);
			vnode_header();
			if (!FSTYPE_IS(mp, MOUNT_UFS) ||
					!FSTYPE_IS(mp, MOUNT_FFS) ||
					!FSTYPE_IS(mp, MOUNT_MFS))
				ufs_header();
			else if (!FSTYPE_IS(mp, MOUNT_NFS))
				nfs_header();
			else if (!FSTYPE_IS(mp, MOUNT_UNION))
				union_header();
			else if (!FSTYPE_IS(mp, MOUNT_LOFS))
				lofs_header();
			(void)printf("\n");
		}
		vnode_print(evp->avnode, vp);
		if (!FSTYPE_IS(mp, MOUNT_UFS) ||
				!FSTYPE_IS(mp, MOUNT_FFS) ||
				!FSTYPE_IS(mp, MOUNT_MFS))
			ufs_print(vp);
		else if (!FSTYPE_IS(mp, MOUNT_NFS))
			nfs_print(vp);
		else if (!FSTYPE_IS(mp, MOUNT_UNION))
			union_print(vp);
		else if (!FSTYPE_IS(mp, MOUNT_LOFS))
			lofs_print(vp);
		(void)printf("\n");
	}
	free(e_vnodebase);
}

void
vnode_header(void)
{
	(void)printf("ADDR     TYP VFLAG  USE HOLD");
}

void
vnode_print(avnode, vp)
	struct vnode *avnode;
	struct vnode *vp;
{
	char *type, flags[16]; 
	char *fp = flags;
	int flag;

	/*
	 * set type
	 */
	switch (vp->v_type) {
	case VNON:
		type = "non"; break;
	case VREG:
		type = "reg"; break;
	case VDIR:
		type = "dir"; break;
	case VBLK:
		type = "blk"; break;
	case VCHR:
		type = "chr"; break;
	case VLNK:
		type = "lnk"; break;
	case VSOCK:
		type = "soc"; break;
	case VFIFO:
		type = "fif"; break;
	case VBAD:
		type = "bad"; break;
	default: 
		type = "unk"; break;
	}
	/*
	 * gather flags
	 */
	flag = vp->v_flag;
	if (flag & VROOT)
		*fp++ = 'R';
	if (flag & VTEXT)
		*fp++ = 'T';
	if (flag & VSYSTEM)
		*fp++ = 'S';
	if (flag & VXLOCK)
		*fp++ = 'L';
	if (flag & VXWANT)
		*fp++ = 'W';
	if (flag & VBWAIT)
		*fp++ = 'B';
	if (flag & VALIASED)
		*fp++ = 'A';
	if (flag == 0)
		*fp++ = '-';
	*fp = '\0';
	(void)printf("%8x %s %5s %4d %4d",
	    avnode, type, flags, vp->v_usecount, vp->v_holdcnt);
}

void
ufs_header(void)
{
	(void)printf(" FILEID IFLAG RDEV|SZ");
}

int
ufs_print(vp) 
	struct vnode *vp;
{
	int flag;
	struct inode inode, *ip = &inode;
	union dinode {
		struct ufs1_dinode dp1;
		struct ufs2_dinode dp2;
	} dip;
	char flagbuf[16], *flags = flagbuf;
	char *name;
	mode_t type;
	dev_t rdev;

	KGETRET(VTOI(vp), &inode, sizeof(struct inode), "vnode's inode");
	KGETRET(ip->i_din.ffs1_din, &dip, sizeof(struct ufs1_dinode), "inode's dinode");
	if (ip->i_size == dip.dp1.di_size) {
		rdev = dip.dp1.di_rdev;
	} else {
		KGETRET(ip->i_din.ffs1_din, &dip, sizeof(struct ufs2_dinode), "inode's UFS2 dinode");
		rdev = dip.dp2.di_rdev;
	}
	flag = ip->i_flag;
	if (flag & IN_LOCKED)
		*flags++ = 'L';
	if (flag & IN_WANTED)
		*flags++ = 'W';
	if (flag & IN_RENAME)
		*flags++ = 'R';
	if (flag & IN_UPDATE)
		*flags++ = 'U';
	if (flag & IN_ACCESS)
		*flags++ = 'A';
	if (flag & IN_CHANGE)
		*flags++ = 'C';
	if (flag & IN_MODIFIED)
		*flags++ = 'M';
	if (flag & IN_SHLOCK)
		*flags++ = 'S';
	if (flag & IN_EXLOCK)
		*flags++ = 'E';
	if (flag & IN_LWAIT)
		*flags++ = 'Z';
	if (flag == 0)
		*flags++ = '-';
	*flags = '\0';

	(void)printf(" %6d %5s", ip->i_number, flagbuf);
	type = ip->i_mode & S_IFMT;
	if (S_ISCHR(ip->i_mode) || S_ISBLK(ip->i_mode))
		if (usenumflag || ((name = devname(rdev, type)) == NULL))
			(void) printf("   %2d,%-2d", major(rdev), minor(rdev));
		else
			(void) printf(" %7s", name);
	else
		(void) printf(" %7qd", ip->i_size);
	return (0);
}

void
nfs_header(void)
{
	(void)printf(" FILEID NFLAG RDEV|SZ");
}

int
nfs_print(vp) 
	struct vnode *vp;
{
	struct nfsnode nfsnode, *np = &nfsnode;
	char flagbuf[16], *flags = flagbuf;
	int flag;
	char *name;
	mode_t type;

	KGETRET(VTONFS(vp), &nfsnode, sizeof(nfsnode), "vnode's nfsnode");
	flag = np->n_flag;
	if (flag & NFLUSHWANT)
		*flags++ = 'W';
	if (flag & NFLUSHINPROG)
		*flags++ = 'P';
	if (flag & NMODIFIED)
		*flags++ = 'M';
	if (flag & NWRITEERR)
		*flags++ = 'E';
	if (flag & NQNFSNONCACHE)
		*flags++ = 'X';
	if (flag & NQNFSWRITE)
		*flags++ = 'O';
	if (flag & NQNFSEVICTED)
		*flags++ = 'G';
	if (flag == 0)
		*flags++ = '-';
	*flags = '\0';

#define VT	np->n_vattr
	(void)printf(" %6d %5s", VT.va_fileid, flagbuf);
	type = VT.va_mode & S_IFMT;
	if (S_ISCHR(VT.va_mode) || S_ISBLK(VT.va_mode))
		if (usenumflag || ((name = devname(VT.va_rdev, type)) == NULL))
			(void) printf("   %2d,%-2d", major(VT.va_rdev), minor(VT.va_rdev));
		else
			(void) printf(" %7s", name);
	else
		(void) printf(" %7qd", np->n_size);
	return (0);
}

void
union_header(void)
{
	(void)printf("    UPPER    LOWER");
}

int
union_print(vp) 
	struct vnode *vp;
{
	struct union_node unode, *up = &unode;

	KGETRET(VTOUNION(vp), &unode, sizeof(unode), "vnode's unode");

	(void)printf(" %8x %8x", up->un_uppervp, up->un_lowervp);
	return (0);
}

void
lofs_header(void)
{
	(void)printf("   LOWER");
}

int
lofs_print(vp)
	struct vnode *vp;
{
	struct lofsnode lnode, *lp = &lnode;

	KGETRET(VTOLOFS(vp), &lnode, sizeof(lnode), "lofs vnode");

	(void)printf(" %8x", lp->a_lofsvp);
	return (0);
}
	
/*
 * Given a pointer to a mount structure in kernel space,
 * read it in and return a usable pointer to it.
 */
struct mount *
getmnt(maddr)
	struct mount *maddr;
{
	static struct mtab {
		struct mtab *next;
		struct mount *maddr;
		struct mount mount;
	} *mhead = NULL;
	struct mtab *mt;

	for (mt = mhead; mt != NULL; mt = mt->next)
		if (maddr == mt->maddr)
			return (&mt->mount);
	if ((mt = malloc(sizeof(struct mtab))) == NULL)
		err(1, NULL);
	KGETRET(maddr, &mt->mount, sizeof(struct mount), "mount table");
	mt->maddr = maddr;
	mt->next = mhead;
	mhead = mt;
	return (&mt->mount);
}

void
mount_print(mp)
	struct mount *mp;
{
	int flags;
	const char *type;

#define ST	mp->mnt_stat
	(void)printf("*** MOUNT %s %s on %s", ST.f_fstypename,
	    ST.f_mntfromname, ST.f_mntonname);
	if (flags == mp->mnt_flag) {
		int i;
		const char *sep = " (";

		for (i = 0; mnt_flags[i].m_flag; i++) {
			if (flags & mnt_flags[i].m_flag) {
				(void)printf("%s%s", sep, mnt_flags[i].m_name);
				flags &= ~mnt_flags[i].m_flag;
				sep = ",";
			}
		}
		if (flags)
			(void)printf("%sunknown_flags:%x", sep, flags);
		(void)printf(")");
	}
	(void)printf("\n");
#undef ST
}

struct e_vnode *
loadvnodes(avnodes)
	int *avnodes;
{
	int mib[2];
	size_t copysize;
	struct e_vnode *vnodebase;

	if (memf != NULL) {
		/*
		 * do it by hand
		 */
		return (kinfo_vnodes(avnodes));
	}
	mib[0] = CTL_KERN;
	mib[1] = KERN_VNODE;
	if (sysctl(mib, 2, NULL, &copysize, NULL, 0) == -1)
		err(1, "sysctl: KERN_VNODE");
	if ((vnodebase = malloc(copysize)) == NULL)
		err(1, NULL);
	if (sysctl(mib, 2, vnodebase, &copysize, NULL, 0) == -1)
		err(1, "sysctl: KERN_VNODE");
	if (copysize % sizeof(struct e_vnode))
		errx(1, "vnode size mismatch");
	*avnodes = copysize / sizeof(struct e_vnode);

	return (vnodebase);
}

/*
 * simulate what a running kernel does in in kinfo_vnode
 */
struct e_vnode *
kinfo_vnodes(avnodes)
	int *avnodes;
{
	struct mntlist mountlist;
	struct mount *mp, mount;
	struct vnode *vp, vnode;
	char *vbuf, *evbuf, *bp;
	int num, numvnodes;

#define VPTRSZ  sizeof(struct vnode *)
#define VNODESZ sizeof(struct vnode)

	KGET(V_NUMV, numvnodes);
	if ((vbuf = malloc((numvnodes + 20) * (VPTRSZ + VNODESZ))) == NULL)
		err(1, NULL);
	bp = vbuf;
	evbuf = vbuf + (numvnodes + 20) * (VPTRSZ + VNODESZ);
	KGET(V_MOUNTLIST, mountlist);
	for (num = 0, mp = CIRCLEQ_FIRST(&mountlist);;
			mp = CIRCLEQ_NEXT(mp, mnt_list)) {
		KGET2(mp, &mount, sizeof(mount), "mount entry");
		for (vp = LIST_FIRST(&mount.mnt_vnodelist); vp != NULL;
				vp = LIST_NEXT(vp, v_mntvnodes)) {
			KGET2(vp, &vnode, sizeof(vnode), "vnode");
			if ((bp + VPTRSZ + VNODESZ) > evbuf)
				/* XXX - should realloc */
				errx(1, "no more room for vnodes");
			memmove(bp, &vp, VPTRSZ);
			bp += VPTRSZ;
			memmove(bp, &vnode, VNODESZ);
			bp += VNODESZ;
			num++;
		}
		if (mp == CIRCLEQ_LAST(&mountlist))
			break;
	}
	*avnodes = num;
	return ((struct e_vnode *)vbuf);
}

char hdr[]="  LINE RAW CAN OUT  HWT LWT     COL STATE  SESS      PGID DISC\n";
int ttyspace = 128;

void
ttymode(void)
{
	struct tty *tty;

	if ((tty = malloc(ttyspace * sizeof(*tty))) == NULL)
		err(1, NULL);
	(void)printf("1 console\n");
	KGET(SCONS, *tty);
	(void)printf(hdr);
	ttyprt(&tty[0], 0);
	if (nl[SNPTY].n_type != 0) {
		ttytype(tty, "pty", SPTY, SNPTY);
	}
}

void
ttytype(tty, name, type, number)
	struct tty *tty;
	char *name;
	int type, number;
{
	struct tty *tp;
	int ntty;

	if (tty == NULL)
		return;
	KGET(number, ntty);
	(void) printf("%d %s %s\n", ntty, name, (ntty == 1) ? "line" : "lines");
	if (ntty > ttyspace) {
		ttyspace = ntty;
		if ((tty = realloc(tty, ttyspace * sizeof(*tty))) == 0)
			err(1, NULL);
	}
	KGET1(type, tty, ntty * sizeof(struct tty), "tty structs");
	(void) printf(hdr);
	for (tp = tty; tp < &tty[ntty]; tp++)
		ttyprt(tp, tp - tty);
}

void
ttyprt(tp, line)
	struct tty *tp;
	int line;
{
	int i, j;
	pid_t pgid;
	char *name, state[20];

	if (usenumflag || tp->t_dev == 0 ||
	   (name = devname(tp->t_dev, S_IFCHR)) == NULL)
		(void)printf("%7d ", line); 
	else
		(void)printf("%7s ", name);
	(void)printf("%2d %3d ", tp->t_rawq.c_cc, tp->t_canq.c_cc);
	(void)printf("%3d %4d %3d %7d ", tp->t_outq.c_cc, tp->t_hiwat, tp->t_lowat, tp->t_col);
	for (i = j = 0; ttystates[i].flag; i++)
		if (tp->t_state & ttystates[i].flag)
			state[j++] = ttystates[i].val;
	if (j == 0)
		state[j++] = '-';
	state[j] = '\0';
	(void)printf("%-6s %8x", state, (u_long)tp->t_session);
	pgid = 0;
	if (tp->t_pgrp != NULL)
		KGET2(&tp->t_pgrp->pg_id, &pgid, sizeof(pid_t), "pgid");
	(void)printf("%6d ", pgid);
	switch (tp->t_line) {
	case TTYDISC:
		(void)printf("termios\n");
		break;
	case NTTYDISC:
		(void)printf("new term\n");
		break;
	case OTTYDISC:
		(void)printf("old term\n");
		break;
	case TABLDISC:
		(void)printf("tab\n");
		break;
	case SLIPDISC:
		(void)printf("slip\n");
		break;
	default:
		(void)printf("%d\n", tp->t_line);
		break;
	}
}

void
procmode(void)
{
	struct e_proc *eproc, *endproc, *epp;
	struct proc *pp;
	int numprocs, np;

	eproc = loadprocs(&numprocs);
	if (totalflag) {
		printf("%3d processes\n", numprocs);
		return;
	}
	np = 0;
	endproc = eproc + numprocs;
	for (epp = eproc; epp < endproc; epp++) {
		pp = &epp->proc;
		if (pp->p_stat) {
			np++;
		}
	}
	if (totalflag) {
		printf("%3d/%3d processes\n", np, numprocs);
		return;
	}
	printf("%d/%d processes\n", np, numprocs);
	printf("   LOC   S       F PRI      SIG   UID SLP TIM  CPU  NI   PGRP    PID   PPID    ADDR   SADDR   DADDR    SIZE   WCHAN    LINK   TEXTP SIGM\n");
	for (epp = eproc; epp < endproc; epp++) {
		pp = &epp->proc;
		if (pp->p_stat == 0) {
			continue;
		}
		proc_print(epp->aproc, pp, numprocs);
	}
	free(eproc);
}

void
proc_print(aproc, pp, numprocs)
	struct proc *aproc;
	struct proc *pp;
	int numprocs;
{
	printf("%7.1o", numprocs + (pp - aproc) * sizeof(*pp));
	printf(" %2d", pp->p_stat);
	printf(" %7.1x", pp->p_flag);
	printf(" %3d", pp->p_pri);
	printf(" %8.1lx", pp->p_sig);
	printf(" %5u", pp->p_uid);
	printf(" %3d", pp->p_slptime);
	printf(" %3d", pp->p_time);
	printf(" %4d", pp->p_cpu & 0377);
	printf(" %3d", pp->p_nice);
	printf(" %6d", pp->p_pgrp);
	printf(" %6d", pp->p_pid);
	printf(" %6d", pp->p_ppid);
	printf(" %7.1o", pp->p_addr);
	printf(" %7.1o", pp->p_saddr);
	printf(" %7.1o", pp->p_daddr);
	printf(" %7.1o", pp->p_dsize+pp->p_ssize);
	printf(" %7.1o", pp->p_wchan);
	printf(" %7.1o", pp->p_link);
	printf(" %7.1o", pp->p_textp);
	printf(" %8.1lx", pp->p_sigmask);
	printf("\n");
}

struct e_proc *
loadprocs(nprocs)
	int *nprocs;
{
	int mib[2];
	size_t copysize;
	struct e_proc *eproc;

	if (memf != NULL) {
		/*
		 * do it by hand
		 */
		return (kinfo_procs(nprocs));
	}
	mib[0] = CTL_KERN;
	mib[1] = KERN_PROC;
	if (sysctl(mib, 2, NULL, &copysize, NULL, 0) == -1) {
		err(1, "sysctl: KERN_PROC");
	}
	if ((eproc = malloc(copysize)) == NULL) {
		err(1, NULL);
	}
	if (sysctl(mib, 2, eproc, &copysize, NULL, 0) == -1) {
		err(1, "sysctl: KERN_PROC");
	}
	if (copysize % sizeof(struct e_proc)) {
		errx(1, "proc size mismatch");
	}
	*nprocs = copysize / sizeof(struct e_proc);
	return (eproc);
}

struct e_proc *
kinfo_procs(nprocs)
	int *nprocs;
{
	struct kinfo_proc *kp;
	struct e_proc *eproc;
	u_int nproc;
	int i;

	KGETPROC(kp, KERN_PROC_ALL, 0, &nprocs, "kinfo proc");
	if ((eproc = malloc(nprocs * sizeof(*eproc))) == NULL) {
		err(1, NULL);
	}
	for (i = nprocs; --i >= 0; ++kp) {
		eproc[i].kinfo = kp;
		if (eproc[i].kinfo == NULL) {
			errx(1, "kinfo empty");
		}
		eproc[i].proc = eproc[i].kinfo->kp_proc;
	}
	*nprocs = nproc;
	return (eproc);
}

struct e_text *
loadtexts(atexts)
	int *atexts;
{
	int mib[2];
	size_t copysize;
	struct e_text *etext;

	mib[0] = CTL_VM;
	mib[1] = VM_TEXT;
		if (memf != NULL) {
		/*
		 * do it by hand
		 */
		return (vminfo_texts(atexts));
	}
	if (sysctl(mib, 2, NULL, &copysize, NULL, 0) == -1) {
		err(1, "sysctl: VM_TEXT");
	}
	if ((etext = malloc(copysize)) == NULL) {
		err(1, NULL);
	}
	if (sysctl(mib, 2, etext, &copysize, NULL, 0) == -1) {
		err(1, "sysctl: VM_TEXT");
	}
	if (copysize % sizeof(struct e_text)) {
		errx(1, "vm text size mismatch");
	}
	*atexts = copysize / sizeof(struct e_text);
	return (etext);
}

struct e_text *
vminfo_texts(atexts)
	int *atexts;
{
	struct txtlist textlist;
	struct vm_text *xp, text;
	char *xbuf, *exbuf, *bp;
	int num, numtexts;

#define	TPTRSZ	sizeof(struct vm_text *)
#define	TEXTSZ	sizeof(struct vm_text)

	KGET(VM_NTEXT, numtexts);
	if ((xbuf = malloc((numtexts + 20) * (TPTRSZ + TEXTSZ))) == NULL) {
		err(1, NULL);
	}
	bp = xbuf;
	exbuf = xbuf + (numtexts + 20) * (TPTRSZ + TEXTSZ);
	KGET(VM_TEXT, textlist);
	for (num = 0, xp = TAILQ_FIRST(&textlist); ; xp = TAILQ_NEXT(xp, psx_list)) {
		KGET2(xp, &text, sizeof(text), "vm_text entry");
		if ((bp + TPTRSZ + TEXTSZ) > exbuf) {
			errx(1, "no more room for vm_texts");
		}
		memmove(bp, &xp, TPTRSZ);
		bp += TPTRSZ;
		memmove(bp, &text, TEXTSZ);
		bp += TEXTSZ;
		num++;
	}
	*atexts = num;
	return ((struct e_text *)xbuf);
}

void
textmode(void)
{
	struct e_text *etext, *endtext, *xtp;
	struct vm_text *xp;
	int numtexts;
	u_int ntx, ntxca;

	etext = loadtexts(&numtexts);
	if (totalflag) {
		printf("%3d texts\n", numtexts);
		return;
	}
	endtext = etext + numtexts;
	for (xtp = etext; xtp < endtext; xtp++) {
		xp = &xtp->text;
		if (xp->psx_vptr != NULL) {
			ntxca++;
		}
		if (xp->psx_count != 0) {
			ntx++;
		}
	}
	if (totalflag) {
		printf("%3d/%3d texts active, %3d used\n", ntx, numtexts, ntxca);
		return;
	}
	printf("%d/%d active texts, %d used\n", ntx, numtexts, ntxca);
	printf("\
   LOC   FLAGS   DADDR   CADDR    SIZE   VPTR   CNT CCNT   FORW     BACK\n");
	for (xtp = etext; xtp < endtext; xtp++) {
		xp = &xtp->text;
		if (xp->psx_vptr == NULL) {
			continue;
		}
		text_print(xtp->atext, xp);
	}
	free(etext);
}

void
text_print(atext, xp)
	struct vm_text *atext;
	struct vm_text *xp;
{
		putf((long) xp->psx_flag & XPAGV, 'P');
		putf((long) xp->psx_flag & XTRC, 'T');
		putf((long) xp->psx_flag & XWRIT, 'W');
		putf((long) xp->psx_flag & XLOAD, 'L');
		putf((long) xp->psx_flag & XLOCK, 'K');
		putf((long) xp->psx_flag & XWANT, 'w');
		putf((long) xp->psx_flag & XUNUSED, 'u');
		printf("%7.1o ", xp->psx_daddr);
		printf("%7.1o ", xp->psx_caddr);
		printf("%7.1o ", xp->psx_size);
		printf("%7.1o", xp->psx_vptr);
		printf("%4u ", xp->psx_count);
		printf("%4u ", xp->psx_ccount);
		printf("%7.1o ", TAILQ_NEXT(xp, psx_list));
		printf("%7.1o\n", TAILQ_PREV(xp, txtlist, psx_list));
}

void
filemode(void)
{
	struct file *fp;
	struct file *addr;
	char *buf, flagbuf[16], *fbp;
	int len, maxfile, nfile;
	static char *dtypes[] = { "???", "inode", "socket" };

	KGET(FNL_MAXFILE, maxfile);
	if (totalflag) {
		KGET(FNL_NFILE, nfile);
		(void)printf("%3d/%3d files\n", nfile, maxfile);
		return;
	}
	if (getfiles(&buf, &len) == -1)
		return;
	/*
	 * Getfiles returns in malloc'd memory a pointer to the first file
	 * structure, and then an array of file structs (whose addresses are
	 * derivable from the previous entry).
	 */
	addr = LIST_FIRST(((struct filelist *)buf));
	fp = (struct file *)(buf + sizeof(struct filelist));
	nfile = (len - sizeof(struct filelist)) / sizeof(struct file);

	(void) printf("%d/%d open files\n", nfile, maxfile);
	(void) printf("   LOC   TYPE    FLG     CNT  MSG    DATA    OFFSET\n");
	for (; (char*) fp < buf + len; addr = LIST_NEXT(fp, f_list), fp++) {
		if ((unsigned) fp->f_type > DTYPE_SOCKET)
			continue;
		(void) printf("%x ", addr);
		(void) printf("%-8.8s", dtypes[fp->f_type]);
		fbp = flagbuf;
		if (fp->f_flag & FREAD)
			*fbp++ = 'R';
		if (fp->f_flag & FWRITE)
			*fbp++ = 'W';
		if (fp->f_flag & FAPPEND)
			*fbp++ = 'A';
#ifdef FSHLOCK	/* currently gone */
		if (fp->f_flag & FSHLOCK)
			*fbp++ = 'S';
		if (fp->f_flag & FEXLOCK)
			*fbp++ = 'X';
#endif
		if (fp->f_flag & FASYNC)
			*fbp++ = 'I';
		*fbp = '\0';
		(void) printf("%6s  %3d", flagbuf, fp->f_count);
		(void) printf("  %3d", fp->f_msgcount);
		(void) printf("  %8.1x", fp->f_data);
		if (fp->f_offset < 0)
			(void) printf("  %qx\n", fp->f_offset);
		else
			(void) printf("  %qd\n", fp->f_offset);
	}
	free(buf);
}

int
getfiles(abuf, alen)
	char **abuf;
	int *alen;
{
	size_t len;
	int mib[2];
	char *buf;

	/*
	 * XXX
	 * Add emulation of KINFO_FILE here.
	 */
	if (memf != NULL)
		errx(1, "files on dead kernel, not implemented\n");

	mib[0] = CTL_KERN;
	mib[1] = KERN_FILE;
	if (sysctl(mib, 2, NULL, &len, NULL, 0) == -1) {
		warn("sysctl: KERN_FILE");
		return (-1);
	}
	if ((buf = malloc(len)) == NULL)
		err(1, NULL);
	if (sysctl(mib, 2, buf, &len, NULL, 0) == -1) {
		warn("sysctl: KERN_FILE");
		return (-1);
	}
	*abuf = buf;
	*alen = len;
	return (0);
}

/*
 * swapmode is based on a program called swapinfo written
 * by Kevin Lahey <kml@rokkaku.atl.ga.us>.
 */
void
swapmode(void)
{
	char *header, *p;
	int hlen, nswap, nswdev, dmmax, nswapmap, niswap, niswdev;
	int s, e, div, i, l, avail, nfree, npfree, used;
	struct swdevt *sw;
	long blocksize, *perdev;
	struct map *swapmap, *kswapmap;
	struct mapent *mp;

	KGET(VM_NSWAP, nswap);
	KGET(VM_NSWDEV, nswdev);
	KGET(VM_DMMAX, dmmax);
	KGET(VM_NSWAPMAP, nswapmap);
	KGET(VM_SWAPMAP, kswapmap); /* kernel `swapmap' is a pointer */
	if ((sw = malloc(nswdev * sizeof(*sw))) == NULL
			|| (perdev = malloc(nswdev * sizeof(*perdev))) == NULL || (mp =
					malloc(nswapmap * sizeof(*mp))) == NULL)
		err(1, "malloc");
	KGET1(VM_SWDEVT, sw, nswdev * sizeof(*sw), "swdevt");
	KGET2((long )kswapmap, mp, nswapmap * sizeof(*mp), "swapmap");

	/* Supports sequential swap */
	if (nl[VM_NISWAP].n_value != 0) {
		KGET(VM_NISWAP, niswap);
		KGET(VM_NISWDEV, niswdev);
	} else {
		niswap = nswap;
		niswdev = nswdev;
	}

	/* First entry in map is `struct map'; rest are mapent's. */
	swapmap = (struct map *)mp;
	if (nswapmap != swapmap->m_limit - (struct mapent *)kswapmap)
		errx(1, "panic: nswapmap goof");

	/* Count up swap space. */
	nfree = 0;
	memset(perdev, 0, nswdev * sizeof(*perdev));
	for (mp++; mp->m_addr != 0; mp++) {
		s = mp->m_addr;			/* start of swap region */
		e = mp->m_addr + mp->m_size;	/* end of region */
		nfree += mp->m_size;

		/*
		 * Swap space is split up among the configured disks.
		 *
		 * For interleaved swap devices, the first dmmax blocks
		 * of swap space some from the first disk, the next dmmax
		 * blocks from the next, and so on up to niswap blocks.
		 *
		 * Sequential swap devices follow the interleaved devices
		 * (i.e. blocks starting at niswap) in the order in which
		 * they appear in the swdev table.  The size of each device
		 * will be a multiple of dmmax.
		 *
		 * The list of free space joins adjacent free blocks,
		 * ignoring device boundries.  If we want to keep track
		 * of this information per device, we'll just have to
		 * extract it ourselves.  We know that dmmax-sized chunks
		 * cannot span device boundaries (interleaved or sequential)
		 * so we loop over such chunks assigning them to devices.
		 */
		i = -1;
		while (s < e) {		/* XXX this is inefficient */
			int bound = roundup(s+1, dmmax);

			if (bound > e)
				bound = e;
			if (bound <= niswap) {
				/* Interleaved swap chunk. */
				if (i == -1)
					i = (s / dmmax) % niswdev;
				perdev[i] += bound - s;
				if (++i >= niswdev)
					i = 0;
			} else {
				/* Sequential swap chunk. */
				if (i < niswdev) {
					i = niswdev;
					l = niswap + sw[i].sw_nblks;
				}
				while (s >= l) {
					/* XXX don't die on bogus blocks */
					if (i == nswdev-1)
						break;
					l += sw[++i].sw_nblks;
				}
				perdev[i] += bound - s;
			}
			s = bound;
		}
	}

	header = getbsize(&hlen, &blocksize);
	if (!totalflag)
		(void)printf("%-11s %*s %8s %8s %8s  %s\n",
		    "Device", hlen, header,
		    "Used", "Avail", "Capacity", "Type");
	div = blocksize / 512;
	avail = npfree = 0;
	for (i = 0; i < nswdev; i++) {
		int xsize, xfree;

		if (!totalflag) {
			p = devname(sw[i].sw_dev, S_IFBLK);
			(void)printf("/dev/%-6s %*d ", p == NULL ? "??" : p,
			    hlen, sw[i].sw_nblks / div);
		}

		/*
		 * Don't report statistics for partitions which have not
		 * yet been activated via swapon(8).
		 */
		if (!(sw[i].sw_flags & SW_FREED)) {
			if (totalflag)
				continue;
			(void)printf(" *** not available for swapping ***\n");
			continue;
		}
		xsize = sw[i].sw_nblks;
		xfree = perdev[i];
		used = xsize - xfree;
		npfree++;
		avail += xsize;
		if (totalflag)
			continue;
		(void)printf("%8d %8d %5.0f%%    %s\n", 
		    used / div, xfree / div,
		    (double)used / (double)xsize * 100.0,
		    (sw[i].sw_flags & SW_SEQUENTIAL) ?
			     "Sequential" : "Interleaved");
	}

	/* 
	 * If only one partition has been set up via swapon(8), we don't
	 * need to bother with totals.
	 */
	used = avail - nfree;
	if (totalflag) {
		(void)printf("%dM/%dM swap space\n", used / 2048, avail / 2048);
		return;
	}
	if (npfree > 1) {
		(void)printf("%-11s %*d %8d %8d %5.0f%%\n",
		    "Total", hlen, avail / div, used / div, nfree / div,
		    (double)used / (double)avail * 100.0);
	}
}

void
putf(v, n)
	long	v;
	char	n;
{
	if (v) {
		printf("%c", n);
	} else {
		printf(" ");
	}
}

void
usage(void)
{
	(void)fprintf(stderr,
	    "usage: pstat -Tfnpstvx [system] [-M core] [-N system]\n");
	exit(1);
}



#ifdef notyet
void
usrmode(void)
{
	struct user U;
	long	*ip;
	int i, j;

	printf("pcb\t%.1o\n", U.u_pcb.pcb_sigc);
	printf("fps\t%.1o %g %g %g %g %g %g\n", U.u_fps.u_fpsr, U.u_fps.u_fpregs[0],
			U.u_fps.u_fpregs[1], U.u_fps.u_fpregs[2], U.u_fps.u_fpregs[3],
			U.u_fps.u_fpregs[4], U.u_fps.u_fpregs[5]);
	printf("fpsaved\t%d\n", U.u_fpsaved);
	printf("fperr\t%.1o %.1o\n", U.u_fperr.f_fec, U.u_fperr.f_fea);
	printf("procp\t%.1o\n", U.u_procp);
	printf("ar0\t%.1o\n", U.u_ar0);
	printf("comm\t%s\n", U.u_comm);
	printf("arg\t%.1o %.1o %.1o %.1o %.1o %.1o\n", U.u_arg[0], U.u_arg[1],
			U.u_arg[2], U.u_arg[3], U.u_arg[4], U.u_arg[5]);
	printf("ap\t%.1o\n", U.u_ap);
	printf("qsave\t");
	for (i = 0; i < sizeof(label_t) / sizeof(int); i++)
		printf("%.1o ", U.u_qsave.val[i]);
	printf("\n");
	printf("r_val1\t%.1o\n", U.u_r.r_val1);
	printf("r_val2\t%.1o\n", U.u_r.r_val2);
	printf("error\t%d\n", U.u_error);
	printf("uids\t%d,%d,%d,%d,%d\n", U.u_uid, U.u_pcred->p_svuid, U.u_pcred->p_ruid, U.u_pcred->p_svgid,
			U.u_pcred->p_rgid);
	printf("groups");
	for (i = 0; (i < NGROUPS) && (U.u_groups[i] != NOGROUP); i++) {
		if (i % 8 == 0)
			printf("\t");
		printf("%u ", U.u_groups[i]);
		if (i % 8 == 7)
			printf("\n");
	}
	if (i % 8)
		printf("\n");
	printf("tsize\t%.1o\n", U.u_tsize);
	printf("dsize\t%.1o\n", U.u_dsize);
	printf("ssize\t%.1o\n", U.u_ssize);
	printf("ssave\t");
	for (i = 0; i < sizeof(label_t) / sizeof(int); i++)
		printf("%.1o ", U.u_ssave.val[i]);
	printf("\n");
	printf("rsave\t");
	for (i = 0; i < sizeof(label_t) / sizeof(int); i++)
		printf("%.1o ", U.u_rsave.val[i]);
	printf("\n");
	printf("uisa");
	for (i = 0; i < sizeof(U.u_uisa) / sizeof(short); i++) {
		if (i % 8 == 0)
			printf("\t");
		printf("%.1o ", U.u_uisa[i]);
		if (i % 8 == 7)
			printf("\n");
	}
	if (i % 8)
		printf("\n");
	printf("uisd");
	for (i = 0; i < sizeof(U.u_uisd) / sizeof(short); i++) {
		if (i % 8 == 0)
			printf("\t");
		printf("%.1o ", U.u_uisd[i]);
		if (i % 8 == 7)
			printf("\n");
	}
	if (i % 8)
		printf("\n");
	printf("sep\t%d\n", U.u_sep);
	printf("ovdata\t%d %d %.1o %d\n", U.u_ovdata.uo_curov, U.u_ovdata.uo_ovbase,
			U.u_ovdata.uo_dbase, U.u_ovdata.uo_nseg);
	printf("ov_offst");
	for (i = 0; i < NOVL; i++) {
		if (i % 8 == 0)
			printf("\t");
		printf("%.1o ", U.u_ovdata.uo_ov_offst[i]);
		if (i % 8 == 7)
			printf("\n");
	}
	if (i % 8)
		printf("\n");
	printf("signal");
	for (i = 0; i < NSIG; i++) {
		if (i % 8 == 0)
			printf("\t");
		printf("%.1o ", U.u_signal[i]);
		if (i % 8 == 7)
			printf("\n");
	}
	if (i % 8)
		printf("\n");
	printf("sigmask");
	for (i = 0; i < NSIG; i++) {
		if (i % 8 == 0)
			printf("\t");
		printf("%.1lo ", U.u_sigmask[i]);
		if (i % 8 == 7)
			printf("\n");
	}
	if (i % 8)
		printf("\n");
	printf("sigonstack\t%.1lo\n", U.u_sigonstack);
	printf("sigintr\t%.1lo\n", U.u_sigintr);
	printf("oldmask\t%.1lo\n", U.u_oldmask);
	printf("code\t%u\n", U.u_code);
	printf("psflags\t%d\n", U.u_psflags);
	printf("ss_base\t%.1o ss_size %.1o ss_flags %.1o\n", U.u_sigstk.ss_base,
			U.u_sigstk.ss_size, U.u_sigstk.ss_flags);
	printf("ofile");
	for (i = 0; i < NOFILE; i++) {
		if (i % 8 == 0)
			printf("\t");
		printf("%.1o ", U.u_ofile[i]);
		if (i % 8 == 7)
			printf("\n");
	}
	if (i % 8)
		printf("\n");
	printf("pofile");
	for (i = 0; i < NOFILE; i++) {
		if (i % 8 == 0)
			printf("\t");
		printf("%.1o ", U.u_pofile[i]);
		if (i % 8 == 7)
			printf("\n");
	}
	if (i % 8)
		printf("\n");
	printf("lastfile\t%d\n", U.u_lastfile);
	printf("cdir\t%.1o\n", U.u_cdir);
	printf("rdir\t%.1o\n", U.u_rdir);
	printf("ttyp\t%.1o\n", U.u_ttyp);
	printf("ttyd\t%d,%d\n", major(U.u_ttyd), minor(U.u_ttyd));
	printf("cmask\t%.1o\n", U.u_cmask);
	printf("ru\t");
	ip = (long*) &U.u_ru;
	for (i = 0; i < sizeof(U.u_ru) / sizeof(long); i++)
		printf("%ld ", ip[i]);
	printf("\n");
	printf("cru\t");
	ip = (long*) &U.u_cru;
	for (i = 0; i < sizeof(U.u_cru) / sizeof(long); i++)
		printf("%ld ", ip[i]);
	printf("\n");
	printf("timer\t%ld %ld %ld %ld\n", U.u_timer[0].it_interval,
			U.u_timer[0].it_value, U.u_timer[1].it_interval,
			U.u_timer[1].it_value);
	printf("start\t%ld\n", U.u_start);
	printf("acflag\t%d\n", U.u_acflag);
	printf("prof\t%.1o %u %u %u\n", U.u_prof.pr_base, U.u_prof.pr_size,
			U.u_prof.pr_off, U.u_prof.pr_scale);
	printf("rlimit cur\t");
	for (i = 0; i < RLIM_NLIMITS; i++) {
		if (U.u_rlimit[i].rlim_cur == RLIM_INFINITY)
			printf("infinite ");
		else
			printf("%ld ", U.u_rlimit[i].rlim_cur);
	}
	printf("\n");
	printf("rlimit max\t");
	for (i = 0; i < RLIM_NLIMITS; i++) {
		if (U.u_rlimit[i].rlim_max == RLIM_INFINITY)
			printf("infinite ");
		else
			printf("%ld ", U.u_rlimit[i].rlim_max);
	}
	printf("\n");
	printf("quota\t%.1o\n", U.u_quota);
	printf("ncache\t%ld %u %d,%d\n", U.u_ncache.nc_prevoffset,
			U.u_ncache.nc_inumber, major(U.u_ncache.nc_dev),
			minor(U.u_ncache.nc_dev));
	printf("login\t%*s\n", MAXLOGNAME, U.u_login);
}
#endif
