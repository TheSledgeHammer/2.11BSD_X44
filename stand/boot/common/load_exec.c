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
#include <sys/user.h>

#include <lib/libsa/loadfile.h>
#include <lib/libsa/stand.h>
#include "bootstrap.h"

#include <machine/loadfile_machdep.h>

/*
 * Attempts to load the file (file) as an module defined by the parameter 'kerneltype'.
 * Kernel types include aout, ecoff, elf32, elf64, xcoff32 & xcoff64.
 * It will be stored at (dest), and a pointer to a module structure describing the loaded object
 * will be saved in (result).
 * The functions for the different files it can load as a module are below.
 */
int
exec_loadfile(char *kerneltype, char *filename, u_long dest, struct preloaded_file **result)
{
    	struct preloaded_file	*fp;
    	u_long                  *marks;
    	int			err;
    	u_int			pad;
    	ssize_t			bytes_read;
    	int                     fd;

	err = loadfile_header(kerneltype, filename);
	if (err != 0)
		goto out;

	marks = fp->f_marks;
	fp->f_name = strdup(filename);
	fp->f_type = strdup(kerneltype);

	marks[MARK_START] = dest;

	if ((fd = loadfile(filename, marks, LOAD_KERNEL)) == -1) {
		err = EPERM;
		goto oerr;
	}
	close(fd);

	dest = marks[MARK_ENTRY];

	printf("%s entry at 0x%lx\n", filename, (uintmax_t) dest);

	fp->f_size = marks[MARK_END] - marks[MARK_START];
	fp->f_addr = marks[MARK_START];

	if (fp->f_size == 0 || fp->f_addr == 0)
		goto ioerr;

	/* Load OK, return module pointer */
	*result = fp;
	err = 0;
	goto out;

ioerr:
	err = EIO;
oerr:
    	file_discard(fp);
out:
	return(err);
}

int
loadfile_header(char *kerneltype, char *filename)
{
	struct preloaded_file	*fp;
	int						err;

    	/*
     	* Open the image, read and validate header
     	*/
	 if (filename == NULL) {	/* can't handle nameless */
		 return(EFTYPE);
	 }

	 fp = file_alloc();
	 if (fp == NULL) {
#ifdef BOOT_AOUT
	    printf("aout_loadfile: cannot allocate module info\n");
#endif
#ifdef BOOT_ECOFF
	    printf("ecoff_loadfile: cannot allocate module info\n");
#endif
#ifdef BOOT_ELF32
	    printf("elf32_loadfile: cannot allocate module info\n");
#endif
#ifdef BOOT_ELF64
	    printf("elf64_loadfile: cannot allocate module info\n");
#endif
#ifdef BOOT_XCOFF32
	    printf("xcoff32_loadfile: cannot allocate module info\n");
#endif
#ifdef BOOT_XCOFF64
	    printf("xcoff64_loadfile: cannot allocate module info\n");
#endif
		 err = EPERM;
	 }
	 return (err);
}

int
aout_loadfile(char *filename, uint64_t dest, struct preloaded_file **result)
{
	return (exec_loadfile(AOUT_KERNELTYPE, filename, dest, result));
}

int
ecoff_loadfile(char *filename, uint64_t dest, struct preloaded_file **result)
{
	return (exec_loadfile(ECOFF_KERNELTYPE, filename, dest, result));
}

int
elf32_loadfile(char *filename, uint64_t dest, struct preloaded_file **result)
{
	return (exec_loadfile(ELF32_KERNELTYPE, filename, dest, result));
}

int
elf64_loadfile(char *filename, u_int64_t dest, struct preloaded_file **result)
{
	return (exec_loadfile(ELF64_KERNELTYPE, filename, dest, result));
}

int
xcoff32_loadfile(char *filename, u_int64_t dest, struct preloaded_file **result)
{
	return (exec_loadfile(XCOFF32_KERNELTYPE, filename, dest, result));
}

int
xcoff64_loadfile(char *filename, u_int64_t dest, struct preloaded_file **result)
{
	return (exec_loadfile(XCOFF64_KERNELTYPE, filename, dest, result));
}
