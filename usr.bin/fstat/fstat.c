/*-
 * Copyright (c) 1988, 1993
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
"@(#) Copyright (c) 1988, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)fstat.c	8.3 (Berkeley) 5/2/95";
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/stat.h>
#include <sys/vnode.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/unpcb.h>
#include <sys/sysctl.h>
#include <sys/filedesc.h>
#define	_KERNEL
#include <sys/mount.h>
#include <sys/file.h>
#undef _KERNEL

#include <net/route.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>

#ifdef INET6
#include <netinet/ip6.h>
//#include <netinet6/in6.h>
#include <netinet6/ip6_var.h>
#include <netinet6/in6_pcb.h>
#endif

#include <netdb.h>
#include <arpa/inet.h>

#include <ctype.h>
#include <errno.h>
#include <kvm.h>
#include <limits.h>
#include <nlist.h>
#include <paths.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <err.h>
#include <util.h>

#include "fstat.h"

#define	TEXT	-1
#define	CDIR	-2
#define	RDIR	-3
#define	TRACE	-4

typedef struct devs {
	struct devs *next;
	long fsid;
	ino_t ino;
	const char *name;
} DEVS;

DEVS *devs;

#ifdef notdef
struct nlist nl[] = {
	{ "" },
};
#endif

int fsflg,		/* show files on same filesystem as file(s) argument */
	pflg,		/* show files open by a particular pid */
	uflg;		/* show files open by a particular (effective) user */
int checkfile; 	/* true if restricting to particular files or filesystems */
int	nflg;		/* (numerical) display f.s. and rdev as dev_t */
int vflg;		/* display errors in locating kernel data objects etc... */

int	Pid;
const char *Uname, *Comm;

kvm_t *kd;

struct file **ofiles;	/* buffer of pointers to file structures */
int maxfiles;

#define ALLOC_OFILES(d)	\
	if ((d) > maxfiles) { \
		free(ofiles); \
		ofiles = malloc((d) * sizeof(struct file *)); \
		if (ofiles == NULL) { \
			fprintf(stderr, "fstat: %s\n", strerror(errno)); \
			exit(1); \
		} \
		maxfiles = (d); \
	}

void dofiles(struct kinfo_proc *);
void vtrans(struct vnode *, int, int);
void ftrans(struct file *, int);
char *getmnton(struct mount *);
void socktrans(struct socket *, int);
void getinetproto(int);
int	getfname(const char *);
void usage(void);

int
main(int argc, char **argv)
{
	struct passwd *passwd;
	struct kinfo_proc *p, *plast;
	int arg, ch, what;
	char *memf, *nlistf;
	char buf[_POSIX2_LINE_MAX];
	int cnt;

	arg = 0;
	what = KERN_PROC_ALL;
	nlistf = memf = NULL;
	while ((ch = getopt(argc, argv, "fnp:u:vNM")) != EOF)
		switch ((char) ch) {
		case 'f':
			fsflg = 1;
			break;
		case 'M':
			memf = optarg;
			break;
		case 'N':
			nlistf = optarg;
			break;
		case 'n':
			nflg = 1;
			break;
		case 'p':
			if (pflg++)
				usage();
			if (!isdigit(*optarg)) {
				fprintf(stderr, "fstat: -p requires a process id\n");
				usage();
			}
			what = KERN_PROC_PID;
			arg = atoi(optarg);
			break;
		case 'u':
			if (uflg++)
				usage();
			if (!(passwd = getpwnam(optarg))) {
				fprintf(stderr, "%s: unknown uid\n", optarg);
				exit(1);
			}
			what = KERN_PROC_UID;
			arg = passwd->pw_uid;
			break;
		case 'v':
			vflg = 1;
			break;
		case '?':
		default:
			usage();
		}

	if (*(argv += optind)) {
		for (; *argv; ++argv) {
			if (getfname(*argv))
				checkfile = 1;
		}
		if (!checkfile)	/* file(s) specified, but none accessable */
			exit(1);
	}

	ALLOC_OFILES(256);	/* reserve space for file pointers */

	if (fsflg && !checkfile) {	
		/* -f with no files means use wd */
		if (getfname(".") == 0)
			exit(1);
		checkfile = 1;
	}

	/*
	 * Discard setgid privileges if not the running kernel so that bad
	 * guys can't print interesting stuff from kernel memory.
	 */
	if (nlistf != NULL || memf != NULL)
		setgid(getgid());

	if ((kd = kvm_openfiles(nlistf, memf, NULL, O_RDONLY, buf)) == NULL) {
		fprintf(stderr, "fstat: %s\n", buf);
		exit(1);
	}
#ifdef notdef
	if (kvm_nlist(kd, nl) != 0) {
		fprintf(stderr, "fstat: no namelist: %s\n", kvm_geterr(kd));
		exit(1);
	}
#endif
	if ((p = kvm_getprocs(kd, what, arg, &cnt)) == NULL) {
		fprintf(stderr, "fstat: %s\n", kvm_geterr(kd));
		exit(1);
	}
	if (nflg)
		printf("%s",
"USER     CMD          PID   FD  DEV    INUM       MODE SZ|DV R/W");
	else
		printf("%s",
"USER     CMD          PID   FD MOUNT      INUM MODE         SZ|DV R/W");
	if (checkfile && fsflg == 0)
		printf(" NAME\n");
	else
		putchar('\n');

	for (plast = &p[cnt]; p < plast; ++p) {
		if (p->kp_proc.p_stat == SZOMB)
			continue;
		dofiles(p);
	}
	exit(0);
}

