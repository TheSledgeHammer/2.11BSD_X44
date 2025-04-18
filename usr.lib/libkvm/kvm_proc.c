/*-
 * Copyright (c) 1989, 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software developed by the Computer Systems
 * Engineering group at Lawrence Berkeley Laboratory under DARPA contract
 * BG 91-66 and contributed to Berkeley.
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
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)kvm_proc.c	8.4 (Berkeley) 8/20/94";
#endif
#endif /* LIBC_SCCS and not lint */

/*
 * Proc traversal interface for kvm.  ps and w are (probably) the exclusive
 * users of this code, so we've factored it out into a separate module.
 * Thus, we keep this grunge out of the other kvm applications (i.e.,
 * most other applications are interested only in open/close/read/nlist).
 */

#include <sys/param.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/exec.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/queue.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <nlist.h>
#include <kvm.h>

#include <vm/include/vm.h>
#include <vm/include/vm_param.h>
#include <vm/include/swap_pager.h>

#include <sys/sysctl.h>

#include <limits.h>
#include <db.h>
#include <paths.h>

#include "kvm_private.h"

static char *kvm_readswap(kvm_t *, const struct proc *, u_long, u_long *);
static int 	kvm_proclist(kvm_t *, int, int, struct proc *, struct kinfo_proc *, int);
static int 	kvm_deadprocs(kvm_t *, int, int, u_long, u_long, int);
static char **kvm_argv(kvm_t *, const struct proc *, u_long, int, int);
static void ps_str_a(struct ps_strings *, u_long *, int *);
static void ps_str_e(struct ps_strings *, u_long *, int *);
static int 	proc_verify(kvm_t *, u_long, const struct proc *);
static char **kvm_doargv(kvm_t *, const struct kinfo_proc *, int, void (*)(struct ps_strings*, u_long *, int *));

ssize_t kvm_uread(kvm_t *, const struct proc *, u_long, char *, size_t);

static char *
kvm_readswap(kd, p, va, cnt)
	kvm_t *kd;
	const struct proc *p;
	u_long va;
	u_long *cnt;
{
	register int ix;
	register u_long addr, head, slot;
	register u_long offset, pagestart, sbstart, pgoff;
	register off_t seekpoint;
	struct vm_map_entry vme;
	struct vm_amap amap;
	struct vm_anon *anonp, anon;
	struct vm_object vmo;
	struct vm_page pg;
	struct pager_struct pager;
	struct swpager swap;
	struct swblock swb;
	static char page[NBPG];

	head = (u_long)&p->p_vmspace->vm_map.cl_header;
	/*
	 * Look through the address map for the memory object
	 * that corresponds to the given virtual address.
	 * The header just has the entire valid range.
	 */
	addr = head;
	while (1) {
		if (KREAD(kd, addr, &vme)) {
			return (0);
		}

		if (va >= vme.start && va <= vme.end && vme.object.vm_object != NULL && vme.aref.ar_amap != NULL) {
			break;
		}

		addr = (u_long) vme.cl_entry.cqe_next;//CIRCLEQ_NEXT(vme, cl_entry);
		if (addr == 0 || addr == head) {
			return (0);
		}
	}

	/*
	 * Check anon's align for given virtual address.
	 */
	if (vme.aref.ar_amap == NULL) {
		return (0);
	}
	addr = (u_long)vme.aref.ar_amap;
	if (KREAD(kd, addr, &amap)) {
		return (0);
	}
	offset = va - vme.start + vme.offset;
	slot = offset / NBPG + vme.aref.ar_pageoff;
	/* sanity-check slot number */
	if (slot > amap.am_nslot) {
		return (0);
	}
	addr = (u_long)amap.am_anon + (offset / NBPG) * sizeof(anonp);
	if (KREAD(kd, addr, &anonp)) {
		return (0);
	}
	addr = (u_long)anonp;
	if (KREAD(kd, addr, &anon)) {
		return (0);
	}
	addr = (u_long)anon.u.an_page;
	if (addr) {
		if (KREAD(kd, addr, &pg)) {
			return (0);
		}
	}

	/*
	 * We found the right object -- follow shadow links.
	 */
	offset = va - vme.start + vme.offset;
	addr = (u_long)vme.object.vm_object;
	while (1) {
		if (KREAD(kd, addr, &vmo)) {
			return (0);
		}
		addr = (u_long) vmo.shadow;
		if (addr == 0) {
			break;
		}
		offset += vmo.shadow_offset;
	}
	if (vmo.pager == 0) {
		return (0);
	}

	offset += vmo.paging_offset;

	/*
	 * Read in the pager info and make sure it's a swap device.
	 */
	addr = (u_long)vmo.pager;
	if (KREAD(kd, addr, &pager) || pager.pg_type != PG_SWAP) {
		return (0);
	}

	/*
	 * Read in the swap_pager private data, and compute the
	 * swap offset.
	 */
	addr = (u_long)pager.pg_data;
	if (KREAD(kd, addr, &swap)) {
		return (0);
	}
	ix = offset / dbtob(swap.sw_bsize);
	if (swap.sw_blocks == 0 || ix >= swap.sw_nblocks) {
		return (0);
	}

	addr = (u_long)&swap.sw_blocks[ix];
	if (KREAD(kd, addr, &swb)) {
		return (0);
	}

	sbstart = (offset / dbtob(swap.sw_bsize)) * dbtob(swap.sw_bsize);
	sbstart /= NBPG;
	pagestart = offset / NBPG;
	pgoff = pagestart - sbstart;

	if (swb.swb_block == 0 || (swb.swb_mask & (1 << pgoff)) == 0) {
		return (0);
	}

	seekpoint = dbtob(swb.swb_block) + ctob(pgoff);
	errno = 0;
	if (lseek(kd->swfd, seekpoint, 0) == -1 && errno != 0) {
		return (0);
	}
	if (read(kd->swfd, page, sizeof(page)) != sizeof(page)) {
		return (0);
	}

	offset %= NBPG;
	*cnt = NBPG - offset;
	return (&page[offset]);
}

