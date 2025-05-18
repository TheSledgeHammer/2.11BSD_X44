/*	$NetBSD: common.c,v 1.20 2008/04/28 20:22:54 martin Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
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

/* Work in Progress: missing load_rtld */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: dlfcn_elf.c,v 1.4.2.1 2004/07/19 09:07:13 tron Exp $");
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"
#include <sys/atomic.h>
#include <sys/mman.h>
#include <assert.h>
#include <aout.h>
#include <link.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>

#undef dlopen
#undef dlclose
#undef dlsym
#undef dlctl
#undef dlerror
#undef dladdr

#define	dlopen		___dlopen
#define	dlclose		___dlclose
#define	dlsym		___dlsym
#define	dlctl		___dlctl
#define	dlerror		___dlerror
#define	dladdr		___dladdr

#include "rtld.h"

#ifdef __weak_alias
__weak_alias(dlopen,___dlopen)
__weak_alias(dlclose,___dlclose)
__weak_alias(dlsym,___dlsym)
__weak_alias(dlctl,___dlctl)
__weak_alias(dlerror,___dlerror)
__weak_alias(dladdr,___dladdr)

__weak_alias(__dlopen,___dlopen)
__weak_alias(__dlclose,___dlclose)
__weak_alias(__dlsym,___dlsym)
__weak_alias(__dlctl,___dlctl)
__weak_alias(__dlerror,___dlerror)
__weak_alias(__dladdr,___dladdr)
#endif

#ifdef DYNAMIC

#ifndef LDSO
#define LDSO	"/usr/libexec/ld.so"
#endif

#define _FATAL(str) 					\
	write(2, str, sizeof(str) - 1); 	\
	exit(1);

static struct ld_entry **ld_entry;
static char dlfcn_error[] = "Service unavailable";
static char crt0_update[] = "crt0: update ld.so";

typedef int (*rtld_entry_fn)(int, struct crt_ldso *);

static rtld_entry_fn rtld_entry(rtld_entry_fn, int, struct crt_ldso *);
static void rtld_ldso(struct _dynamic *dp, struct ld_entry **lde, struct crt_ldso *crt, struct exec *hdr);
static void load_rtld(struct _dynamic *);
static void *lde_dlopen(struct ld_entry **, const char *, int);
static int 	lde_dlclose(struct ld_entry **, void *);
static void *lde_dlsym(struct ld_entry **, void *, const char *);
static int 	lde_dlctl(struct ld_entry **, void *, void *, int);
static char *lde_dlerror(struct ld_entry **);
static int 	lde_dladdr(struct ld_entry **, const void *, Dl_info *);

/*
 * DL stubs
 */
/*ARGSUSED*/
void *
dlopen(const char *name, int mode)
{
	return (lde_dlopen(ld_entry, name, mode));
}

/*ARGSUSED*/
int
dlclose(void *fd)
{
	return (lde_dlclose(ld_entry, fd));
}

/*ARGSUSED*/
void *
dlsym(void *handle, const char *name)
{
	return (lde_dlsym(ld_entry, handle, name));
}

/*ARGSUSED*/
int
dlctl(void *fd, void *arg, int cmd)
{
	return (lde_dlctl(ld_entry, fd, arg, cmd));
}

/*ARGSUSED*/
__aconst char *
dlerror()
{
	return (lde_dlerror(ld_entry));
}

/*ARGSUSED*/
int
dladdr(const void *addr, Dl_info *dli)
{
	return (lde_dladdr(ld_entry, addr, dli));
}

/*
 * rtld init
 */
static rtld_entry_fn
rtld_entry(rtld_entry_fn entry, int version, struct crt_ldso *crt)
{
	return ((*entry)(version, crt));
}