#define PREFIX(i) \
	printf("%-8.8s %-10s %5d", Uname, Comm, Pid); \
	switch(i) { \
	case TEXT: \
		printf(" text"); \
		break; \
	case CDIR: \
		printf("   wd"); \
		break; \
	case RDIR: \
		printf(" root"); \
		break; \
	case TRACE: \
		printf("   tr"); \
		break; \
	default: \
		printf(" %4d", i); \
		break; \
	}

/*
 * print open files attributed to this process
 */
void
dofiles(struct kinfo_proc *kp)
{
	int i;
	struct filedesc0 filed0;
#define	filed	filed0.fd_fd
	struct proc *p = &kp->kp_proc;
	struct eproc *ep = &kp->kp_eproc;

	Uname = user_from_uid(ep->e_ucred.cr_uid, 0);
	Pid = p->p_pid;
	Comm = p->p_comm;

	if (p->p_fd == NULL)
		return;
	if (!KVM_READ(p->p_fd, &filed0, sizeof (filed0))) {
		dprintf("can't read filedesc at %p for pid %d\n",
			p->p_fd, Pid);
		return;
	}

	/*
	 * root directory vnode, if one
	 */
	if (filed.fd_rdir)
		vtrans(filed.fd_rdir, RDIR, FREAD);
	/*
	 * current working directory vnode
	 */
	vtrans(filed.fd_cdir, CDIR, FREAD);
	/*
	 * ktrace vnode, if one
	 */
	if (p->p_tracep)
		ftrans(p->p_tracep, TRACE);
	/*
	 * open files
	 */
#define FPSIZE	(sizeof(struct file *))
	ALLOC_OFILES(filed.fd_lastfile+1);
	if (filed.fd_nfiles > NDFILE) {
		if (!KVM_READ(filed.fd_ofiles, ofiles,
				(filed.fd_lastfile+1) * FPSIZE)) {
			dprintf("can't read file structures at %p for pid %d\n",
			filed.fd_ofiles, Pid);
			return;
		}
	} else
		bcopy(filed0.fd_dfiles, ofiles, (filed.fd_lastfile + 1) * FPSIZE);
	for (i = 0; i <= filed.fd_lastfile; i++) {
		if (ofiles[i] == NULL)
			continue;
		ftrans(ofiles[i], i);
	}
}