/*
 * Read proc's from memory file into buffer bp, which has space to hold
 * at most maxcnt procs.
 */
static int
kvm_proclist(kd, what, arg, p, bp, maxcnt)
	kvm_t *kd;
	int what, arg;
	struct proc *p;
	struct kinfo_proc *bp;
	int maxcnt;
{
	register int cnt = 0;
	struct eproc eproc;
	struct pgrp pgrp;
	struct session sess;
	struct tty tty;
	struct proc proc;

	for (; cnt < maxcnt && p != 0; p = p->p_list.le_next) {
		if (KREAD(kd, (u_long)p, &proc)) {
			_kvm_err(kd, kd->program, "can't read proc at %p", p);
			return (-1);
		}
		if (KREAD(kd, (u_long)proc.p_cred, &eproc.e_pcred) == 0) {
			_kvm_err(kd, kd->program, "can't read proc credentials at %p", p);
			return (-1);
		}

		switch (what) {
			
		case KERN_PROC_PID:
			if (proc.p_pid != (pid_t)arg) {
				continue;
			}
			break;

		case KERN_PROC_UID:
			if (eproc.e_ucred.cr_uid != (uid_t)arg) {
				continue;
			}
			break;

		case KERN_PROC_RUID:
			if (eproc.e_pcred.p_ruid != (uid_t)arg) {
				continue;
			}
			break;
		}
		/*
		 * We're going to add another proc to the set.  If this
		 * will overflow the buffer, assume the reason is because
		 * nprocs (or the proc list) is corrupt and declare an error.
		 */
		if (cnt >= maxcnt) {
			_kvm_err(kd, kd->program, "nprocs corrupt");
			return (-1);
		}
		/*
		 * gather eproc
		 */
		eproc.e_paddr = p;
		if (KREAD(kd, (u_long )proc.p_pgrp, &pgrp)) {
			_kvm_err(kd, kd->program, "can't read pgrp at %p", proc.p_pgrp);
			return (-1);
		}
		eproc.e_sess = pgrp.pg_session;
		eproc.e_pgid = pgrp.pg_id;
		eproc.e_jobc = pgrp.pg_jobc;
		if (KREAD(kd, (u_long )pgrp.pg_session, &sess)) {
			_kvm_err(kd, kd->program, "can't read session at %p", pgrp.pg_session);
			return (-1);
		}
		if ((proc.p_flag & P_CONTROLT) && sess.s_ttyp != NULL) {
			if (KREAD(kd, (u_long )sess.s_ttyp, &tty)) {
				_kvm_err(kd, kd->program, "can't read tty at %p", sess.s_ttyp);
				return (-1);
			}
			eproc.e_tdev = tty.t_dev;
			eproc.e_tsess = tty.t_session;
			if (tty.t_pgrp != NULL) {
				if (KREAD(kd, (u_long )tty.t_pgrp, &pgrp)) {
					_kvm_err(kd, kd->program, "can't read tpgrp at %p",
							tty.t_pgrp);
					return (-1);
				}
				eproc.e_tpgid = pgrp.pg_id;
			} else {
				eproc.e_tpgid = -1;
			}
		} else {
			eproc.e_tdev = NODEV;
		}
		eproc.e_flag = sess.s_ttyvp ? EPROC_CTTY : 0;
		if (sess.s_leader == p) {
			eproc.e_flag |= EPROC_SLEADER;
		}
		if (proc.p_wmesg) {
			(void) kvm_read(kd, (u_long) proc.p_wmesg, eproc.e_wmesg, WMESGLEN);
		}

#ifdef sparc
		(void)kvm_read(kd, (u_long)&proc.p_vmspace->vm_rssize,
		    (char *)&eproc.e_vm.vm_rssize,
		    sizeof(eproc.e_vm.vm_rssize));
		(void)kvm_read(kd, (u_long)&proc.p_vmspace->vm_tsize,
		    (char *)&eproc.e_vm.vm_tsize,
		    3 * sizeof(eproc.e_vm.vm_rssize));	/* XXX */
#else
		(void) kvm_read(kd, (u_long) proc.p_vmspace, (char*) &eproc.e_vm,
				sizeof(eproc.e_vm));
#endif
		eproc.e_xsize = eproc.e_xrssize = 0;
		eproc.e_xccount = eproc.e_xswrss = 0;

		switch (what) {

		case KERN_PROC_PGRP:
			if (eproc.e_pgid != (pid_t) arg) {
				continue;
			}
			break;

		case KERN_PROC_TTY:
			if ((proc.p_flag & P_CONTROLT) == 0
					|| eproc.e_tdev != (dev_t) arg) {
				continue;
			}
			break;
		}
		bcopy(&proc, &bp->kp_proc, sizeof(proc));
		bcopy(&eproc, &bp->kp_eproc, sizeof(eproc));
		++bp;
		++cnt;
	}
	return (cnt);
}

