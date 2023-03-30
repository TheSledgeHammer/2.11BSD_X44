/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 *
 * from: Utah $Hdr: vm_mmap.c 1.6 91/10/21$
 *
 *	@(#)vm_mmap.c	8.10 (Berkeley) 2/19/95
 */
/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)vm_proc.c	1.2 (2.11BSD GTE) 12/24/92
 */

/*
 * Mapped file (mmap) interface to VM
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/filedesc.h>
#include <sys/resourcevar.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/map.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/conf.h>
#include <sys/devsw.h>
#include <sys/mount.h>
#include <sys/sysdecl.h>

#include <miscfs/specfs/specdev.h>

#include <vm/include/vm.h>
#include <vm/include/vm_pager.h>
#include <vm/include/vm_prot.h>
//#include <vm/include/vm_text.h>

#ifdef DEBUG
int mmapdebug = 0;
#define MDB_FOLLOW	0x01
#define MDB_SYNC	0x02
#define MDB_MAPIT	0x04
#endif

/* ARGSUSED */
int
sstk()
{
	register struct sstk_args {
		syscallarg(int) incr;
	} *uap = (struct sstk_args *) u.u_ap;

	/* Not yet implemented */
	return (EOPNOTSUPP);
}

/* ARGSUSED */
int
sbrk()
{
	register struct sbrk_args {
		syscallarg(int)	type;
		syscallarg(segsz_t)	size;
		syscallarg(caddr_t)	addr;
		syscallarg(int)	sep;
		syscallarg(int)	flags;
		syscallarg(int) incr;
	} *uap = (struct sbrk_args *) u.u_ap;

	struct proc *p;
	register_t *retval;
	register segsz_t n, d;

	p = u.u_procp;
	n = btoc(SCARG(uap, size));
	if (!SCARG(uap, sep)) {
		SCARG(uap, sep) = PSEG_NOSEP;
	} else {
		n -= ctos(p->p_tsize) * stoc(1);
	}
	if (n < 0) {
		n = 0;
	}

	if(vm_estabur(p, n, p->p_ssize, p->p_tsize, SCARG(uap, sep), SEG_RO)) {
		return (0);
	}
	vm_expand(p, n, S_DATA);
	/* set d to (new - old) */
	d = n - p->p_dsize;
	if (d > 0) {
		bzero(p->p_daddr + p->p_dsize, d);
	}
	p->p_dsize = n;
	return (0);
}
