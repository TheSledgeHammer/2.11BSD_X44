/*	$NetBSD: uvm_aobj.c,v 1.18.2.1 1999/04/16 16:27:13 chs Exp $	*/

/*
 * Copyright (c) 1998 Chuck Silvers, Charles D. Cranor and
 *                    Washington University.
 * All rights reserved.
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
 *      This product includes software developed by Charles D. Cranor and
 *      Washington University.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * from: Id: uvm_aobj.c,v 1.1.2.5 1998/02/06 05:14:38 chs Exp
 */
/*
 * uvm_aobj.c: anonymous memory uvm_object pager
 *
 * author: Chuck Silvers <chuq@chuq.com>
 * started: Jan-1998
 *
 * - design mostly from Chuck Cranor
 */

#ifndef _VM_AOBJECT_PAGER_H_
#define _VM_AOBJECT_PAGER_H_

#include <devel/vm/uvm/vm_aobject.h>

/*
 * globals
 */
struct aobjpager {
	vm_aobject_t				vm_aobject;
};
typedef struct aobjpager		*aobj_pager_t;

/*
 * local functions
 */
static void						uao_init (void);
static boolean_t				uao_flush (struct vm_object *, vaddr_t, vaddr_t, int);
static int						uao_get (struct vm_object *, vaddr_t, vm_page_t *, int *, int, vm_prot_t, int, int);
static boolean_t				uao_releasepg (struct vm_page *, struct vm_page **);
void 							uao_dropswap (struct vm_object *, int);
struct vm_object				*uao_create (vm_size_t, int);
void							uao_detach (struct vm_object *);
void							uao_reference (struct vm_object *);

#endif /* _VM_AOBJECT_PAGER_H_ */