/*
 * Build proc info array by reading in proc list from a crash dump.
 * Return number of procs read.  maxcnt is the max we will read.
 */
static int
kvm_deadprocs(kd, what, arg, a_allproc, a_zombproc, maxcnt)
	kvm_t *kd;
	int what, arg;
	u_long a_allproc;
	u_long a_zombproc;
	int maxcnt;
{
	register struct kinfo_proc *bp = kd->procbase;
	register int acnt, zcnt;
	struct proc *p;

	if (KREAD(kd, a_allproc, &p)) {
		_kvm_err(kd, kd->program, "cannot read allproc");
		return (-1);
	}
	acnt = kvm_proclist(kd, what, arg, p, bp, maxcnt);
	if (acnt < 0)
		return (acnt);

	if (KREAD(kd, a_zombproc, &p)) {
		_kvm_err(kd, kd->program, "cannot read zombproc");
		return (-1);
	}
	zcnt = kvm_proclist(kd, what, arg, p, bp + acnt, maxcnt - acnt);
	if (zcnt < 0) {
		zcnt = 0;
	}
	return (acnt + zcnt);
}

struct nlist nl[] = {
#define	X_NPROC		0
		{ .n_name = "_nprocs" },
#define	X_ALLPROC	1
		{ .n_name = "_allproc" },
#define	X_ZOMBPROC	2
		{ .n_name = "_zombproc" },
#define	X_END		3
		{ .n_name = NULL },
};

