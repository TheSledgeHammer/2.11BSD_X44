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

#ifndef _LIBI386_H_
#define _LIBI386_H_

struct i386_devdesc {
	union {
		struct devdesc	dd;				/* Must be first. */
	} d_kind;
};

extern struct devdesc currdev;	/* our current device */
#define MAXDEV			31		/* maximum number of distinct devices */
#define MAXBDDEV		MAXDEV

/* format support */
extern struct file_format i386_aout;
extern struct file_format i386_ecoff;
extern struct file_format i386_elf32;
extern struct file_format i386_elf64;
extern struct file_format i386_xcoff32;
extern struct file_format i386_xcoff64;

extern uint32_t		bios_basemem;	/* base memory in bytes */
extern uint32_t		bios_extmem;	/* extended memory in bytes */
extern vm_offset_t	memtop;			/* last address of physical memory + 1 */
extern vm_offset_t	memtop_copyin;	/* memtop less heap size for the cases */
									/* when heap is at the top of         */
									/* extended memory; for other cases   */
									/* just the same as memtop            */
extern uint32_t		high_heap_size;	/* extended memory region available */
extern vm_offset_t	high_heap_base;	/* for use as the heap */

/* bio.c */
/* 16KB buffer space for real mode data transfers. */
#define	BIO_BUFFER_SIZE 0x4000
void *bio_alloc(size_t);
void bio_free(void*, size_t);

/* bioscd.c */

/* biosdisk.c */

/* biosfd.c */

/* biosmem.c */
void bios_getmem(void);
int command_biosmem(int, char **);

/* biospci.c */

/* biospnp.c */

/* biossmap.c */
void bios_getsmap(void);
int command_smap(int, char **);

/* bootinfo.c */
int bi_load(struct bootinfo *, struct preloaded_file *, char *, char *);

/* bootload.c */
int boot_loadfile(char *, char *, uint64_t, struct preloaded_file **);
int boot_exec(struct preloaded_file *, char *);

/* devicename.c */
int i386_getdev(void **, const char *, const char *);
char *i386_fmtdev(void *);

/* i386_autoload.c */
int i386_autoload(void);

/* i386_copy.c */
ssize_t i386_copyin(const void *, vm_offset_t, const size_t);
ssize_t i386_copyout(const vm_offset_t, void *, const size_t);
ssize_t i386_readin(const int, vm_offset_t, const size_t);

/* i386_reboot.c */
int command_reboot(int, char **);

/* pread.c */
int pread(int, vm_offset_t, size_t);

/* time.c */
time_t time(time_t *);

/* i386 commands */
#define I386_COMMANDS \
	{ "reboot", "reboot the system", command_reboot }, \
	{ "biosmem", "show BIOS memory setup", command_biosmem }, \
	{  "smap", "show BIOS SMAP", command_smap },

#endif /* _LIBI386_H_ */
