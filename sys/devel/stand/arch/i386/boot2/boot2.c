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
#include <sys/exec_aout.h>
#include <sys/exec_elf.h>
#include <sys/errno.h>

#include <machine/bootinfo.h>
#include <machine/psl.h>
#include <machine/aout_machdep.h>
#include <machine/elf_machdep.h>

#include <stdarg.h>

#include <btxv86.h>

#include <boot2.h>
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

#define DRV_FLOPPY		0x00
#define DRV_HARD		0x80
#define DRV_MASK		0x7f

#define TYPE_AD			0	 	/* scsi (DTYPE_SCSI) */
#define TYPE_DA			1		/* ide (DTYPE_ST506 | DTYPE_ESDI) */
#define TYPE_MAXHARD	TYPE_DA
#define TYPE_FD			2		/* floppy (DTYPE_FLOPPY | DTYPE_ATAPI) */

#define INVALID_S		"Invalid %s\n"

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

static const char *const dev_nm[NDEV] = { "ad", "da", "fd" };
static const unsigned char dev_maj[NDEV] = { 30, 4, 2 };

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
struct boot2_dmadat *boot2_dmadat;
off_t fs_off;
uint8_t ls;

void exit(int);
static void load_aout(struct exec *, uint32_t *, ino_t, const char *, int, struct bootinfo *);
static void load_elf(Elf32_Ehdr *, uint32_t *, ino_t, const char *, int, struct bootinfo *);
static void load(void);
static int dev2bios(const char *, int, int *);
static int bios2dev(int *, int, int);
static int parse(void);
static void boot2_banner(void);
static uint32_t dskmakebootdev(void);
static int xfsread_path(const char *, void *, size_t, off_t);
static int xfsread(ino_t, const char *, void *, size_t, off_t);
int dskread(void *, unsigned, unsigned); /* UNUSED */
static void printf(const char *, ...);
static void putchar(int);
static uint32_t memsize(void);
static int drvread(void *, unsigned, unsigned);
static int keyhit(unsigned);
static int xputc(int);
static int xgetc(int);
static int getc(int);

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
xfsread_path(const char *path, void *buf, size_t nbyte, off_t offset)
{
	int error;

	error = fsread(path, buf, nbyte, offset);
	if (error < 0) {
		printf(INVALID_S, "format");
		return (error);
	}
	return (0);
}