struct kinfo_proc *
kvm_getprocs(kd, op, arg, cnt)
	kvm_t *kd;
	int op, arg;
	int *cnt;
{
    size_t size;
	int mib[4], st, nprocs;

	if (kd->procbase != 0) {
		free((void *)kd->procbase);
		/* 
		 * Clear this pointer in case this call fails.  Otherwise,
		 * kvm_close() will free it again.
		 */
		kd->procbase = 0;
	}
	if (ISALIVE(kd)) {
		size = 0;
		mib[0] = CTL_KERN;
		mib[1] = KERN_PROC;
		mib[2] = op;
		mib[3] = arg;
		st = sysctl(mib, 4, NULL, &size, NULL, 0);
		if (st == -1) {
			_kvm_syserr(kd, kd->program, "kvm_getprocs");
			return (0);
		}
		kd->procbase = (struct kinfo_proc *)_kvm_malloc(kd, size);
		if (kd->procbase == 0) {
			return (0);
		}
		st = sysctl(mib, 4, kd->procbase, &size, NULL, 0);
		if (st == -1) {
			_kvm_syserr(kd, kd->program, "kvm_getprocs");
			return (0);
		}
		if (size % sizeof(struct kinfo_proc) != 0) {
			_kvm_err(kd, kd->program,
					"proc size mismatch (%d total, %d chunks)", size,
					sizeof(struct kinfo_proc));
			return (0);
		}
		nprocs = size / sizeof(struct kinfo_proc);
	} else {
		struct nlist *p;

		if (kvm_nlist(kd, nl) != 0) {
			for (p = nl; p->n_type != 0; ++p)
				;
			_kvm_err(kd, kd->program, "%s: no such symbol", p->n_name);
			return (0);
		}
		if (KREAD(kd, nl[X_NPROC].n_value, &nprocs)) {
			_kvm_err(kd, kd->program, "can't read nprocs");
			return (0);
		}
		size = nprocs * sizeof(struct kinfo_proc);
		kd->procbase = (struct kinfo_proc *)_kvm_malloc(kd, size);
		if (kd->procbase == 0) {
			return (0);
		}
		nprocs = kvm_deadprocs(kd, op, arg, nl[X_ALLPROC].n_value, nl[X_ZOMBPROC].n_value, nprocs);
#ifdef notdef
		size = nprocs * sizeof(struct kinfo_proc);
		(void)realloc(kd->procbase, size);
#endif
	}
	*cnt = nprocs;
	return (kd->procbase);
}

void
_kvm_freeprocs(kd)
	kvm_t *kd;
{
	if (kd->procbase) {
		free(kd->procbase);
		kd->procbase = 0;
	}
}

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

/*
 * Read in an argument vector from the user address space of process p.
 * addr if the user-space base address of narg null-terminated contiguous 
 * strings.  This is used to read in both the command arguments and
 * environment strings.  Read at most maxcnt characters of strings.
 */
static char **
kvm_argv(kd, p, addr, narg, maxcnt)
	kvm_t *kd;
	const struct proc *p;
	register u_long addr;
	register int narg;
	register int maxcnt;
{
	register char *cp;
	register int len, cc;
	register char **argv;

	/*
	 * Check that there aren't an unreasonable number of agruments,
	 * and that the address is in user space.
	 */
	if (narg > 512 || addr < VM_MIN_ADDRESS || addr >= VM_MAXUSER_ADDRESS) {
		return (0);
	}

	if (kd->argv == 0) {
		/*
		 * Try to avoid reallocs.
		 */
		kd->argc = MAX(narg + 1, 32);
		kd->argv = (char **)_kvm_malloc(kd, kd->argc * 
						sizeof(*kd->argv));
		if (kd->argv == 0) {
			return (0);
		}
	} else if (narg + 1 > kd->argc) {
		kd->argc = MAX(2 * kd->argc, narg + 1);
		kd->argv = (char**) _kvm_realloc(kd, kd->argv,
				kd->argc * sizeof(*kd->argv));
		if (kd->argv == 0) {
			return (0);
		}
	}
	if (kd->argspc == 0) {
		kd->argspc = (char *)_kvm_malloc(kd, NBPG);
		if (kd->argspc == 0) {
			return (0);
		}
		kd->arglen = NBPG;
	}
	cp = kd->argspc;
	argv = kd->argv;
	*argv = cp;
	len = 0;
	/*
	 * Loop over pages, filling in the argument vector.
	 */
	while (addr < VM_MAXUSER_ADDRESS) {
		cc = NBPG - (addr & PGOFSET);
		if (maxcnt > 0 && cc > maxcnt - len) {
			cc = maxcnt - len;
		}
		if (len + cc > kd->arglen) {
			register int off;
			register char **pp;
			register char *op = kd->argspc;

			kd->arglen *= 2;
			kd->argspc = (char*) _kvm_realloc(kd, kd->argspc, kd->arglen);
			if (kd->argspc == 0) {
				return (0);
			}
			cp = &kd->argspc[len];
			/*
			 * Adjust argv pointers in case realloc moved
			 * the string space.
			 */
			off = kd->argspc - op;
			for (pp = kd->argv; pp < argv; ++pp) {
				*pp += off;
			}
		}
		if (kvm_uread(kd, p, addr, cp, cc) != cc) {
			/* XXX */
			return (0);
		}
		len += cc;
		addr += cc;

		if (maxcnt == 0 && len > 16 * NBPG) {
			/* sanity */
			return (0);
		}

		while (--cc >= 0) {
			if (*cp++ == 0) {
				if (--narg <= 0) {
					*++argv = 0;
					return (kd->argv);
				} else {
					*++argv = cp;
				}
			}
		}
		if (maxcnt > 0 && len >= maxcnt) {
			/*
			 * We're stopping prematurely.  Terminate the
			 * argv and current string.
			 */
			*++argv = 0;
			*cp = 0;
            		break;
		}
	}
    	*argv = NULL;
    	return (kd->argv);
}

