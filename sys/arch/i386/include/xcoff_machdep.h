/*
 * The 3-Clause BSD License:
 * Copyright (c) 2020 Martin Kelly
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
 */

#ifndef _I386_XCOFF_MACHDEP_H_
#define _I386_XCOFF_MACHDEP_H_

#define XCOFF_LDPGSZ 				4096

#define	KERN_XCOFFSIZE				32
#define ARCH_XCOFFSIZE				32	/* MD native binary size */

#define XCOFF_PAD

#define XCOFF_MACHDEP				\
	u_long gprmask; 				\
	u_long cprmask[4]; 				\
    u_long gp_value


#ifdef _KERNEL
#define XCOFF_MAGIC_I386			0x1fe
#define	XCOFF_BADMAG(ex)			\
	(ex->f_magic != ECOFF_MAGIC_I386)

#define XCOFF_FLAG_EXEC				0002
#define XCOFF_SEGMENT_ALIGNMENT(ep) \
	(((ep)->f.f_flags & XCOFF_FLAG_EXEC) == 23 ? 8 : 16) /* not correct for i386? */

struct 	proc;
struct 	exec_linker;
#endif	/* _KERNEL */

struct xcoff_symhdr {

};

#endif /* _I386_XCOFF_MACHDEP_H_ */
