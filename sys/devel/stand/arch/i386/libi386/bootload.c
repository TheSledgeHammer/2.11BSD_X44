/*-
 * Copyright (c) 1998 Michael Smith <msmith@freebsd.org>
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
 *
 * $FreeBSD$
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/reboot.h>
#include <sys/exec.h>
#include <sys/boot.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/loadfile.h>

#include <bootstrap.h>
#include <libi386.h>
#include <btxv86.h>

#include <machine/bootinfo.h>
#include <machine/cpufunc.h>
#include <machine/psl.h>
#include <machine/specialreg.h>

static int bi_checkcpu(void);
static void bi_load_legacy(struct bootinfo, char *);

/*
 * Check to see if this CPU supports long mode.
 */
static int
bi_checkcpu(void)
{
	char *cpu_vendor;
	int vendor[3];
	int eflags;
	unsigned int regs[4];

	/* Check for presence of "cpuid". */
	eflags = read_eflags();
	write_eflags(eflags ^ PSL_ID);
	if (!((eflags ^ read_eflags()) & PSL_ID))
		return (0);

	/* Fetch the vendor string. */
	do_cpuid(0, regs);
	vendor[0] = regs[1];
	vendor[1] = regs[3];
	vendor[2] = regs[2];
	cpu_vendor = (char *)vendor;

	/* Check for vendors that support AMD features. */
	if (strncmp(cpu_vendor, INTEL_VENDOR_ID, 12) != 0
			&& strncmp(cpu_vendor, AMD_VENDOR_ID, 12) != 0
			&& strncmp(cpu_vendor, HYGON_VENDOR_ID, 12) != 0
			&& strncmp(cpu_vendor, CENTAUR_VENDOR_ID, 12) != 0)
		return (0);

	/* Has to support AMD features. */
	do_cpuid(0x80000000, regs);
	if (!(regs[0] >= 0x80000001))
		return (0);

	/* Check for long mode. */
	do_cpuid(0x80000001, regs);

	return (regs[3] & AMDID_LM);
}

int
bi_load(struct bootinfo *bi, struct preloaded_file *fp, char *kerntype, char *args)
{
	int error;

	/* Check long mode support */
	if (!bi_checkcpu()) {
		printf("CPU doesn't support long mode\n");
		return (EINVAL);
	}

	/*
	 * Version 1 bootinfo.
	 */
	bi->bi_version = 1;

	fp = file_findfile(NULL, kerntype);
	if (fp == NULL) {
		return (EINVAL);
	}

	error = md_load(bi->bi_boothowto, bi->bi_kernend, bi->bi_environment,
			bi->bi_nsymtab, bi->bi_symtab, bi->bi_esymtab, fp, args);
	if (error != 0) {
		return (error);
	}
	return (0);
}

/* Needs fixing!! */
static void
bi_load_legacy(struct bootinfo bi, char *args)
{
	int bootdevnr, i, howto;
	const char *kernelpath;
	char *kernelname;

	/* legacy bootinfo structure */
	kernelname = getenv("kernelname");
	i386_getdev(NULL, kernelname, &kernelpath);
	bi.bi_version = BOOTINFO_VERSION;
	bi.bi_kernelname = 0; 						/* XXX char * -> kernel name */
	bi.bi_nfs_diskless = 0; 					/* struct nfs_diskless * */
	bi.bi_n_bios_used = 0; 						/* XXX would have to hook biosdisk driver for these */
	for (i = 0; i < N_BIOS_GEOM; i++) {
		bi.bi_bios_geom[i] = bd_getbigeom(i);
	}
	bi.bi_size = sizeof(bi);
	bi.bi_memsizes_valid = 1;
	bi.bi_basemem = bios_basemem / 1024;
	bi.bi_extmem = bios_extmem / 1024;
	bi.bi_kernelname = VTOP(kernelpath);

	/* legacy boot arguments */
	bi.bi_howtop = howto | RB_BOOTINFO;
	bi.bi_bootdevp = bootdevnr;
	bi.bi_bip = VTOP(&bi);
}