static void
rtld_ldso(struct _dynamic *dp, struct ld_entry **lde, struct crt_ldso *crt, struct exec *hdr)
{
	rtld_entry_fn entry;
	int ret;
	char *data, *bss;

#ifdef DEBUG
	if ((crt->crt_ldso = getenv("LDSO")) == NULL) {
		crt->crt_ldso = LDSO;
	}
#else
	crt->crt_ldso = LDSO;
#endif

	crt->crt_ldfd = open(crt->crt_ldso, 0, 0);
	if (crt->crt_ldfd == -1) {
		_FATAL("No ld.so\n");
	}

	/* Read LDSO exec header */
	ret = read(crt->crt_ldfd, hdr, sizeof(*hdr));
	if (ret < sizeof(*hdr)) {
		_FATAL("Failure reading ld.so\n");
	}
	if (N_GETMAGIC(*hdr) != ZMAGIC && N_GETMAGIC(*hdr) != QMAGIC) {
		_FATAL("Bad magic: ld.so\n");
	}
	/* We use MAP_ANON */
	crt->crt_dzfd = -1;

	crt->crt_ba = mmap(0, *hdr->a_text + *hdr->a_data + *hdr->a_bss,
	PROT_READ | PROT_EXEC,
	MAP_FILE | MAP_PRIVATE, crt->crt_ldfd, N_TXTOFF(*hdr));
	if (crt->crt_ba == -1) {
		_FATAL("Cannot map ld.so\n");
	}

#undef N_DATADDR
#undef N_BSSADDR
#define N_DATADDR(x)	((x)->a_text)
#define N_BSSADDR(x)	((x)->a_text + (x)->a_data)

	/* Map in data segment of ld.so writable */
	data = mmap(crt->crt_ba + N_DATADDR(*hdr), hdr->a_data,
	PROT_READ | PROT_WRITE | PROT_EXEC,
	MAP_FILE | MAP_PRIVATE | MAP_FIXED, crt->crt_ldfd, N_DATOFF(*hdr));
	if (data == -1) {
		_FATAL("Cannot map ld.so\n");
	}

	/* Map bss segment of ld.so zero */
	bss = mmap(crt->crt_ba + N_BSSADDR(*hdr), hdr->a_bss,
	PROT_READ | PROT_WRITE,
	MAP_ANON | MAP_PRIVATE | MAP_FIXED, crt->crt_dzfd, 0);
	if (hdr->a_bss && bss == -1) {
		_FATAL("Cannot map ld.so\n");
	}

	crt->crt_dp = dp;
	crt->crt_ep = environ;
	crt->crt_bp = (caddr_t)_callmain;
	crt->crt_prog = __progname;

	(*lde) = &crt->crt_ldentry;
	entry = (rtld_entry_fn)(crt->crt_ba + sizeof(*hdr));
	if (rtld_entry(entry, CRT_VERSION_BSD_4, crt) == -1) {
		(void)write(2, crt0_update, sizeof(crt0_update)-1);
		if (rtld_entry(entry, CRT_VERSION_BSD_3, crt) == -1) {
			_FATAL("ld.so failed\n");
		}
		(*lde) = &dp->d_entry;
		return;
	}
	atexit((*lde)->dlexit);
	return;
}

static void
load_rtld(struct _dynamic *dp)
{
	static struct crt_ldso crt;
	struct exec	hdr;

	rtld_ldso(dp, ld_entry, &crt, &hdr);
}

/*
 * ld_entry functions
 */
static void *
lde_dlopen(struct ld_entry **lde, const char *name, int mode)
{
	if ((*lde) == NULL) {
		return (NULL);
	}
	return (((*lde)->dlopen)(name, mode));
}

static int
lde_dlclose(struct ld_entry **lde, void *fd)
{
	if ((*lde) == NULL) {
		return (-1);
	}
	return (((*lde)->dlclose)(fd));
}

static void *
lde_dlsym(struct ld_entry **lde, void *handle, const char *name)
{
	if ((*lde) == NULL) {
		return (NULL);
	}
	return (((*lde)->dlsym)(handle, name));
}

static int
lde_dlctl(struct ld_entry **lde, void *fd, void *arg, int cmd)
{
	if ((*lde) == NULL) {
		return (-1);
	}
	return (((*lde)->dlctl)(fd, arg, cmd));
}

static char *
lde_dlerror(struct ld_entry **lde)
{
	int error;

	if ((*lde) == NULL || lde_dlctl(lde, NULL, DL_GETERRNO, &error) == -1) {
		return (dlfcn_error);
	}
	return ((char *)(error == 0 ? NULL : strerror(error)));
}

static int
lde_dladdr(struct ld_entry **lde, const void *addr, Dl_info *dli)
{
	if ((*lde) == NULL || (*lde)->dladdr == NULL) {
		return (0);
	}
	return ((*lde)->dladdr)(addr, dli);
}
#endif /* DYNAMIC */