static void
ps_str_a(p, addr, n)
	struct ps_strings *p;
	u_long *addr;
	int *n;
{
	*addr = (u_long)p->ps_argvstr;
	*n = p->ps_nargvstr;
}

static void
ps_str_e(p, addr, n)
	struct ps_strings *p;
	u_long *addr;
	int *n;
{
	*addr = (u_long)p->ps_envstr;
	*n = p->ps_nenvstr;
}

/*
 * Determine if the proc indicated by p is still active.
 * This test is not 100% foolproof in theory, but chances of
 * being wrong are very low.
 */
static int
proc_verify(kd, kernp, p)
	kvm_t *kd;
	u_long kernp;
	const struct proc *p;
{
	struct proc kernproc;

	/*
	 * Just read in the whole proc.  It's not that big relative
	 * to the cost of the read system call.
	 */
	if (KREAD(kd, kernp, &kernproc)) {
		return (0);
	}
	return (p->p_pid == kernproc.p_pid
			&& (kernproc.p_stat != SZOMB || p->p_stat == SZOMB));
}

static char **
kvm_doargv(kd, kp, nchr, info)
	kvm_t *kd;
	const struct kinfo_proc *kp;
	int nchr;
	void (*info)(struct ps_strings*, u_long *, int *);
{
	register const struct proc *p = &kp->kp_proc;
	register char **ap;
	u_long addr;
	int cnt;
	struct ps_strings arginfo;

	/*
	 * Pointers are stored at the top of the user stack.
	 */
	if (p->p_stat == SZOMB
			|| kvm_uread(kd, p, USRSTACK - sizeof(arginfo), (char*) &arginfo,
					sizeof(arginfo)) != sizeof(arginfo)) {
		return (0);
	}

	(*info)(&arginfo, &addr, &cnt);
	ap = kvm_argv(kd, p, addr, cnt, nchr);
	/*
	 * For live kernels, make sure this process didn't go away.
	 */
	if (ap != 0 && ISALIVE(kd)
			&& !proc_verify(kd, (u_long) kp->kp_eproc.e_paddr, p)) {
		ap = 0;
	}
	return (ap);
}

/*
 * Get the command args.  This code is now machine independent.
 */
char **
kvm_getargv(kd, kp, nchr)
	kvm_t *kd;
	const struct kinfo_proc *kp;
	int nchr;
{
	return (kvm_doargv(kd, kp, nchr, ps_str_a));
}

char **
kvm_getenvv(kd, kp, nchr)
	kvm_t *kd;
	const struct kinfo_proc *kp;
	int nchr;
{
	return (kvm_doargv(kd, kp, nchr, ps_str_e));
}

/*
 * Read from user space.  The user context is given by p.
 */
ssize_t
kvm_uread(kd, p, uva, buf, len)
	kvm_t *kd;
	register const struct proc *p;
	register u_long uva;
	register char *buf;
	register size_t len;
{
	register char *cp;

	cp = buf;
	while (len > 0) {
		u_long pa;
		register int cc;
		
		cc = _kvm_uvatop(kd, p, uva, &pa);
		if (cc > 0) {
			if (cc > len) {
				cc = len;
			}
			errno = 0;
			if (lseek(kd->pmfd, (off_t) pa, 0) == -1 && errno != 0) {
				_kvm_err(kd, 0, "invalid address (%lx)", uva);
				break;
			}
			cc = read(kd->pmfd, cp, cc);
			if (cc < 0) {
				_kvm_syserr(kd, 0, _PATH_MEM);
				break;
			} else if (cc < len) {
				_kvm_err(kd, kd->program, "short read");
				break;
			}
		} else if (ISALIVE(kd)) {
			/* try swap */
			register char *dp;
			u_long cnt;

			dp = kvm_readswap(kd, p, uva, &cnt);
			if (dp == 0) {
				_kvm_err(kd, 0, "invalid address (%lx)", uva);
				return (0);
			}
			cc = MIN(cnt, len);
			bcopy(dp, cp, cc);
		} else {
			break;
		}
		cp += cc;
		uva += cc;
		len -= cc;
	}
	return ((ssize_t)(cp - buf));
}