void
ftrans(struct file *fp, int i)
{
	struct file file;

	if (!KVM_READ(fp, &file, sizeof(file))) {
		dprintf("can't read file %d at %p for pid %d\n", i, fp, Pid);
		return;
	}
	switch (file.f_type) {
	case DTYPE_VNODE:
		vtrans((struct vnode *)file.f_data, i, file.f_flag);
		break;
	case DTYPE_SOCKET:
		if (checkfile == 0) {
			socktrans((struct socket *)file.f_data, i);
		}
		break;
	case DTYPE_PIPE:
	case DTYPE_KQUEUE:
	case DTYPE_CRYPTO:
	default:
		dprintf("unknown file type %d for file %d of pid %d\n", file.f_type, i,
				Pid);
		break;
	}
}

void
vtrans(struct vnode *vp, int i, int flag)
{
	struct vnode vn;
	struct filestat fst;
	char rw[3], mode[15];
	const char *badtype = NULL, *filename;

	filename = badtype = NULL;
	if (!KVM_READ(vp, &vn, sizeof(struct vnode))) {
		dprintf("can't read vnode at %p for pid %d\n", vp, Pid);
		return;
	}
	if (vn.v_type == VNON || vn.v_tag == VT_NON)
		badtype = "none";
	else if (vn.v_type == VBAD)
		badtype = "bad";
	else
		switch (vn.v_tag) {
		case VT_UFS:
			if (!ufs_filestat(&vn, &fst))
				badtype = "error";
			break;
		case VT_MFS:
			if (!ufs_filestat(&vn, &fst))
				badtype = "error";
			break;
		case VT_LFS:
			if (!ufs_filestat(&vn, &fst))
				badtype = "error";
			break;
		case VT_UFS211:
			if (!ufs211_filestat(&vn, &fst))
				badtype = "error";
			break;
		case VT_NFS:
			if (!nfs_filestat(&vn, &fst))
				badtype = "error";
			break;
		case VT_ISOFS:
			if (!isofs_filestat(&vn, &fst))
				badtype = "error";
			break;
		case VT_MSDOSFS:
			if (!msdosfs_filestat(&vn, &fst))
				badtype = "error";
			break;
		case VT_LOFS:
			if (!lofs_filestat(&vn, &fst))
				badtype = "error";
			break;
		case VT_UFML:
			if (!ufml_filestat(&vn, &fst))
				badtype = "error";
			break;
		default: {
			static char unknown[10];
			sprintf(unknown, "?(%x)", vn.v_tag);
            badtype = unknown;
			break;
		}
	}

	if (checkfile) {
		int fsmatch = 0;
		register DEVS *d;

		if (badtype)
			return;
		for (d = devs; d != NULL; d = d->next)
			if (d->fsid == fst.fsid) {
				fsmatch = 1;
				if (d->ino == fst.fileid) {
					filename = d->name;
					break;
				}
			}
		if (fsmatch == 0 || (filename == NULL && fsflg == 0))
			return;
	}
	PREFIX(i);
	if (badtype) {
		(void)printf(" -         -  %10s    -\n", badtype);
		return;
	}
	if (nflg)
		(void)printf(" %2d,%-2d", major(fst.fsid), minor(fst.fsid));
	else
		(void)printf(" %-8s", getmnton(vn.v_mount));
	if (nflg)
		(void)sprintf(mode, "%o", fst.mode);
	else
		strmode(fst.mode, mode);

	(void)printf(" %6ld %10s", fst.fileid, mode);
	switch (vn.v_type) {
	case VBLK:
	case VCHR: {
		char *name;

		if (nflg || ((name = devname(fst.rdev, vn.v_type == VCHR ?
		    S_IFCHR : S_IFBLK)) == NULL)) {
			printf("  %2d,%-2d", major(fst.rdev), minor(fst.rdev));
		} else {
			printf(" %6s", name);
		}
		break;
	}
	default:
		printf(" %6ld", fst.size);
	}
	rw[0] = '\0';
	if (flag & FREAD)
		strcat(rw, "r");
	if (flag & FWRITE)
		strcat(rw, "w");
	printf(" %2s", rw);
	if (filename && !fsflg)
		printf("  %s", filename);
	putchar('\n');
}

