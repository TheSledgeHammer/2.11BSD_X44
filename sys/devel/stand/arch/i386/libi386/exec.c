/*
 * The 3-Clause BSD License:
 * Copyright (c) 2026 Martin Kelly
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

#include <lib/libsa/stand.h>
#include <lib/libsa/loadfile.h>

#include <stand/common/bootstrap.h>
#include <libi386.h>

int aout_load(char *, uint64_t, struct preloaded_file **);
int aout_exec(struct preloaded_file *);
int ecoff_load(char *, uint64_t, struct preloaded_file **);
int ecoff_exec(struct preloaded_file *);
int elf32_load(char *, uint64_t, struct preloaded_file **);
int elf32_exec(struct preloaded_file *);
int elf64_load(char *, uint64_t, struct preloaded_file **);
int elf64_exec(struct preloaded_file *);
int xcoff32_load(char *, uint64_t, struct preloaded_file **);
int xcoff32_exec(struct preloaded_file *);
int xcoff64_load(char *, uint64_t, struct preloaded_file **);
int xcoff64_exec(struct preloaded_file *);

struct file_format i386_aout = {
		.l_load = aout_load,
		.l_exec = aout_exec,
};

struct file_format i386_ecoff = {
		.l_load = ecoff_load,
		.l_exec = ecoff_exec,
};

struct file_format i386_elf32 = {
		.l_load = elf32_load,
		.l_exec = elf32_exec,
};

struct file_format i386_elf64 = {
		.l_load = elf64_load,
		.l_exec = elf64_exec
};

struct file_format i386_xcoff32 = {
		.l_load = xcoff32_load,
		.l_exec = xcoff32_exec
};

struct file_format i386_xcoff64 = {
		.l_load = xcoff64_load,
		.l_exec = xcoff64_exec
};

int
aout_load(char *filename, uint64_t dest, struct preloaded_file **fp)
{
	return (boot_loadfile(filename, AOUT_KERNELTYPE, dest, fp));
}

int
aout_exec(struct preloaded_file *fp)
{
	return (boot_exec(fp, AOUT_KERNELTYPE));
}

int
ecoff_load(char *filename, uint64_t dest, struct preloaded_file **fp)
{
	return (boot_loadfile(filename, ECOFF_KERNELTYPE, dest, fp));
}

int
ecoff_exec(struct preloaded_file *fp)
{
	return (boot_exec(fp, ECOFF_KERNELTYPE));
}

int
elf32_load(char *filename, uint64_t dest, struct preloaded_file **fp)
{
	return (boot_loadfile(filename, ELF32_KERNELTYPE, dest, fp));
}

int
elf32_exec(struct preloaded_file *fp)
{
	return (boot_exec(fp, ELF32_KERNELTYPE));
}

int
elf64_load(char *filename, uint64_t dest, struct preloaded_file **fp)
{
	return (boot_loadfile(filename, ELF64_KERNELTYPE, dest, fp));
}

int
elf64_exec(struct preloaded_file *fp)
{
	return (boot_exec(fp, ELF64_KERNELTYPE));
}

int
xcoff32_load(char *filename, uint64_t dest, struct preloaded_file **fp)
{
	return (boot_loadfile(filename, XCOFF32_KERNELTYPE, dest, fp));
}

int
xcoff32_exec(struct preloaded_file *fp)
{
	return (boot_exec(fp, XCOFF32_KERNELTYPE));
}

int
xcoff64_load(char *filename, uint64_t dest, struct preloaded_file **fp)
{
	return (boot_loadfile(filename, XCOFF64_KERNELTYPE, dest, fp));
}

int
xcoff64_exec(struct preloaded_file *fp)
{
	return (boot_exec(fp, XCOFF64_KERNELTYPE));
}
