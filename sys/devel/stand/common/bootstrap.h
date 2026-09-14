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

#ifndef _BOOTSTRAP_H_
#define _BOOTSTRAP_H_

#include <sys/types.h>
#include <sys/queue.h>

struct preloaded_file;

/*
 * Preloaded file information. Depending on type, file can contain
 * additional units called 'modules'.
 *
 * At least one file (the kernel) must be loaded in order to boot.
 * The kernel is always loaded first.
 *
 * String fields (m_name, m_type) should be dynamically allocated.
 */
struct preloaded_file {

    char					*f_name;			/* file name */
    char					*f_type;			/* verbose file type, eg 'ELF kernel', 'pnptable', etc. */
    char					*f_args;			/* arguments for the file */
    int						f_loader;			/* index of the loader that read the file */
    vm_offset_t				f_addr;				/* load address */
    size_t					f_size;				/* file size */
    struct preloaded_file	*f_next;			/* next file */
    u_long                  f_marks[MARK_MAX];	/* filled by loadfile() */

#ifdef notyet
    /* ELF Symbols */
	uint32_t				f_flags;
	uint32_t 				f_mem_lower;
	uint32_t 				f_mem_upper;
	uint32_t				f_elfshdr_num;
	uint32_t				f_elfshdr_size;
	caddr_t					f_elfshdr_addr;
	uint32_t				f_elfshdr_shndx;
#endif
};

struct file_format {
    /* Load function must return EFTYPE if it can't handle the module supplied */
    int		(* l_load)(char *, uint64_t, struct preloaded_file **);
    /* Only a loader that will load a kernel (first module) should have an exec handler */
    int		(* l_exec)(struct preloaded_file *);
};

/*
 * The intention of the architecture switch is to provide a convenient
 * encapsulation of the interface between the bootstrap MI and MD code.
 * MD code may selectively populate the switch at runtime based on the
 * actual configuration of the target system.
 */
struct arch_switch {
	/* Automatically load modules as required by detected hardware */
	int (*arch_autoload)(void);

	/* Locate the device for (name), return pointer to tail in (*path) */
	int (*arch_getdev)(void **, const char *, const char **);

	/* Copy from local address space to module address space, similar to bcopy() */
	ssize_t (*arch_copyin)(const void *, vm_offset_t, const size_t);

	/* Copy to local address space from module address space, similar to bcopy() */
	ssize_t (*arch_copyout)(const vm_offset_t, void *, const size_t);

	/* Read from file to module address space, same semantics as read() */
	ssize_t (*arch_readin)(const int, vm_offset_t, const size_t);

	/* Perform ISA byte port I/O (only for systems with ISA) */
	int (*arch_isainb)(int);
	void (*arch_isaoutb)(int, int);
};

/* kerneltype names */
#define AOUT_KERNELTYPE 	"aout kernel"
#define ECOFF_KERNELTYPE 	"ecoff kernel"
#define ELF32_KERNELTYPE 	"elf32 kernel"
#define ELF64_KERNELTYPE 	"elf64 kernel"
#define XCOFF32_KERNELTYPE 	"xcoff32 kernel"
#define XCOFF64_KERNELTYPE 	"xcoff64 kernel"

extern struct file_format *file_formats[]; /* supplied by consumer */
extern struct preloaded_file *preloaded_files;
extern struct arch_switch archsw;

/* boot.c */
int autoboot(int, char *);
void autoboot_maybe(void);
int getrootmount(char*);

/* disk.c */
void disk_setbootdev(struct devdesc *, uint32_t);
int disk_makebootdev(struct devdesc *, int);
int disk_device_type(uint32_t);
int disk_device_adaptor(uint32_t);
int disk_device_controller(uint32_t);
int disk_device_slice(uint32_t);
int disk_device_partition(uint32_t);
void disk_format(uint32_t, int, int, int, int, int);
int disk_getdev(struct devdesc **, const char *, const char **);
char* disk_fmtdev(struct devdesc *);
int disk_parsedev(struct devdesc **, const char *, const char **);
int disk_setcurrdev(struct env_var *, int, void *);

/* fileload.c */
struct preloaded_file* file_alloc(void);
struct preloaded_file* file_findfile(char *, char *);
int file_loadkernel(char *, int, char **);
void file_discard(struct preloaded_file*);

/* load_exec.c */
int exec_loadfile(char *, char *, u_int64_t, int, struct preloaded_file **);

/* load_ksyms.c */
void ksyms_addr_set(void *, void *, void *);

/* metadata.c */
int md_getboothowto(char *);
void md_setboothowto(int);
int md_load(int, vm_offset_t, vm_offset_t, caddr_t, caddr_t, caddr_t, struct preloaded_file *, char *);
vm_offset_t md_copyenv(vm_offset_t);
vm_offset_t md_align(vm_offset_t);

/* misc.c */
char* unargv(int, char **);
void hexdump(caddr_t, size_t);
size_t strlenout(vm_offset_t);
char* strdupout(vm_offset_t);
void kern_bzero(vm_offset_t, size_t);
int kern_pread(int, vm_offset_t, size_t, off_t);
void* alloc_pread(int, off_t, size_t);
/* This must be provided by the MD code, but should it be in the archsw? */
void delay(int);
void dev_cleanup(void);
time_t time(time_t *);

#endif /* _BOOTSTRAP_H_ */