char *
getmnton(struct mount *m)
{
	static struct mount mount;
	static struct mtab {
		struct mtab *next;
		struct mount *m;
		char mntonname[MNAMELEN];
	} *mhead = NULL;
	register struct mtab *mt;

	for (mt = mhead; mt != NULL; mt = mt->next)
		if (m == mt->m)
			return (mt->mntonname);
	if (!KVM_READ(m, &mount, sizeof(struct mount))) {
		fprintf(stderr, "can't read mount table at %p\n", m);
		return (NULL);
	}
	if ((mt = malloc(sizeof (struct mtab))) == NULL) {
		fprintf(stderr, "fstat: %s\n", strerror(errno));
		exit(1);
	}
	mt->m = m;
	bcopy(&mount.mnt_stat.f_mntonname[0], &mt->mntonname[0], MNAMELEN);
	mt->next = mhead;
	mhead = mt;
	return (mt->mntonname);
}

static const char *
inet_addrstr(char *buf, size_t len, const struct in_addr *a, uint16_t p)
{
	char addr[256];

	if (a->s_addr == INADDR_ANY) {
		if (p == 0)
			addr[0] = '\0';
		else
			strlcpy(addr, "*", sizeof(addr));
	} else {
		struct sockaddr_in sin;
		const int niflags = NI_NUMERICHOST;

		(void)memset(&sin, 0, sizeof(sin));
		sin.sin_family = AF_INET6;
		sin.sin_len = sizeof(sin);
		sin.sin_addr = *a;

		if (getnameinfo((struct sockaddr *)&sin, sin.sin_len,
		    addr, sizeof(addr), NULL, 0, niflags))
			if (inet_ntop(AF_INET, a, addr, sizeof(addr)) == NULL)
				strlcpy(addr, "invalid", sizeof(addr));
	}
	if (addr[0])
		snprintf(buf, len, "%s:%u", addr, p);
	else
		strlcpy(buf, addr, len);
	return buf;
}

#ifdef INET6
static const char *
inet6_addrstr(char *buf, size_t len, const struct in6_addr *a, uint16_t p)
{
	char addr[256];

	if (IN6_IS_ADDR_UNSPECIFIED(a)) {
		if (p == 0)
			addr[0] = '\0';
		else
			strlcpy(addr, "*", sizeof(addr));
	} else {
		struct sockaddr_in6 sin6;
		const int niflags = NI_NUMERICHOST;

		(void)memset(&sin6, 0, sizeof(sin6));
		sin6.sin6_family = AF_INET6;
		sin6.sin6_len = sizeof(sin6);
		sin6.sin6_addr = *a;

		if (IN6_IS_ADDR_LINKLOCAL(a) &&
		    *(u_int16_t *)&sin6.sin6_addr.s6_addr[2] != 0) {
			sin6.sin6_scope_id =
				ntohs(*(uint16_t *)&sin6.sin6_addr.s6_addr[2]);
			sin6.sin6_addr.s6_addr[2] = 0;
			sin6.sin6_addr.s6_addr[3] = 0;
		}

		if (getnameinfo((struct sockaddr *)&sin6, sin6.sin6_len,
		    addr, sizeof(addr), NULL, 0, niflags))
			if (inet_ntop(AF_INET6, a, addr, sizeof(addr)) == NULL)
				strlcpy(addr, "invalid", sizeof(addr));
	}
	if (addr[0])
		snprintf(buf, len, "[%s]:%u", addr, p);
	else
		strlcpy(buf, addr, len);

	return buf;
}
#endif

