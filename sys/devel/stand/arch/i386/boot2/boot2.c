/*
 * Copyright (c) 1998 Robert Nordier
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are freely
 * permitted provided that the above copyright notice and this
 * paragraph and the following disclaimer are duplicated in all
 * such forms.
 *
 * This software is provided "AS IS" and without any express or
 * implied warranties, including, without limitation, the implied
 * warranties of merchantability and fitness for a particular
 * purpose.
 */

/*
 * $FreeBSD$
 */

#include <sys/param.h>
#include <sys/reboot.h>

#include <sys/disklabel.h>
#include <sys/diskmbr.h>
#include <sys/dirent.h>

#include <machine/bootinfo.h>
#include <machine/psl.h>
#include <machine/elf_machdep.h>

#include <stdarg.h>

#include <a.out.h>

#include <btxv86.h>

#include "boot2.h"
#include "bootpaths.h"
#include "lib.h"

#include "rbx.h"

#define IO_KEYBOARD		1
#define IO_SERIAL		2

#define SECOND			18		/* Circa that many ticks in a second. */

#define ARGS			0x900
#define NOPT			12
#define NDEV			3
#define MEM_BASE		0x12
#define MEM_EXT 		0x15
#define V86_CY(x)		((x) & PSL_C)
#define V86_ZR(x)		((x) & PSL_Z)

#define DRV_HARD		0x80
#define DRV_MASK		0x7f

#define TYPE_AD			0
#define TYPE_DA			1
#define TYPE_MAXHARD	TYPE_DA
#define TYPE_FD			2

extern uint32_t _end;

static const char optstr[NOPT] = "DhaCgmnPprsv";
static const unsigned char flags[NOPT] = {
		RBX_DUAL,
		RBX_SERIAL,
		RBX_ASKNAME,
		RBX_CDROM,
		RBX_GDB,
		RBX_MUTE,
		RBX_NOINTR,
		RBX_PROBEKBD,
		RBX_PAUSE,
		RBX_DFLTROOT,
		RBX_SINGLE,
		RBX_VERBOSE
};

static struct dsk {
    unsigned drive;
    unsigned type;
    unsigned unit;
    unsigned slice;
    unsigned part;
    unsigned start;
    int 	 init;
} dsk;

static char cmd[512];
static char kname[1024];
static uint32_t opts = 0;
static struct bootinfo bootinfo;
static uint8_t ioctrl = IO_KEYBOARD;
off_t fs_off;
int ls;

static int xfsread_path(const char *, void *, size_t);
static int xfsread(ino_t, void *, size_t);

void
memcpy(void *d, const void *s, int len)
{
    char *dd = d;
    const char *ss = s;

	while (--len >= 0)
	    dd[len] = ss[len];
}

int
strcmp(const char *s1, const char *s2)
{
	for (; *s1 == *s2 && *s1; s1++, s2++)
		;
	return ((int)((unsigned char)*s1 - (unsigned char)*s2));
}

static int
xfsread_path(const char *path, void *buf, size_t nbyte)
{
	int error;

	error = fsread(path, buf, nbyte, fs_off);
	if (error < 0) {
		printf("Invalid %s\n", "format");
		return (error);
	}
	return (0);
}

static int
xfsread(ino_t ino, void *buf, size_t nbyte)
{
	if (ino) {
		return (xfsread_path(kname, buf, nbyte));
	}
	return (-1);
}

static inline uint32_t
memsize(void)
{
    v86.addr = MEM_EXT;
    v86.eax = 0x8800;
    v86int();
    return (v86.eax);
}

static inline void
getstr(void)
{
	char *s;
	int c;

	s = cmd;
	for (;;) {
		switch (c = xgetc(0)) {
		case 0:
			break;
		case '\177':
		case '\b':
			if (s > cmd) {
				s--;
				printf("\b \b");
			}
			break;
		case '\n':
		case '\r':
			*s = 0;
			return;
		default:
			if (s - cmd < sizeof(cmd) - 1)
				*s++ = c;
			putchar(c);
		}
	}
}

static inline void
putc(int c)
{
    v86.addr = 0x10;
    v86.eax = 0xe00 | (c & 0xff);
    v86.ebx = 0x7;
    v86int();
}

int
main(void)
{
	int autoboot;
	ino_t ino;

	kname = NULL;
	v86.ctl = V86_FLAGS;
	v86.efl = PSL_RESERVED_DEFAULT | PSL_I;
    dsk.drive = *(uint8_t *)PTOV(ARGS);
    dsk.type = dsk.drive & DRV_HARD ? TYPE_AD : TYPE_FD;
    dsk.unit = dsk.drive & DRV_MASK;
    dsk.slice = *(uint8_t *)PTOV(ARGS + 1) + 1;
    bootinfo.bi_version = BOOTINFO_VERSION;
    bootinfo.bi_size = sizeof(bootinfo);
    bootinfo.bi_basemem = 0;	/* XXX will be filled by loader or kernel */
    bootinfo.bi_extmem = memsize();
    bootinfo.bi_memsizes_valid++;

	/* Process configuration file */

	autoboot = 1;
	if ((ino = lookup(PATH_CONFIG)) || (ino = lookup(PATH_DOTCONFIG))) {
		if (ino) {
			fsread(PATH_CONFIG, cmd, sizeof(cmd), 0);
		}
	}

	if (*cmd) {
		printf("%s: %s", PATH_CONFIG, cmd);
		if (parse()) {
			autoboot = 0;
		}
		/* Do not process this command twice */
		*cmd = 0;
	}
	/*
	 * Try to exec stage 3 boot loader. If interrupted by a keypress,
	 * or in case of failure, try to load a kernel directly instead.
	 */

	if (autoboot && !*kname) {
		memcpy(kname, PATH_LOADER, sizeof(PATH_LOADER));
		if (!keyhit(3 * SECOND)) {
			load();
			memcpy(kname, PATH_KERNEL, sizeof(PATH_KERNEL));
		}
	}

	/* Present the user with the boot2 prompt. */

	for (;;) {
		printf("\n211BSD boot\n"
				"Default: %u:%s(%u,%c)%s\n"
				"boot: ", dsk.drive & DRV_MASK, dev_nm[dsk.type], dsk.unit,
				'a' + dsk.part, kname);
		if (ioctrl & IO_SERIAL) {
			sio_flush();
		}
		if (!autoboot || keyhit(5 * SECOND)) {
			getstr();
		} else {
			putchar('\n');
		}
		autoboot = 0;
		if (parse()) {
			putchar('\a');
		} else {
			load();
		}
	}
}



static void
load(void)
{
	caddr_t p;
	ino_t ino;

	if (!(ino = lookup(kname))) {
		if (!ls) {
			printf("No %s\n", kname);
		}
		return;
	}
	if (xfsread(ino, &hdr, sizeof(hdr))) {
		return;
	}
}