static int
xfsread(ino_t ino, const char *path, void *buf, size_t nbyte, off_t offset)
{
	if (ino) {
		return (xfsread_path(path, buf, nbyte, offset));
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

static void
boot2_banner(void)
{
	printf("\n211BSD boot\n"
			"Default: %u:%s(%u,%c)%s\n"
			"boot: ", dsk.drive & DRV_MASK, dev_nm[dsk.type], dsk.unit,
			'a' + dsk.part, kname);
}

static uint32_t
dskmakebootdev(void)
{
	int error, major;

	error = bios2dev(&major, dsk.type, dsk.unit);
	if ((error != 0) && (major < 0)) {
		major = dev_maj[dsk.type];
	}
#ifdef DISK_SLICES
	return (MAKEBOOTDEV2(major, dsk.slice, dsk.unit, dsk.part));
#else /* !DISK_SLICES */
	return (MAKEBOOTDEV1(major, B_SLICE_TO_B_ADAPTOR(dsk.slice),
			B_SLICE_TO_B_CONTROLLER(dsk.slice), dsk.unit, dsk.part));
#endif /* !DISK_SLICES */
}

int
main(void)
{
	int autoboot;
	ino_t ino;

	kname = NULL;
	boot2_dmadat = (void *)(roundup2(__base + (int32_t) & _end, 0x10000)
			- __base);
	v86.ctl = V86_FLAGS;
	v86.efl = PSL_RESERVED_DEFAULT | PSL_I;
	dsk.drive = *(uint8_t *)PTOV(ARGS);
	dsk.type = dsk.drive & DRV_HARD ? TYPE_AD : TYPE_FD;
	dsk.unit = dsk.drive & DRV_MASK;
	dsk.slice = *(uint8_t *)PTOV(ARGS + 1) + 1;
	bootinfo.bi_version = BOOTINFO_VERSION;
	bootinfo.bi_size = sizeof(bootinfo);
	bootinfo.bi_basemem = 0; /* XXX will be filled by loader or kernel */
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
		if (!autoboot || !OPT_CHECK(RBX_QUIET))
			boot2_banner();
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

/* XXX - Needed for btxld to link the boot2 binary; do not remove. */
void
exit(int x)
{

}

static void
load_aout(struct exec *ex, uint32_t *addr, ino_t ino, const char *name, int fmt, struct bootinfo *boot)
{
	caddr_t p;
	uint32_t x;

	if (fmt == 0) {
		*addr = ex->a_entry & 0xffffff;
		p = PTOV(*addr);
		fs_off = PAGE_SIZE;
		if (xfsread(ino, name, p, ex->a_text, fs_off))
			return;
		p += roundup2(ex->a_text, PAGE_SIZE);
		if (xfsread(ino, name, p, ex->a_data, fs_off))
			return;
		p += ex->a_data + roundup2(ex->a_bss, PAGE_SIZE);
		boot->bi_symtab = VTOP(p);
		memcpy(p, ex->a_syms, sizeof(ex->a_syms));
		p += sizeof(ex->a_syms);
		if (ex->a_syms) {
			if (xfsread(ino, name, p, ex->a_syms, fs_off))
				return;
			p += ex->a_syms;
			if (xfsread(ino, p, sizeof(int), fs_off))
				return;
			x = *(uint32_t *)p;
			p += sizeof(int);
			x -= sizeof(int);
			if (xfsread(ino, name, p, x, fs_off))
				return;
			p += x;
		}
	} else {
		return;
	}
	boot->bi_esymtab = VTOP(p);
	boot->bi_kernelname = VTOP(name);
	boot->bi_bios_dev = dsk.drive;
}

static void
load_elf(Elf32_Ehdr *eh, uint32_t *addr, ino_t ino, const char *name, int fmt, struct bootinfo *boot)
{
	Elf32_Phdr ep[2];
	Elf32_Shdr es[2];
	caddr_t p;
	int i, j;

	if (fmt == 1) {
		fs_off = eh->e_phoff;
		for (j = i = 0; i < eh->e_phnum && j < 2; i++) {
			if (xfsread(ino, name, ep + j, sizeof(ep[0]), fs_off))
				return;
			if (ep[j].p_type == PT_LOAD)
				j++;
		}
		for (i = 0; i < 2; i++) {
			p = PTOV(ep[i].p_paddr & 0xffffff);
			fs_off = ep[i].p_offset;
			if (xfsread(ino, name, p, ep[i].p_filesz, fs_off))
				return;
		}
		p += roundup2(ep[1].p_memsz, PAGE_SIZE);
		boot->bi_symtab = VTOP(p);
		if (eh->e_shnum == eh->e_shstrndx + 3) {
			fs_off = eh->e_shoff + sizeof(es[0]) * (eh->e_shstrndx + 1);
			if (xfsread(ino, name, &es, sizeof(es), fs_off))
				return;
			for (i = 0; i < 2; i++) {
				memcpy(p, &es[i].sh_size, sizeof(es[i].sh_size));
				p += sizeof(es[i].sh_size);
				fs_off = es[i].sh_offset;
				if (xfsread(ino, name, p, es[i].sh_size, fs_off))
					return;
				p += es[i].sh_size;
			}
		}
		*addr = eh->e_entry & 0xffffff;
	} else {
		return;
	}
	boot->bi_esymtab = VTOP(p);
	boot->bi_kernelname = VTOP(name);
	boot->bi_bios_dev = dsk.drive;
}

static void
load(void)
{
	union {
		struct exec ex;
		Elf32_Ehdr eh;
	} hdr;
	ino_t ino;
	uint32_t addr;
	int fmt;

	if (!(ino = lookup(kname))) {
		if (!ls) {
			printf("No %s\n", kname);
		}
		return;
	}
	if (xfsread(ino, kname, &hdr, sizeof(hdr), 0)) {
		return;
	}
	if (N_GETMAGIC(hdr.ex) == ZMAGIC) {
		fmt = 0;
	} else if (IS_ELF(hdr.eh)) {
		fmt = 1;
	} else {
		printf("Invalid %s\n", "format");
		return;
	}
	load_aout(&hdr.ex, &addr, ino, kname, fmt, &bootinfo);
	load_elf(&hdr.eh, &addr, ino, kname, fmt, &bootinfo);
	__exec((caddr_t)addr, RB_BOOTINFO | (opts & RBX_MASK), dskmakebootdev(), 0,
			0, 0, VTOP(&bootinfo));
}

static int
dev2bios(const char *devname, int unit, int *biosdev)
{
    if ((strcmp(devname, dev_nm[0]) == 0) || (strcmp(devname, dev_nm[1]) == 0)) {
        *biosdev = (DRV_HARD + unit);
    } else if (strcmp(devname, dev_nm[2]) == 0) {
        *biosdev = (DRV_FLOPPY + unit);
    } else {
        return (ENXIO);
    }
    return (0);
}

static int
bios2dev(int *major, int type, int unit)
{
	const char *devname;
	int error, biosdev;

    devname = dev_nm[type];
    *major = -1;
    error = dev2bios(devname, unit, &biosdev);
    if (error != 0) {
    	printf("Invalid %s\n", devname);
    	return (error);
    }
    *major = biosdev;
    return (0);
}

static int
parse(void)
{
	char *arg = cmd;
	char *p, *q;
	unsigned int drv;
	int c, i;

	while ((c = *arg++)) {
		if (c == ' ' || c == '\t' || c == '\n')
			continue;
		for (p = arg; *p && *p != '\n' && *p != ' ' && *p != '\t'; p++)
			;
		if (*p)
			*p++ = 0;
		if (c == '-') {
			while ((c = *arg++)) {
				for (i = 0; c != optstr[i]; i++)
					if (i == NOPT - 1)
						return (-1);
				opts ^= 1 << flags[i];
			}
			if (OPT_CHECK(RBX_PROBEKBD)) {
				i = *(uint8_t *)PTOV(0x496) & 0x10;
				printf("Keyboard: %s\n", i ? "yes" : "no");
				if (!i)
					opts |= OPT_SET(RBX_DUAL) | OPT_SET(RBX_SERIAL);
				opts &= ~OPT_SET(RBX_PROBEKBD);
			}
			ioctrl = OPT_CHECK(RBX_DUAL) ? (IO_SERIAL | IO_KEYBOARD) :
						OPT_CHECK(RBX_SERIAL) ? IO_SERIAL : IO_KEYBOARD;
			if (ioctrl & IO_SERIAL)
				sio_init();
		} else {
			for (q = arg--; *q && *q != '('; q++)
				;
			if (*q) {
				drv = -1;
				if (arg[1] == ':') {
					drv = *arg - '0';
					if (drv > 9)
						return (-1);
					arg += 2;
				}
				if (q - arg != 2)
					return (-1);
				for (i = 0; arg[0] != dev_nm[i][0] || arg[1] != dev_nm[i][1];
						i++)
					if (i == NDEV - 1)
						return (-1);
				dsk.type = i;
				arg += 3;
				dsk.unit = *arg - '0';
				if (arg[1] != ',' || dsk.unit > 9)
					return (-1);
				arg += 2;
				dsk.slice = WHOLE_DISK_SLICE;
				if (arg[1] == ',') {
					dsk.slice = *arg - '0' + 1;
					if (dsk.slice > NDOSPART)
						return (-1);
					arg += 2;
				}
				if (arg[1] != ')')
					return (-1);
				dsk.part = *arg - 'a';
				if (dsk.part > 7)
					return (-1);
				arg += 2;
				if (drv == -1)
					drv = dsk.unit;
				dsk.drive = (dsk.type <= TYPE_MAXHARD ? DRV_HARD : 0) + drv;

			}
			if ((i = p - arg - !*(p - 1))) {
				if ((size_t) i >= sizeof(kname))
					return (-1);
				memcpy(kname, arg, i + 1);
			}
		}
		arg = p;
	}
	return (dskprobe());
}

static int
dskprobe(void)
{
	struct dos_partition *dp;
	struct disklabel *d;
	char *sec;
	unsigned i;
	uint8_t sl;

	/*
	 * Probe slice table
	 */
	sec = boot2_dmadat->secbuf;
	dsk.start = 0;
	if (drvread(sec, DOSBBSECTOR, 1))
		return (-1);
	dp = (void *)(sec + DOSPARTOFF);
	sl = dsk.slice;
	if (sl < BASE_SLICE) {
		for (i = 0; i < NDOSPART; i++)
			if ((dp[i].dp_typ == DOSPTYP_386BSD
					|| dp[i].dp_typ == DOSPTYP_DFLYBSD)
					&& ((dp[i].dp_flag & 0x80) || sl < BASE_SLICE)) {
				sl = BASE_SLICE + i;
				if ((dp[i].dp_flag & 0x80) || dsk.slice == COMPATIBILITY_SLICE)
					break;
			}
		if (dsk.slice == WHOLE_DISK_SLICE)
			dsk.slice = sl;
	}
	if (sl != WHOLE_DISK_SLICE) {
		if (sl != COMPATIBILITY_SLICE)
			dp += sl - BASE_SLICE;
		if (dp->dp_typ != DOSPTYP_386BSD && dp->dp_typ != DOSPTYP_DFLYBSD) {
			printf(INVALID_S, "slice");
			return (-1);
		}
		dsk.start = dp->dp_start;
	}

	/*
	 * Probe label and partition table
	 */
	if (drvread(sec, dsk.start + LABELSECTOR, 1))
		return (-1);
	d = (void *)(sec + LABELOFFSET);
	if (d->d_magic != DISKMAGIC || d->d_magic2 != DISKMAGIC) {
		if (dsk.part != RAW_PART) {
			printf(INVALID_S, "label");
			return (-1);
		}
	} else {
		if (!dsk.init) {
			if (d->d_type == DTYPE_SCSI)
				dsk.type = TYPE_DA;
			dsk.init++;
		}
		if (dsk.part >= d->d_npartitions || !d->d_partitions[dsk.part].p_size) {
			printf(INVALID_S, "partition");
			return (-1);
		}
		dsk.start += d->d_partitions[dsk.part].p_offset;
		dsk.start -= d->d_partitions[RAW_PART].p_offset;
	}
	return (0);
}

/* UNUSED */
int
dskread(void *buf, unsigned lba, unsigned nblk)
{
	return (drvread(buf, dsk.start + lba, nblk));
}

static void
printf(const char *fmt,...)
{
	va_list ap;
	char buf[10];
	char *s;
	unsigned u;
	int c;

	va_start(ap, fmt);
	while ((c = *fmt++)) {
		if (c == '%') {
			c = *fmt++;
			switch (c) {
			case 'c':
				putchar(va_arg(ap, int));
				continue;
			case 's':
				for (s = va_arg(ap, char*); *s; s++)
					putchar(*s);
				continue;
			case 'u':
				u = va_arg(ap, unsigned);
				s = buf;
				do
					*s++ = '0' + u % 10U;
				while (u /= 10U);
				while (--s >= buf)
					putchar(*s);
				continue;
			}
		}
		putchar(c);
	}
	va_end(ap);
	return;
}

static void
putchar(int c)
{
	if (c == '\n')
		xputc('\r');
	xputc(c);
}

static int
drvread(void *buf, unsigned lba, unsigned nblk)
{
	static unsigned c = 0x2d5c7c2f;

	printf("%c\b", c = c << 8 | c >> 24);
	v86.ctl = V86_ADDR | V86_CALLF | V86_FLAGS;
	v86.addr = XREADORG; /* call to xread in boot1 */
	v86.es = VTOPSEG(buf);
	v86.eax = lba;
	v86.ebx = VTOPOFF(buf);
	v86.ecx = lba >> 16;
	v86.edx = nblk << 8 | dsk.drive;
	v86int();
	v86.ctl = V86_FLAGS;
	if (V86_CY(v86.efl)) {
		printf("error %u lba %u\n", v86.eax >> 8 & 0xff, lba);
		return (-1);
	}
	return (0);
}

static int
keyhit(unsigned ticks)
{
	uint32_t t0, t1;

	if (OPT_CHECK(RBX_NOINTR))
		return (0);
	t0 = 0;
	for (;;) {
		if (xgetc(1))
			return (1);
		t1 = *(uint32_t *)PTOV(0x46c);
		if (!t0)
			t0 = t1;
		if (t1 < t0 || t1 >= t0 + ticks)
			return (0);
	}
}

static int
xputc(int c)
{
	if (ioctrl & IO_KEYBOARD)
		putc(c);
	if (ioctrl & IO_SERIAL)
		sio_putc(c);
	return (c);
}

static int
getc(int fn)
{
	v86.addr = 0x16;
	v86.eax = fn << 8;
	v86int();
	return (fn == 0 ? v86.eax & 0xff : !V86_ZR(v86.efl));
}

static int
xgetc(int fn)
 {
	if (OPT_CHECK(RBX_NOINTR))
		return (0);
	for (;;) {
		if ((ioctrl & IO_KEYBOARD) && getc(1))
			return (fn ? 1 : getc(0));
		if ((ioctrl & IO_SERIAL) && sio_ischar())
			return (fn ? 1 : sio_getc());
		if (fn)
			return (0);
	}
}
