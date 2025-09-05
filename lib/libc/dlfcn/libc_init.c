/* 	$NetBSD: initfini.c,v 1.14 2017/06/17 15:26:44 joerg Exp $	 */

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: initfini.c,v 1.14 2017/06/17 15:26:44 joerg Exp $");

#ifdef _LIBC
#include "namespace.h"
#endif

#include <sys/types.h>
#include <sys/exec.h>

#include "dlfcn_private.h"

void	_libc_init(void) __attribute__((__constructor__, __used__));

void	__guard_setup(void);
void	__atexit_init(void);
//void 	__static_tls_setup(void);

static int libc_initialised;

void
_libc_init(void)
{
	if (libc_initialised) {
		return;
	}

	libc_initialised = 1;

	/* Only initialize _dlauxinfo for static binaries. */
	if (__ps_strings != NULL) {
		dlauxinfo_init(__ps_strings->ps_nargvstr, &__ps_strings->ps_argvstr,
				__ps_strings->ps_nenvstr);
	}

	/* For -fstack-protector */
	__guard_setup();

	/* Initialize TLS for statically linked programs. */
	//__static_tls_setup();

	/* Initialize the atexit mutexes */
	__atexit_init();
}
