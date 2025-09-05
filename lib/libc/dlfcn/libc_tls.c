/*	$NetBSD: tls.c,v 1.6 2011/04/07 02:19:28 matt Exp $	*/

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Joerg Sonnenberger.
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
__RCSID("$NetBSD: tls.c,v 1.6 2011/04/07 02:19:28 matt Exp $");

#include <sys/param.h>
#include <sys/mman.h>
#include <sys/tls.h>

#include <link_elf.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const void *tls_initaddr;
static size_t tls_initsize;
static size_t tls_size;
static size_t tls_allocation;
static void *initial_thread_tcb;

void * __libc_tls_get_addr(void);
static int __static_tls_setup_cb(struct dl_phdr_info *, size_t, void *);
void __static_tls_setup(void) __dso_hidden;

void *
__libc_tls_get_addr(void)
{
	abort();
	/* NOTREACHED */
}

struct tls_tcb *
_rtld_tls_allocate(void)
{
	struct tls_tcb *tcb;
	char *p;

	if (initial_thread_tcb == NULL) {
		tls_allocation = tls_size + sizeof(*tcb);

		initial_thread_tcb = p = mmap(NULL, tls_allocation,
				PROT_READ | PROT_WRITE, MAP_ANON, -1, 0);
	} else {
		p = calloc(1, tls_allocation);
	}
	if (p == NULL) {
		static const char msg[] =  "TLS allocation failed, terminating\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		_exit(127);
	}
	tcb = (struct tls_tcb *)(p + tls_size);
	tcb->tcb_self = tcb;
	memcpy(p, tls_initaddr, tls_initsize);

	return (tcb);
}

void
_rtld_tls_free(struct tls_tcb *tcb)
{
	char *p;

	p = (char *)tcb - tls_size;
	if (p == initial_thread_tcb) {
		munmap(p, tls_allocation);
	} else {
		free(p);
	}
}

static int
__static_tls_setup_cb(struct dl_phdr_info *data, size_t len, void *cookie)
{
	const Elf_Phdr *phdr;
	const Elf_Phdr *phlimit;

	phdr = data->dlpi_phdr;
	phlimit = data->dlpi_phdr + data->dlpi_phnum;
	for (; phdr < phlimit; ++phdr) {
		if (phdr->p_type != PT_TLS) {
			continue;
		}
		tls_initaddr = (void *)(phdr->p_vaddr + data->dlpi_addr);
		tls_initsize = phdr->p_filesz;
		tls_size = phdr->p_memsz;
	}
	return (0);
}

void
__static_tls_setup(void)
{
	struct tls_tcb *tcb;

	dl_iterate_phdr(__static_tls_setup_cb, NULL);

	tcb = _rtld_tls_allocate();

	/*
	 * This Okay for i386, but
	 * will need to change
	 * when other architectures
	 * come into play.
	 */
#if defined(i386)
	settls(tcb, GSBASE);
#endif
}