void
socktrans(struct socket *sock, int i)
{
	static const char *stypename[] = {
		"unused",	/* 0 */
		"stream", 	/* 1 */
		"dgram",	/* 2 */
		"raw",		/* 3 */
		"rdm",		/* 4 */
		"seqpak"	/* 5 */
	};
#define	STYPEMAX 5
	struct socket	so;
	struct protosw	proto;
	struct domain	dom;
	struct inpcb	inpcb;
#ifdef INET6
	struct in6pcb	in6pcb;
#endif
	struct unpcb	unpcb;
	int len;
	char dname[32];
	char lbuf[512], fbuf[512];
	PREFIX(i);

	/* fill in socket */
	if (!KVM_READ(sock, &so, sizeof(struct socket))) {
		dprintf("can't read sock at %p\n", sock);
		goto bad;
	}

	/* fill in protosw entry */
	if (!KVM_READ(so.so_proto, &proto, sizeof(struct protosw))) {
		dprintf("can't read protosw at %p", so.so_proto);
		goto bad;
	}

	/* fill in domain */
	if (!KVM_READ(proto.pr_domain, &dom, sizeof(struct domain))) {
		dprintf("can't read domain at %p\n", proto.pr_domain);
		goto bad;
	}

	if ((len = kvm_read(kd, (u_long)dom.dom_name, dname,
	    sizeof(dname) - 1)) < 0) {
		dprintf("can't read domain name at %p\n",
			dom.dom_name);
		dname[0] = '\0';
	}
	else
		dname[len] = '\0';

	if ((u_short)so.so_type > STYPEMAX)
		printf("* %s ?%d", dname, so.so_type);
	else
		printf("* %s %s", dname, stypename[so.so_type]);

	/* 
	 * protocol specific formatting
	 *
	 * Try to find interesting things to print.  For tcp, the interesting
	 * thing is the address of the tcpcb, for udp and others, just the
	 * inpcb (socket pcb).  For unix domain, its the address of the socket
	 * pcb and the address of the connected pcb (if connected).  Otherwise
	 * just print the protocol number and address of the socket itself.
	 * The idea is not to duplicate netstat, but to make available enough
	 * information for further analysis.
	 */
	fbuf[0] = '\0';
	lbuf[0] = '\0';
	switch (dom.dom_family) {
	case AF_INET:
		getinetproto(proto.pr_protocol);
		if (proto.pr_protocol == IPPROTO_TCP) {
			if (so.so_pcb) {
				if (kvm_read(kd, (u_long) so.so_pcb, (char *)&inpcb,
						sizeof(struct inpcb)) != sizeof(struct inpcb)) {
					dprintf("can't read inpcb at %x\n", so.so_pcb);
					goto bad;
				}
				inet_addrstr(lbuf, sizeof(lbuf), &inpcb.inp_laddr, ntohs(inpcb.inp_lport));
				inet_addrstr(fbuf, sizeof(fbuf), &inpcb.inp_faddr, ntohs(inpcb.inp_fport));
			}
			break;
		} else if (proto.pr_protocol == IPPROTO_UDP) {
			if (so.so_pcb) {
				if (kvm_read(kd, (u_long) so.so_pcb, (char *)&inpcb,
						sizeof(struct inpcb)) != sizeof(struct inpcb)) {
					dprintf("can't read inpcb at %x\n", so.so_pcb);
					goto bad;
				}
				inet_addrstr(lbuf, sizeof(lbuf), &inpcb.inp_laddr, ntohs(inpcb.inp_lport));
				inet_addrstr(fbuf, sizeof(fbuf), &inpcb.inp_faddr, ntohs(inpcb.inp_fport));
			}
			break;
		} else if (so.so_pcb) {
			printf(" %lx", (long)so.so_pcb);
		}
		break;
#ifdef INET6
	case AF_INET6:
		getinetproto(proto.pr_protocol);
		if (proto.pr_protocol == IPPROTO_TCP) {
			if (so.so_pcb) {
				if (kvm_read(kd, (u_long) so.so_pcb, (char *)&in6pcb,
						sizeof(struct in6pcb)) != sizeof(struct in6pcb)) {
					dprintf("can't read in6pcb at %p", so.so_pcb);
					goto bad;
				}
				inet6_addrstr(lbuf, sizeof(lbuf), &in6pcb.in6p_laddr, ntohs(in6pcb.in6p_lport));
				inet6_addrstr(fbuf, sizeof(fbuf), &in6pcb.in6p_faddr, ntohs(in6pcb.in6p_fport));
			}
			break;
		} else if (proto.pr_protocol == IPPROTO_UDP) {
			if (so.so_pcb) {
				if (kvm_read(kd, (u_long) so.so_pcb, (char *)&in6pcb,
						sizeof(struct in6pcb)) != sizeof(struct in6pcb)) {
					dprintf("can't read inpcb at %p", so.so_pcb);
					goto bad;
				}
				inet6_addrstr(lbuf, sizeof(lbuf), &in6pcb.in6p_laddr, ntohs(in6pcb.in6p_lport));
				inet6_addrstr(fbuf, sizeof(fbuf), &in6pcb.in6p_faddr, ntohs(in6pcb.in6p_fport));
			}
			break;
		} else if (so.so_pcb) {
			printf(" %lx", (long)so.so_pcb);
		}
		break;
#endif
	case AF_UNIX:
		/* print address of pcb and connected pcb */
		if (so.so_pcb) {
			printf(" %x", (int)so.so_pcb);
			if (kvm_read(kd, (u_long) so.so_pcb, (char *)&unpcb,
					sizeof(struct unpcb)) != sizeof(struct unpcb)) {
				dprintf("can't read unpcb at %x\n", so.so_pcb);
				goto bad;
			}
			if (unpcb.unp_conn) {
				char shoconn[4], *cp;

				cp = shoconn;
				if (!(so.so_state & SS_CANTRCVMORE))
					*cp++ = '<';
				*cp++ = '-';
				if (!(so.so_state & SS_CANTSENDMORE))
					*cp++ = '>';
				*cp = '\0';
				printf(" %s %x", shoconn, (int)unpcb.unp_conn);
			}
		}
		break;
	default:
		/* print protocol number and socket address */
		printf(" %d %x", proto.pr_protocol, (int)sock);
	}
	printf("\n");
	return;
bad:
	printf("* error\n");
}

