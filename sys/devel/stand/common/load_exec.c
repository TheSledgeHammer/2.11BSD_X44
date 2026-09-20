/*	$NetBSD: load_elf64.c,v 1.4 2017/10/15 01:28:32 maya Exp $	*/

/*-
 * Copyright (c) 1998 Michael Smith <msmith@freebsd.org>
 * Copyright (c) 1998 Peter Wemm <peter@freebsd.org>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
/* __FBSDID("$FreeBSD: src/sys/boot/common/load_elf.c,v 1.30.2.1 2004/09/03 19:25:40 iedowse Exp $"); */

#include <sys/param.h>
#include <sys/exec.h>

#include <lib/libsa/loadfile.h>
#include <lib/libsa/stand.h>

#include <bootstrap.h>

static int loadfile_header(char *, char *);

/*
 * Attempts to load the file (file) as an module defined by the parameter 'kerneltype'.
 * Kernel types include aout, ecoff, elf32, elf64, xcoff32 & xcoff64.
 * It will be stored at (dest), and a pointer to a module structure describing the loaded object
 * will be saved in (result).
 * The functions for the different files it can load as a module are below.
 */
int
exec_loadfile(char *filename, char *kerneltype, uint64_t dest, int flags, struct preloaded_file **result)
{
	struct preloaded_file *fp;
	u_long *marks;
	int err;
	u_int pad;
	ssize_t bytes_read;
	int fd;

	err = loadfile_header(filename, kerneltype);
	if (err != 0) {
		goto out;
	}

	marks = fp->f_marks;
	fp->f_name = strdup(filename);
	fp->f_type = strdup(kerneltype);

	marks[MARK_START] = dest;
	fd = loadfile(filename, marks, flags);
	if (fd == -1) {
		err = EPERM;
		goto oerr;
	}
	close(fd);

	dest = marks[MARK_ENTRY];

	printf("%s entry at 0x%lx\n", filename, (uintmax_t)dest);

	if (fp->f_marks[MARK_ENTRY] != marks[MARK_ENTRY]) {
		fp->f_marks[MARK_ENTRY] = marks[MARK_ENTRY];
	}
	fp->f_size = marks[MARK_END] - marks[MARK_START];
	fp->f_addr = marks[MARK_START];

	if (fp->f_size == 0 || fp->f_addr == 0) {
		err = EIO;
		goto oerr;
	}

	/* Load OK, return module pointer */
	*result = fp;
	err = 0;
	goto out;

oerr:
	file_discard(fp);
out:
	return (err);
}

static int
loadfile_header(char *filename, char *kerneltype)
{
	struct preloaded_file *fp;

	/*
	 * Open the image, read and validate header
	 */
	if (filename == NULL) { /* can't handle nameless */
		return (EFTYPE);
	}

	fp = file_alloc();
	if (fp == NULL) {
		printf("exec_loadfile: cannot allocate info\n");
		return (EPERM);
	}
	return (0);
}
