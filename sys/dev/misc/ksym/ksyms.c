/*	$NetBSD: kern_ksyms.c,v 1.88 2020/01/05 21:12:34 pgoyette Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software developed for The NetBSD Foundation
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

/*
 * Copyright (c) 2001, 2003 Anders Magnusson (ragge@ludd.luth.se).
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

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/exec.h>
#include <sys/systm.h>
#include <sys/uio.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/fcntl.h>
#include <sys/conf.h>
#include <sys/devsw.h>
#include <sys/exec_elf.h>
#include <sys/ioccom.h>
#include <sys/queue.h>
#include <sys/user.h>
#include <sys/lock.h>
#define _KSYMS_PRIVATE
#include <sys/ksyms.h>

dev_type_open(ksymsopen);
dev_type_close(ksymsclose);
dev_type_read(ksymsread);
dev_type_write(ksymswrite);

const struct cdevsw ksyms_cdevsw = {
		.d_open = ksymsopen,
		.d_close = ksymsclose,
		.d_read = ksymsread,
		.d_write = ksymswrite,
		.d_stop = nullstop,
		.d_tty = notty,
		.d_poll = nopoll,
		.d_mmap = nommap,
		.d_discard = nodiscard,
		.d_type = D_OTHER
};

void
ksymsattach(int num)
{

}

int
ksymsopen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	if (minor(dev) != 0 || !ksyms_loaded) {
		return (ENXIO);
	}

	/*
	 * Create a "snapshot" of the kernel symbol table.  Setting
	 * ksyms_isopen will prevent symbol tables from being freed.
	 */
	simple_lock(&ksyms_lock);
	ksyms_hdr.kh_shdr[SYMTAB].sh_size = ksyms_symsz;
	ksyms_hdr.kh_shdr[SYMTAB].sh_info = ksyms_symsz / sizeof(Elf_Sym);
	ksyms_hdr.kh_shdr[STRTAB].sh_offset = ksyms_symsz + ksyms_hdr.kh_shdr[SYMTAB].sh_offset;
	ksyms_hdr.kh_shdr[STRTAB].sh_size = ksyms_strsz;
	ksyms_hdr.kh_shdr[SHCTF].sh_offset = ksyms_strsz + ksyms_hdr.kh_shdr[STRTAB].sh_offset;
	ksyms_hdr.kh_shdr[SHCTF].sh_size = ksyms_ctfsz;
	ksyms_isopen = TRUE;
	simple_unlock(&ksyms_lock);

	return (0);
}

int
ksymsclose(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	//ksyms_delsymtab();
	return (0);
}

int
ksymsread(dev, uio, flags)
	dev_t dev;
	struct uio *uio;
	int flags;
{
	struct ksyms_symtab *st;
	size_t filepos, inpos, off;
	int error;

	/*
	 * First: Copy out the ELF header.   XXX Lose if ksymsopen()
	 * occurs during read of the header.
	 */
	off = uio->uio_offset;
	if (off < sizeof(struct ksyms_hdr)) {
		error = uiomove((char *)&ksyms_hdr + off,
		    sizeof(struct ksyms_hdr) - off, uio);
		if (error != 0)
			return error;
	}

	/*
	 * Copy out the symbol table.
	 */
	filepos = sizeof(struct ksyms_hdr);
	TAILQ_FOREACH(st, &ksyms_symtabs, sd_queue) {
		if (st->sd_gone == FALSE)
			continue;
		if (uio->uio_resid == 0)
			return 0;
		if (uio->uio_offset <= st->sd_symsize + filepos) {
			inpos = uio->uio_offset - filepos;
			error = uiomove((char *)st->sd_symstart + inpos,
			   st->sd_symsize - inpos, uio);
			if (error != 0)
				return error;
		}
		filepos += st->sd_symsize;
	}

	/*
	 * Copy out the string table
	 */
	KASSERT(filepos == sizeof(struct ksyms_hdr) +
	    ksyms_hdr.kh_shdr[SYMTAB].sh_size);
	TAILQ_FOREACH(st, &ksyms_symtabs, sd_queue) {
		if (st->sd_gone == FALSE)
			continue;
		if (uio->uio_resid == 0)
			return 0;
		if (uio->uio_offset <= st->sd_strsize + filepos) {
			inpos = uio->uio_offset - filepos;
			error = uiomove((char *)st->sd_strstart + inpos,
			   st->sd_strsize - inpos, uio);
			if (error != 0)
				return error;
		}
		filepos += st->sd_strsize;
	}

	/*
	 * Copy out the CTF table.
	 */
	st = TAILQ_FIRST(&ksyms_symtabs);
	if (st->sd_ctfstart != NULL) {
		if (uio->uio_resid == 0)
			return 0;
		if (uio->uio_offset <= st->sd_ctfsize + filepos) {
			inpos = uio->uio_offset - filepos;
			error = uiomove((char *)st->sd_ctfstart + inpos,
			    st->sd_ctfsize - inpos, uio);
			if (error != 0)
				return error;
		}
		filepos += st->sd_ctfsize;
	}

	return (0);
}

int
ksymswrite(dev, uio, flags)
	dev_t dev;
	struct uio *uio;
	int flags;
{
	return (EROFS);
}