/*
 * getinetproto --
 *	print name of protocol number
 */
void
getinetproto(int number)
{
	const char *cp;

	switch (number) {
	case IPPROTO_IP:
		cp = "ip";
		break;
	case IPPROTO_ICMP:
		cp = "icmp";
		break;
	case IPPROTO_GGP:
		cp = "ggp";
		break;
	case IPPROTO_TCP:
		cp = "tcp";
		break;
	case IPPROTO_EGP:
		cp = "egp";
		break;
	case IPPROTO_PUP:
		cp = "pup";
		break;
	case IPPROTO_UDP:
		cp = "udp";
		break;
	case IPPROTO_IDP:
		cp = "idp";
		break;
	case IPPROTO_RAW:
		cp = "raw";
		break;
	case IPPROTO_ICMPV6:
		cp = "icmp6";
		break;
	default:
		printf(" %d", number);
		return;
	}
	printf(" %s", cp);
}

int
getfname(const char *filename)
{
	struct stat statbuf;
	DEVS *cur;

	if (stat(filename, &statbuf)) {
		fprintf(stderr, "fstat: %s: %s\n", filename, strerror(errno));
		return (0);
	}
	if ((cur = malloc(sizeof(DEVS))) == NULL) {
		fprintf(stderr, "fstat: %s\n", strerror(errno));
		exit(1);
	}
	cur->next = devs;
	devs = cur;

	cur->ino = statbuf.st_ino;
	cur->fsid = statbuf.st_dev & 0xffff;
	cur->name = filename;
	return (1);
}

mode_t
getftype(enum vtype v_type)
{
	mode_t ftype;

	switch (v_type) {
	case VREG:
		ftype = S_IFREG;
		break;
	case VDIR:
		ftype = S_IFDIR;
		break;
	case VBLK:
		ftype = S_IFBLK;
		break;
	case VCHR:
		ftype = S_IFCHR;
		break;
	case VLNK:
		ftype = S_IFLNK;
		break;
	case VSOCK:
		ftype = S_IFSOCK;
		break;
	case VFIFO:
		ftype = S_IFIFO;
		break;
	default:
		ftype = 0;
		break;
	};

	return (ftype);
}

void
usage(void)
{
	(void)fprintf(stderr,
 "usage: fstat [-fnv] [-p pid] [-u user] [-N system] [-M core] [file ...]\n");
	exit(1);
}
