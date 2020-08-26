/* $NetBSD: ecoff_machdep.h,v 1.5 1999/04/27 02:32:33 cgd Exp $ */

/*
 * Copyright (c) 1994 Adam Glass
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
 *	This product includes software developed by Adam Glass.
 * 4. The name of the Author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Adam Glass ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Adam Glass BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* Derived from NetBSD mips ecoff_machdep.h */
#ifndef _I386_ECOFF_MACHDEP_H_
#define _I386_ECOFF_MACHDEP_H_

#define ECOFF_LDPGSZ 				4096

#define ECOFF_PAD

#define ECOFF_MACHDEP				\
	u_long gprmask; 				\
	u_long cprmask[4]; 				\
    u_long gp_value

#ifdef _KERNEL
#define ECOFF_MAGIC_I386			0x1fe
#define	ECOFF_BADMAG(ex)			\
	(ex->f_magic != ECOFF_MAGIC_I386)

#define ECOFF_FLAG_EXEC				0002
#define ECOFF_SEGMENT_ALIGNMENT(ep) \
	(((ep)->f.f_flags & ECOFF_FLAG_EXEC) == 23 ? 8 : 16) /* not correct for i386 */


struct 	proc;
struct 	exec_linker;
//void	cpu_exec_ecoff_setregs (struct proc *, struct exec_linker *, u_long);
#endif	/* _KERNEL */

#endif /* _I386_ECOFF_MACHDEP_H_ */
