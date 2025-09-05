/*
 * The 3-Clause BSD License:
 * Copyright (c) 2025 Martin Kelly
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

#ifndef SYS_TLS_H_
#define SYS_TLS_H_

#include <sys/cdefs.h>
#include <machine/types.h>

struct tls_tcb {
	void	*tcb_self;
	void	**tcb_dtv;
	void	*tcb_pthread;
};

/* which args */
#define FSBASE 'f'
#define GSBASE 'g'

/*
 * cmd options for tls syscalls
 * - tls: set, get
 */
enum tls_cmdops {
	GETTLS,
	SETTLS,
};

#ifdef _KERNEL
int cpu_get_tls_tcb(struct proc *, void *, char);
int cpu_set_tls_tcb(struct proc *, void *, char);
#else
__BEGIN_DECLS
/* kernel tls syscall callback */
int tls(int, void *, char);
/* get tls */
int gettls(void *, char);
/* set tls */
int settls(void *, char);
struct tls_tcb *_rtld_tls_allocate(void);
void _rtld_tls_free(struct tls_tcb *);
__END_DECLS
#endif /* !_KERNEL */

#endif /* SYS_TLS_H_ */
