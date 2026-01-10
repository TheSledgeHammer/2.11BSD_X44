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
#include <vm/include/vm_text.h>

#ifdef DEBUG
int mmapdebug = 0;
#define MDB_FOLLOW	0x01
#define MDB_SYNC	0x02
#define MDB_MAPIT	0x04
#endif

/* ARGSUSED */
int
sbrk()
{
	register struct sbrk_args {
		syscallarg(int) incr;
		syscallarg(int)	sep;
	} *uap = (struct sbrk_args *) u.u_ap;

	struct proc *p;
	register_t *retval;
	register segsz_t n, d;
	int error;

	p = u.u_procp;
	n = btoc(SCARG(uap, incr));
	switch (SCARG(uap, sep)) {
	case PSEG_NOSEP:
		SCARG(uap, sep) = PSEG_NOSEP;
		break;

	case PSEG_SEP:
		SCARG(uap, sep) = PSEG_SEP;
		n -= ctos(p->p_tsize) * stoc(1);
		break;

	default:
		SCARG(uap, sep) = PSEG_NOSEP;
		break;
	}

	if (n < 0) {
		n = 0;
	}
	error = vm_estabur(p, n, p->p_ssize, p->p_tsize, SCARG(uap, sep), SEG_RO);
	if (error != 0) {
		return (error);
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

/* A copy of sbrk (above) but targets the stack instead */
/* ARGSUSED */
int
sstk()
{
	register struct sstk_args {
		syscallarg(int) incr;
		syscallarg(int)	sep;
	} *uap = (struct sstk_args *) u.u_ap;
	struct proc *p;
	register_t *retval;
	register segsz_t n, s;
	int error;
	
	p = u.u_procp;
	n = btoc(SCARG(uap, incr));
	switch (SCARG(uap, sep)) {
	case PSEG_NOSEP:
		SCARG(uap, sep) = PSEG_NOSEP;
		break;

	case PSEG_SEP:
		SCARG(uap, sep) = PSEG_SEP;
		n -= ctos(p->p_tsize) * stoc(1);
		break;

	default:
		SCARG(uap, sep) = PSEG_NOSEP;
		break;
	}

	if (n < 0) {
		n = 0;
	}
	error = vm_estabur(p, p->p_dsize, n, p->p_tsize, SCARG(uap, sep), SEG_RO);
	if (error != 0) {
		return (error);
	}
	vm_expand(p, n, S_STACK);
	/* set s to (new - old) */
	s = n - p->p_ssize;
	if (s > 0) {
		bzero(p->p_saddr + p->p_ssize, s);
	}
	p->p_ssize = n;
	return (0);
}

int
mmap()
{
	register struct mmap_args {
		syscallarg(caddr_t) addr;
		syscallarg(size_t) len;
		syscallarg(int) prot;
		syscallarg(int) flags;
		syscallarg(int) fd;
		syscallarg(long) pad;
		syscallarg(off_t) pos;
	} *uap = (struct mmap_args *) u.u_ap;

	struct proc *p;
	register_t *retval;
	register struct filedesc *fdp;
	register struct file *fp;
	struct vnode *vp;
	vm_offset_t addr, pos;
	vm_size_t size;
	vm_prot_t prot, maxprot;
	caddr_t handle;
	int flags, error;

	p = u.u_procp;
	fdp = p->p_fd;
	prot = SCARG(uap, prot) & VM_PROT_ALL;
	flags = SCARG(uap, flags);
	pos = SCARG(uap, pos);
#ifdef DEBUG
	if (mmapdebug & MDB_FOLLOW)
		printf("mmap(%d): addr %x len %x pro %x flg %x fd %d pos %x\n",
				p->p_pid, SCARG(uap, addr), SCARG(uap, len), prot, flags,
				SCARG(uap, fd), pos);
#endif

	/*
	 * Address (if FIXED) must be page aligned.
	 * Size is implicitly rounded to a page boundary.
	 *
	 * XXX most (all?) vendors require that the file offset be
	 * page aligned as well.  However, we already have applications
	 * (e.g. nlist) that rely on unrestricted alignment.  Since we
	 * support it, let it happen.
	 */
	addr = (vm_offset_t) SCARG(uap, addr);
	if (((flags & MAP_FIXED) && (addr & PAGE_MASK)) ||
#if 0
		    ((flags & MAP_ANON) == 0 && (pos & PAGE_MASK)) ||
#endif
			(ssize_t)SCARG(uap, len) < 0
			|| ((flags & MAP_ANON) && SCARG(uap, fd) != -1))
		return (EINVAL);
	size = (vm_size_t) round_page(SCARG(uap, len));
	/*
	 * Check for illegal addresses.  Watch out for address wrap...
	 * Note that VM_*_ADDRESS are not constants due to casts (argh).
	 */
	if (flags & MAP_FIXED) {
		if (VM_MAXUSER_ADDRESS > 0 && addr + size >= VM_MAXUSER_ADDRESS)
			return (EINVAL);
		if (VM_MIN_ADDRESS > 0 && addr < VM_MIN_ADDRESS)
			return (EINVAL);
		if (addr > addr + size)
			return (EINVAL);
	}
	/*
	 * XXX for non-fixed mappings where no hint is provided or
	 * the hint would fall in the potential heap space,
	 * place it after the end of the largest possible heap.
	 *
	 * There should really be a pmap call to determine a reasonable
	 * location.
	 */
	else if (addr < round_page(p->p_vmspace->vm_daddr + MAXDSIZ))
		addr = round_page(p->p_vmspace->vm_daddr + MAXDSIZ);
	if (flags & MAP_ANON) {
		/*
		 * Mapping blank space is trivial.
		 */
		handle = NULL;
		maxprot = VM_PROT_ALL;
		pos = 0;
	} else {
		/*
		 * Mapping file, get fp for validation.
		 * Obtain vnode and make sure it is of appropriate type.
		 */
		if (((unsigned) SCARG(uap, fd)) >= fdp->fd_nfiles||
		(fp = fdp->fd_ofiles[SCARG(uap, fd)]) == NULL)
			return (EBADF);
		if (fp->f_type != DTYPE_VNODE)
			return (EINVAL);
		vp = (struct vnode*) fp->f_data;
		if (vp->v_type != VREG && vp->v_type != VCHR)
			return (EINVAL);
		/*
		 * XXX hack to handle use of /dev/zero to map anon
		 * memory (ala SunOS).
		 */
		if (vp->v_type == VCHR && iszerodev(vp->v_rdev)) {
			handle = NULL;
			maxprot = VM_PROT_ALL;
			flags |= MAP_ANON;
		} else {
			/*
			 * Ensure that file and memory protections are
			 * compatible.  Note that we only worry about
			 * writability if mapping is shared; in this case,
			 * current and max prot are dictated by the open file.
			 * XXX use the vnode instead?  Problem is: what
			 * credentials do we use for determination?
			 * What if proc does a setuid?
			 */
			maxprot = VM_PROT_EXECUTE; /* ??? */
			if (fp->f_flag & FREAD)
				maxprot |= VM_PROT_READ;
			else if (prot & PROT_READ)
				return (EACCES);
			if (flags & MAP_SHARED) {
				if (fp->f_flag & FWRITE)
					maxprot |= VM_PROT_WRITE;
				else if (prot & PROT_WRITE)
					return (EACCES);
			} else
				maxprot |= VM_PROT_WRITE;
			handle = (caddr_t) vp;
		}
	}
	error = vm_mmap(&p->p_vmspace->vm_map, &addr, size, prot, maxprot, flags,
			handle, pos);
	if (error == 0)
		*retval = (register_t) addr;
	return (error);
}

int
msync()
{
	register struct msync_args {
		syscallarg(caddr_t) addr;
		syscallarg(size_t) len;
		syscallarg(int) flags;
	} *uap = (struct msync_args *) u.u_ap;

	struct proc *p;
	register_t *retval;
	vm_offset_t addr;
	vm_size_t size, pageoff;
	vm_map_t map;
	int rv, flags;
	bool_t syncio, invalidate;

	p = u.u_procp;
#ifdef DEBUG
	if (mmapdebug & (MDB_FOLLOW|MDB_SYNC))
		printf("msync(%d): addr %x len %x\n",
		       p->p_pid, SCARG(uap, addr), SCARG(uap, len));
#endif
	if (((vm_offset_t) SCARG(uap, addr) & PAGE_MASK) || SCARG(uap, addr) + SCARG(uap, len) < SCARG(uap, addr))
		return (EINVAL);

	map = &p->p_vmspace->vm_map;
	addr = (vm_offset_t) SCARG(uap, addr);
	size = (vm_size_t) SCARG(uap, len);
	flags = SCARG(uap, flags);

	/* sanity check flags */
	if ((flags & ~(MS_ASYNC | MS_SYNC | MS_INVALIDATE)) != 0
			|| (flags & (MS_ASYNC | MS_SYNC | MS_INVALIDATE)) == 0
			|| (flags & (MS_ASYNC | MS_SYNC)) == (MS_ASYNC | MS_SYNC))
		return (EINVAL);
	if ((flags & (MS_ASYNC | MS_SYNC)) == 0)
		flags |= MS_SYNC;

	/*
	 * align the address to a page boundary and adjust the size accordingly.
	 */

	pageoff = (addr & PAGE_MASK);
	addr -= pageoff;
	size += pageoff;
	size = (vm_size_t)round_page(size);

	/* disallow wrap-around. */
	if (addr + size < addr)
		return (EINVAL);

	/*
	 * XXX Gak!  If size is zero we are supposed to sync "all modified
	 * pages with the region containing addr".  Unfortunately, we
	 * don't really keep track of individual mmaps so we approximate
	 * by flushing the range of the map entry containing addr.
	 * This can be incorrect if the region splits or is coalesced
	 * with a neighbor.
	 */
	if (size == 0) {
		vm_map_entry_t entry;

		vm_map_lock_read(map);
		rv = vm_map_lookup_entry(map, addr, &entry);
		vm_map_unlock_read(map);
		if (!rv)
			return (EINVAL);
		addr = entry->start;
		size = entry->end - entry->start;
	}
#ifdef DEBUG
	if (mmapdebug & MDB_SYNC)
		printf("msync: cleaning/flushing address range [%x-%x)\n", addr,
				addr + size);
#endif
	/*
	 * Could pass this in as a third flag argument to implement
	 * Sun's MS_ASYNC.
	 */
	syncio = TRUE;
	/*
	 * XXX bummer, gotta flush all cached pages to ensure
	 * consistency with the file system cache.  Otherwise, we could
	 * pass this in to implement Sun's MS_INVALIDATE.
	 */
	invalidate = TRUE;

	/*
	 * Clean the pages and interpret the return value.
	 */
	rv = vm_map_clean(map, addr, addr + size, syncio, invalidate);
	switch (rv) {
	case KERN_SUCCESS:
		break;
	case KERN_INVALID_ADDRESS:
		return (EINVAL); /* Sun returns ENOMEM? */
	case KERN_FAILURE:
		return (EIO);
	default:
		return (EINVAL);
	}
	return (0);
}

int
munmap()
{
	register struct munmap_args {
		syscallarg(caddr_t) addr;
		syscallarg(size_t) len;
	} *uap = (struct munmap_args *) u.u_ap;

	struct proc *p;
	register_t *retval;
	vm_offset_t addr;
	vm_size_t size;
	vm_map_t map;

	p = u.u_procp;
#ifdef DEBUG
	if (mmapdebug & MDB_FOLLOW)
		printf("munmap(%d): addr %x len %x\n", p->p_pid, SCARG(uap, addr),
				SCARG(uap, len));
#endif
	addr = (vm_offset_t) SCARG(uap, addr);
	if ((addr & PAGE_MASK) || SCARG(uap, len) < 0)
		return (EINVAL);
	size = (vm_size_t) round_page(SCARG(uap, len));
	if (size == 0)
		return (0);
	/*
	 * Check for illegal addresses.  Watch out for address wrap...
	 * Note that VM_*_ADDRESS are not constants due to casts (argh).
	 */
	if (VM_MAXUSER_ADDRESS > 0 && addr + size >= VM_MAXUSER_ADDRESS)
		return (EINVAL);
	if (VM_MIN_ADDRESS > 0 && addr < VM_MIN_ADDRESS)
		return (EINVAL);
	if (addr > addr + size)
		return (EINVAL);
	map = &p->p_vmspace->vm_map;
	/*
	 * Make sure entire range is allocated.
	 * XXX this seemed overly restrictive, so we relaxed it.
	 */
#if 0
	if (!vm_map_check_protection(map, addr, addr + size, VM_PROT_NONE))
		return (EINVAL);
#endif
	/* returns nothing but KERN_SUCCESS anyway */
	(void) vm_map_remove(map, addr, addr + size);
	return (0);
}

void
munmapfd(p, fd)
	struct proc *p;
	int fd;
{
#ifdef DEBUG
	if (mmapdebug & MDB_FOLLOW)
		printf("munmapfd(%d): fd %d\n", p->p_pid, fd);
#endif

	/*
	 * XXX should vm_deallocate any regions mapped to this file
	 */
	p->p_fd->fd_ofileflags[fd] &= ~UF_MAPPED;
}

int
mprotect()
{
	register struct mprotect_args {
		syscallarg(caddr_t) addr;
		syscallarg(size_t) len;
		syscallarg(int) prot;
	} *uap = (struct mprotect_args *) u.u_ap;

	struct proc *p;
	register_t *retval;
	vm_offset_t addr;
	vm_size_t size, pageoff;
	register vm_prot_t prot;

	p = u.u_procp;
#ifdef DEBUG
	if (mmapdebug & MDB_FOLLOW)
		printf("mprotect(%d): addr %x len %x prot %d\n",
		       p->p_pid, SCARG(uap, addr), SCARG(uap, len), SCARG(uap, prot));
#endif

	addr = (vm_offset_t)SCARG(uap, addr);
	if ((addr & PAGE_MASK) || SCARG(uap, len) < 0)
		return(EINVAL);
	size = (vm_size_t)SCARG(uap, len);
	prot = SCARG(uap, prot) & VM_PROT_ALL;

	/*
	 * align the address to a page boundary and adjust the size accordingly.
	 */

	pageoff = (addr & PAGE_MASK);
	addr -= pageoff;
	size += pageoff;
	size = round_page(size);

	switch (vm_map_protect(&p->p_vmspace->vm_map, addr, addr+size, prot, FALSE)) {
	case KERN_SUCCESS:
		return (0);
	case KERN_PROTECTION_FAILURE:
		return (EACCES);
	}
	return (EINVAL);

	return (0);
}

int
minherit()
{
	register struct minherit_args {
		syscallarg(caddr_t) addr;
		syscallarg(size_t) len;
		syscallarg(int) inherit;
	} *uap = (struct minherit_args *) u.u_ap;

	struct proc *p;
	register_t *retval;
	vm_offset_t addr;
	vm_size_t size, pageoff;
	vm_inherit_t inherit;
	int error;

	p = u.u_procp;
	addr = (vm_offset_t)SCARG(uap, addr);
	size = (vm_size_t)SCARG(uap, len);
	inherit = SCARG(uap, inherit);

	/*
	 * align the address to a page boundary and adjust the size accordingly.
	 */

	pageoff = (addr & PAGE_MASK);
	addr -= pageoff;
	size += pageoff;
	size = (vm_size_t)round_page(size);

	if ((int)size < 0)
		return (EINVAL);
	error = vm_map_inherit(&p->p_vmspace->vm_map, addr, addr + size, inherit);
	return (error);
}

/* ARGSUSED */
int
madvise()
{
	register struct madvise_args {
		syscallarg(caddr_t) addr;
		syscallarg(size_t) len;
		syscallarg(int) behav;
	} *uap = (struct madvise_args *) u.u_ap;

	struct proc *p;
	register_t *retval;
	vm_offset_t addr;
	vm_size_t size, pageoff;
	int advice, error;

	p = u.u_procp;
	addr = (vm_offset_t)SCARG(uap, addr);
	size = (vm_size_t)SCARG(uap, len);
	advice = SCARG(uap, behav);

	/*
	 * align the address to a page boundary, and adjust the size accordingly
	 */

	pageoff = (addr & PAGE_MASK);
	addr -= pageoff;
	size += pageoff;
	size = (vm_size_t)round_page(size);

	if ((ssize_t)size <= 0)
		return (EINVAL);

	switch (advice) {
	case MADV_NORMAL:
	case MADV_RANDOM:
	case MADV_SEQUENTIAL:

		error = vm_map_advice(&p->p_vmspace->vm_map, addr, addr + size, advice);
		break;

	case MADV_WILLNEED:

		/*
		 * Activate all these pages, pre-faulting them in if
		 * necessary.
		 */

		error = vm_map_willneed(&p->p_vmspace->vm_map, addr, addr + size);
		break;

	case MADV_DONTNEED:

		/*
		 * Deactivate all these pages.  We don't need them
		 * any more.  We don't, however, toss the data in
		 * the pages.
		 */

		error = vm_map_clean(&p->p_vmspace->vm_map, addr, addr + size, TRUE, FALSE);
		break;

	case MADV_FREE:

		/*
		 * These pages contain no valid data, and may be
		 * garbage-collected.  Toss all resources, including
		 * any swap space in use.
		 */

		error = vm_map_clean(&p->p_vmspace->vm_map, addr, addr + size, TRUE, TRUE);
		break;

	case MADV_SPACEAVAIL:

		/*
		 * XXXMRG What is this?  I think it's:
		 *
		 *	Ensure that we have allocated backing-store
		 *	for these pages.
		 *
		 * This is going to require changes to the page daemon,
		 * as it will free swap space allocated to pages in core.
		 * There's also what to do for device/file/anonymous memory.
		 */

		return (EINVAL);

	default:
		return (EINVAL);
	}

	return (error);
}

/* ARGSUSED */
int
mincore()
{
	register struct mincore_args {
		syscallarg(caddr_t) addr;
		syscallarg(size_t) len;
		syscallarg(char *) vec;
	} *uap = (struct mincore_args *) u.u_ap;

	struct proc *p;
	register_t *retval;

	return (EOPNOTSUPP);
}

int
mlock()
{
	register struct mlock_args {
		syscallarg(caddr_t) addr;
		syscallarg(size_t) len;
	} *uap = (struct mlock_args *) u.u_ap;

	struct proc *p;
	register_t *retval;
	vm_offset_t addr;
	vm_size_t size;
	int error;
	extern int vm_page_max_wired;

	p = u.u_procp;
#ifdef DEBUG
	if (mmapdebug & MDB_FOLLOW)
		printf("mlock(%d): addr %x len %x\n",
		       p->p_pid, SCARG(uap, addr), SCARG(uap, len));
#endif
	addr = (vm_offset_t)SCARG(uap, addr);
	if ((addr & PAGE_MASK) || SCARG(uap, addr) + SCARG(uap, len) < SCARG(uap, addr))
		return (EINVAL);
	size = round_page((vm_size_t)SCARG(uap, len));
	if (atop(size) + cnt.v_page_wire_count > vm_page_max_wired)
		return (EAGAIN);
#ifdef pmap_wired_count
	if (size + ptoa(pmap_wired_count(vm_map_pmap(&p->p_vmspace->vm_map))) >
	    p->p_rlimit[RLIMIT_MEMLOCK].rlim_cur)
		return (EAGAIN);
#else
	if ((error = suser1(p->p_ucred, &p->p_acflag)))
		return (error);
#endif

	error = vm_map_pageable(&p->p_vmspace->vm_map, addr, addr+size, FALSE);
	return (error == KERN_SUCCESS ? 0 : ENOMEM);
}

int
munlock()
{
	register struct munlock_args {
		syscallarg(caddr_t) addr;
		syscallarg(size_t) len;
	} *uap = (struct munlock_args *) u.u_ap;

	struct proc *p;
	register_t *retval;
	vm_offset_t addr;
	vm_size_t size;
	int error;

	p = u.u_procp;
#ifdef DEBUG
	if (mmapdebug & MDB_FOLLOW)
		printf("munlock(%d): addr %x len %x\n", p->p_pid, SCARG(uap, addr),
				SCARG(uap, len));
#endif
	addr = (vm_offset_t) SCARG(uap, addr);
	if ((addr & PAGE_MASK)
			|| SCARG(uap, addr) + SCARG(uap, len) < SCARG(uap, addr))
		return (EINVAL);
#ifndef pmap_wired_count
	if ((error = suser1(p->p_ucred, &p->p_acflag)))
		return (error);
#endif
	size = round_page((vm_size_t)SCARG(uap, len));

	error = vm_map_pageable(&p->p_vmspace->vm_map, addr, addr + size, TRUE);
	return (error == KERN_SUCCESS ? 0 : ENOMEM);
}

/*
 * Internal version of mmap.
 * Currently used by mmap, exec, and sys5 shared memory.
 * Handle is either a vnode pointer or NULL for MAP_ANON.
 */
int
vm_mmap(map, addr, size, prot, maxprot, flags, handle, foff)
	register vm_map_t map;
	register vm_offset_t *addr;
	register vm_size_t size;
	vm_prot_t prot, maxprot;
	register int flags;
	caddr_t handle;		/* XXX should be vp */
	vm_offset_t foff;
{
	register vm_pager_t pager;
	bool_t fitit;
	vm_object_t object;
	struct vnode *vp = NULL;
	int type;
	int rv = KERN_SUCCESS;

	if (size == 0)
		return (0);

	if ((flags & MAP_FIXED) == 0) {
		fitit = TRUE;
		*addr = round_page(*addr);
	} else {
		fitit = FALSE;
		(void) vm_deallocate(map, *addr, size);
	}

	/*
	 * Lookup/allocate pager.  All except an unnamed anonymous lookup
	 * gain a reference to ensure continued existance of the object.
	 * (XXX the exception is to appease the pageout daemon)
	 */
	if (flags & MAP_ANON)
		type = PG_DFLT;
	else {
		vp = (struct vnode*) handle;
		if (vp->v_type == VCHR) {
			type = PG_DEVICE;
			handle = (caddr_t) vp->v_rdev;
		} else
			type = PG_VNODE;
	}
	pager = vm_pager_allocate(type, handle, size, prot, foff);
	if (pager == NULL)
		return (type == PG_DEVICE ? EINVAL : ENOMEM);
	/*
	 * Find object and release extra reference gained by lookup
	 */
	object = vm_object_lookup(pager);
	vm_object_deallocate(object);

	/*
	 * Anonymous memory.
	 */
	if (flags & MAP_ANON) {
		rv = vm_allocate_with_pager(map, addr, size, fitit, pager, foff, TRUE);
		if (rv != KERN_SUCCESS) {
			if (handle == NULL)
				vm_pager_deallocate(pager);
			else
				vm_object_deallocate(object);
			goto out;
		}
		/*
		 * Don't cache anonymous objects.
		 * Loses the reference gained by vm_pager_allocate.
		 * Note that object will be NULL when handle == NULL,
		 * this is ok since vm_allocate_with_pager has made
		 * sure that these objects are uncached.
		 */
		(void) pager_cache(object, FALSE);
#ifdef DEBUG
		if (mmapdebug & MDB_MAPIT)
			printf("vm_mmap(%d): ANON *addr %x size %x pager %x\n",
			       curproc->p_pid, *addr, size, pager);
#endif
	}
	/*
	 * Must be a mapped file.
	 * Distinguish between character special and regular files.
	 */
	else if (vp->v_type == VCHR) {
		rv = vm_allocate_with_pager(map, addr, size, fitit, pager, foff, FALSE);
		/*
		 * Uncache the object and lose the reference gained
		 * by vm_pager_allocate().  If the call to
		 * vm_allocate_with_pager() was sucessful, then we
		 * gained an additional reference ensuring the object
		 * will continue to exist.  If the call failed then
		 * the deallocate call below will terminate the
		 * object which is fine.
		 */
		(void) pager_cache(object, FALSE);
		if (rv != KERN_SUCCESS)
			goto out;
	}
	/*
	 * A regular file
	 */
	else {
#ifdef DEBUG
		if (object == NULL)
			printf("vm_mmap: no object: vp %x, pager %x\n",
			       vp, pager);
#endif
		/*
		 * Map it directly.
		 * Allows modifications to go out to the vnode.
		 */
		if (flags & MAP_SHARED) {
			rv = vm_allocate_with_pager(map, addr, size, fitit, pager, foff,
					FALSE);
			if (rv != KERN_SUCCESS) {
				vm_object_deallocate(object);
				goto out;
			}
			/*
			 * Don't cache the object.  This is the easiest way
			 * of ensuring that data gets back to the filesystem
			 * because vnode_pager_deallocate() will fsync the
			 * vnode.  pager_cache() will lose the extra ref.
			 */
			if (prot & VM_PROT_WRITE)
				pager_cache(object, FALSE);
			else
				vm_object_deallocate(object);
		}
		/*
		 * Copy-on-write of file.  Two flavors.
		 * MAP_COPY is true COW, you essentially get a snapshot of
		 * the region at the time of mapping.  MAP_PRIVATE means only
		 * that your changes are not reflected back to the object.
		 * Changes made by others will be seen.
		 */
		else {
			vm_map_t tmap;
			vm_offset_t off;

			/* locate and allocate the target address space */
			rv = vm_map_find(map, NULL, (vm_offset_t) 0, addr, size, fitit);
			if (rv != KERN_SUCCESS) {
				vm_object_deallocate(object);
				goto out;
			}
			tmap = vm_map_create(pmap_create(size), VM_MIN_ADDRESS,
			VM_MIN_ADDRESS + size, TRUE);
			off = VM_MIN_ADDRESS;
			rv = vm_allocate_with_pager(tmap, &off, size, TRUE, pager, foff,
					FALSE);
			if (rv != KERN_SUCCESS) {
				vm_object_deallocate(object);
				vm_map_deallocate(tmap);
				goto out;
			}
			/*
			 * (XXX)
			 * MAP_PRIVATE implies that we see changes made by
			 * others.  To ensure that we need to guarentee that
			 * no copy object is created (otherwise original
			 * pages would be pushed to the copy object and we
			 * would never see changes made by others).  We
			 * totally sleeze it right now by marking the object
			 * internal temporarily.
			 */
			if ((flags & MAP_COPY) == 0)
				object->flags |= OBJ_INTERNAL;
			rv = vm_map_copy(map, tmap, *addr, size, off, FALSE, FALSE);
			object->flags &= ~OBJ_INTERNAL;
			/*
			 * (XXX)
			 * My oh my, this only gets worse...
			 * Force creation of a shadow object so that
			 * vm_map_fork will do the right thing.
			 */
			if ((flags & MAP_COPY) == 0) {
				vm_map_t tmap;
				vm_map_entry_t tentry;
				vm_object_t tobject;
				vm_offset_t toffset;
				vm_prot_t tprot;
				bool_t twired, tsu;

				tmap = map;
				vm_map_lookup(&tmap, *addr, VM_PROT_WRITE, &tentry, &tobject,
						&toffset, &tprot, &twired, &tsu);
				vm_map_lookup_done(tmap, tentry);
			}
			/*
			 * (XXX)
			 * Map copy code cannot detect sharing unless a
			 * sharing map is involved.  So we cheat and write
			 * protect everything ourselves.
			 */
			vm_object_pmap_copy(object, foff, foff + size);
			vm_object_deallocate(object);
			vm_map_deallocate(tmap);
			if (rv != KERN_SUCCESS)
				goto out;
		}
#ifdef DEBUG
		if (mmapdebug & MDB_MAPIT)
			printf("vm_mmap(%d): FILE *addr %x size %x pager %x\n",
			       curproc->p_pid, *addr, size, pager);
#endif
	}
	/*
	 * Correct protection (default is VM_PROT_ALL).
	 * If maxprot is different than prot, we must set both explicitly.
	 */
	rv = KERN_SUCCESS;
	if (maxprot != VM_PROT_ALL)
		rv = vm_map_protect(map, *addr, *addr + size, maxprot, TRUE);
	if (rv == KERN_SUCCESS && prot != maxprot)
		rv = vm_map_protect(map, *addr, *addr + size, prot, FALSE);
	if (rv != KERN_SUCCESS) {
		(void) vm_deallocate(map, *addr, size);
		goto out;
	}
	/*
	 * Shared memory is also shared with children.
	 */
	if (flags & MAP_SHARED) {
		rv = vm_map_inherit(map, *addr, *addr + size, VM_INHERIT_SHARE);
		if (rv != KERN_SUCCESS) {
			(void) vm_deallocate(map, *addr, size);
			goto out;
		}
	}
out:
#ifdef DEBUG
	if (mmapdebug & MDB_MAPIT)
		printf("vm_mmap: rv %d\n", rv);
#endif
	switch (rv) {
	case KERN_SUCCESS:
		return (0);
	case KERN_INVALID_ADDRESS:
	case KERN_NO_SPACE:
		return (ENOMEM);
	case KERN_PROTECTION_FAILURE:
		return (EACCES);
	default:
		return (EINVAL);
	}
}
