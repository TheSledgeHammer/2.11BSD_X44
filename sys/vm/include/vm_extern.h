/*-
 * Copyright (c) 1992, 1993
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
 *
 *	@(#)vm_extern.h	8.5 (Berkeley) 5/3/95
 */

struct buf;
struct loadavg;
struct proc;
struct vmspace;
struct vmtotal;
struct mount;
struct vnode;

#ifdef KGDB
void		 	chgkprot (caddr_t, int, int);
#endif

#ifdef KERNEL
#ifdef TYPEDEF_FOR_UAP
int		 		compat_43_getpagesize (struct proc *p, void *, int *);
int		 		madvise (struct proc *, void *, int *);
int		 		mincore (struct proc *, void *, int *);
int		 		mprotect (struct proc *, void *, int *);
int		 		msync (struct proc *, void *, int *);
int		 		munmap (struct proc *, void *, int *);
int		 		obreak (struct proc *, void *, int *);
int				sbrk (struct proc *, void *, int *);
int		 		smmap (struct proc *, void *, int *);
int		 		sstk (struct proc *, void *, int *);
#endif

void		 	assert_wait (void *, boolean_t);
int		 		grow (struct proc *, vm_offset_t);
void		 	iprintf (const char *, ...);
int		 		kernacc (caddr_t, int, int);
vm_offset_t	 	kmem_alloc (vm_map_t, vm_size_t);
vm_offset_t	 	kmem_alloc_pageable (vm_map_t, vm_size_t);
vm_offset_t	 	kmem_alloc_wait (vm_map_t, vm_size_t);
void		 	kmem_free (vm_map_t, vm_offset_t, vm_size_t);
void		 	kmem_free_wakeup (vm_map_t, vm_offset_t, vm_size_t);
void		 	kmem_init (vm_offset_t, vm_offset_t);
vm_offset_t	 	kmem_malloc (vm_map_t, vm_size_t, boolean_t);
vm_map_t	 	kmem_suballoc (vm_map_t, vm_offset_t *, vm_offset_t *, vm_size_t, boolean_t);
void		 	loadav (struct loadavg *);
void		 	munmapfd (struct proc *, int);
int		 		pager_cache (vm_object_t, boolean_t);
void			sched (void);
int		 		svm_allocate (struct proc *, void *, int *);
int		 		svm_deallocate (struct proc *, void *, int *);
int		 		svm_inherit (struct proc *, void *, int *);
int		 		svm_protect (struct proc *, void *, int *);
void			swapinit (void);
int		 		swapon (struct proc *, void *, int *);
void		 	swapout (struct proc *);
void		 	swapout_threads (void);
int			 	swfree (struct proc *, int);
void		 	swstrategy (struct buf *);
void		 	thread_block (void);
void		 	thread_sleep (void *, simple_lock_t, boolean_t);
void			thread_wakeup (void *);
int		 		useracc (caddr_t, int, int);
int		 		vm_allocate (vm_map_t, vm_offset_t *, vm_size_t, boolean_t);
int		 		vm_allocate_with_pager (vm_map_t, vm_offset_t *, vm_size_t, boolean_t, vm_pager_t, vm_offset_t, boolean_t);
int		 		vm_deallocate (vm_map_t, vm_offset_t, vm_size_t);
int		 		vm_fault (vm_map_t, vm_offset_t, vm_prot_t, boolean_t);
void		 	vm_fault_copy_entry (vm_map_t, vm_map_t, vm_map_entry_t, vm_map_entry_t);
void		 	vm_fault_unwire (vm_map_t, vm_offset_t, vm_offset_t);
int		 		vm_fault_wire (vm_map_t, vm_offset_t, vm_offset_t);
int		 		vm_fork (struct proc *, struct proc *, int);
int		 		vm_inherit (vm_map_t, vm_offset_t, vm_size_t, vm_inherit_t);
void			vm_init_limits (struct proc *);
void		 	vm_mem_init (void);
int		 		vm_mmap (vm_map_t, vm_offset_t *, vm_size_t, vm_prot_t, vm_prot_t, int, caddr_t, vm_offset_t);
int		 		vm_protect (vm_map_t, vm_offset_t, vm_size_t, boolean_t, vm_prot_t);
void		 	vm_set_page_size (void);
int		 		vm_sysctl (int *, u_int, void *, size_t *, void *, size_t, struct proc *);
void		 	vmmeter (void);
struct vmspace	*vmspace_alloc (vm_offset_t, vm_offset_t, int);
struct vmspace	*vmspace_fork (struct vmspace *);
void		 	vmspace_free (struct vmspace *);
void		 	vmtotal (struct vmtotal *);
void		 	vnode_pager_setsize (struct vnode *, u_long);
void		 	vnode_pager_umount (struct mount *);
boolean_t	 	vnode_pager_uncache (struct vnode *);
void		 	vslock (caddr_t, u_int);
void		 	vsunlock (caddr_t, u_int, int);
#endif
