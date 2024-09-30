/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
static char copyright[] =
"@(#) Copyright (c) 1992, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)gcore.c	8.2 (Berkeley) 9/23/93";
#endif /* not lint */

/*
 * Originally written by Eric Cooper in Fall 1981.
 * Inspired by a version 6 program by Len Levin, 1978.
 * Several pieces of code lifted from Bill Joy's 4BSD ps.
 * Most recently, hacked beyond recognition for 4.4BSD by Steven McCanne,
 * Lawrence Berkeley Laboratory.
 *
 * Portions of this software were developed by the Computer Systems
 * Engineering group at Lawrence Berkeley Laboratory under DARPA
 * contract BG 91-66 and contributed to Berkeley.
 */

#include <sys/param.h>
#include <sys/ptrace.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/sysctl.h>

#include <machine/vmparam.h>

#include <fcntl.h>
#include <kvm.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#ifdef EXEC_ELF32
#include <sys/exec_elf.h>
#define GCORE_ELF32
#endif
#ifdef EXEC_ELF64
#include <sys/exec_elf.h>
#define GCORE_ELF64
#endif
#ifdef EXEC_AOUT
#include <a.out.h>
#define GCORE_AOUT
#endif

static int open_corefile(char *);
static void	core(int, int, struct kinfo_proc *);
static void datadump(int, int, struct proc *, u_long, int);
static void userdump(int, struct proc *, u_long, int);
static void usage(void);
#ifdef GCORE_AOUT
static int read_aout(char **, int);
#endif
#ifdef GCORE_ELF32
static int read_elf32(char **, int);
#endif
#ifdef GCORE_ELF64
static int read_elf64(char **, int);
#endif
static int read_exec(char **, int);
static int elf_datoff(int, off_t, off_t, off_t, off_t);
static char *saveRead(int, off_t, off_t, char *);

kvm_t *kd;
static int data_offset;

int
main(argc, argv)
	int argc;
	char *argv[];
{
	struct proc *p;
	struct kinfo_proc *ki;
	char *corefile;
	char errbuf[_POSIX2_LINE_MAX];
	int ch, cnt, efd, fd, pid, sflag, uid;

	corefile = NULL;
	while ((ch = getopt(argc, argv, "c:s")) != EOF) {
		switch (ch) {
		case 'c':
			corefile = optarg;
			break;
		case 's':
			sflag = 1;
			break;
		default:
			usage();
			break;
		}
	}
	argv += optind;
	argc -= optind;

	if (argc != 2) {
		usage();
	}

	kd = kvm_openfiles(0, 0, 0, O_RDONLY, errbuf);
	if (kd == NULL) {
		err(1, "%s", errbuf);
	}

	uid = getuid();
	pid = atoi(argv[1]);

	ki = kvm_getprocs(kd, KERN_PROC_PID, pid, &cnt);
	if (ki == NULL || cnt != 1) {
		err(1, "%d: not found", pid);
	}

	p = &ki->kp_proc;
	if (ki->kp_eproc.e_pcred.p_ruid != uid && uid != 0) {
		err(1, "%d: not owner", pid);
	}
	if (p->p_stat == P_SLOAD) {
		err(1, "%d: swapped out", pid);
	}
	if (p->p_stat == SZOMB) {
		err(1, "%d: zombie", pid);
	}
	if (p->p_flag & P_WEXIT) {
		err(0, "process exiting");
	}
	if (p->p_flag & (P_SYSTEM | P_SSYS)) {	/* Swapper or pagedaemon. */
		err(1, "%d: system process");
	}

	fd = open_corefile(corefile);

	efd = open(argv[0], O_RDONLY, 0);
	if (efd < 0) {
		err(1, "%s: %s\n", argv[0], strerror(errno));
	}

	cnt = read_exec(argv, efd);

	if (sflag && kill(pid, SIGSTOP) < 0) {
		err(0, "%d: stop signal: %s", pid, strerror(errno));
	}

	core(efd, fd, ki);

	if (sflag && kill(pid, SIGCONT) < 0) {
		err(0, "%d: continue signal: %s", pid, strerror(errno));
	}
	(void)close(fd);

	exit(0);
}

static int
open_corefile(corefile)
	char *corefile;
{
	char *fname[MAXPATHLEN + 1];
	int fd;

	if (corefile == NULL) {
		(void)snprintf(fname, sizeof(fname), "%d.core", pid);
		corefile = fname;
	}
	fd = open(corefile, O_RDWR|O_CREAT|O_TRUNC, DEFFILEMODE);
	if (fd < 0) {
		err(1, "%s: %s\n", corefile, strerror(errno));
	}
	return (fd);
}

/*
 * core --
 *	Build the core file.
 */
static void
core(efd, fd, ki)
	int efd;
	int fd;
	struct kinfo_proc *ki;
{
	union {
		struct user user;
		char ubytes[ctob(UPAGES)];
	} uarea;
	struct proc *p = &ki->kp_proc;
	int tsize = ki->kp_eproc.e_vm->vm_tsize;
	int dsize = ki->kp_eproc.e_vm->vm_dsize;
	int ssize = ki->kp_eproc.e_vm->vm_ssize;
	int cnt;

	/* Read in user struct */
	cnt = kvm_read(kd, (u_long)p->p_addr, &uarea, sizeof(uarea));
	if (cnt != sizeof(uarea)) {
		err(1, "read user structure: %s",
				cnt > 0 ? strerror(EIO) : strerror(errno));
	}

	/*
	 * Fill in the eproc vm parameters, since these are garbage unless
	 * the kernel is dumping core or something.
	 */
	uarea.user->u_kproc = ki;

	/* Dump user area */
	cnt = write(fd, &uarea, sizeof(uarea));
	if (cnt != sizeof(uarea)) {
		err(1, "write user structure: %s",
				cnt > 0 ? strerror(EIO) : strerror(errno));
	}

	/* Dump data segment */
	datadump(efd, fd, p, USRTEXT + ctob(tsize), dsize);

	/* Dump stack segment */
	userdump(fd, p, USRSTACK - ctob(ssize), ssize);
}

static void
datadump(efd, fd, p, addr, npage)
	register int efd;
	register int fd;
	struct proc *p;
	register u_long addr;
	register int npage;
{
	register int cc, delta;
	char buffer[PAGE_SIZE];

	delta = data_offset - addr;
	while (--npage >= 0) {
		cc = kvm_uread(kd, p, addr, buffer, PAGE_SIZE);
		if (cc != PAGE_SIZE) {
			/* Try to read the page from the executable. */
			if (lseek(efd, (off_t) addr + delta, SEEK_SET) == -1) {
				err(1, "seek executable: %s", strerror(errno));
			}
			cc = read(efd, buffer, sizeof(buffer));
			if (cc != sizeof(buffer)) {
				if (cc < 0) {
					err(1, "read executable: %s", strerror(errno));
				} else {
					/* Assume untouched bss page. */
					bzero(buffer, sizeof(buffer));
				}
			}
		}
		cc = write(fd, buffer, PAGE_SIZE);
		if (cc != PAGE_SIZE) {
			err(1, "write data segment: %s",
					cc > 0 ? strerror(EIO) : strerror(errno));
		}
		addr += PAGE_SIZE;
	}
}

static void
userdump(fd, p, addr, npage)
	register int fd;
	struct proc *p;
	register u_long addr;
	register int npage;
{
	register int cc;
	char buffer[PAGE_SIZE];

	while (--npage >= 0) {
		cc = kvm_uread(kd, p, addr, buffer, PAGE_SIZE);
		if (cc != PAGE_SIZE) {
			/* Could be an untouched fill-with-zero page. */
			bzero(buffer, PAGE_SIZE);
		}
		cc = write(fd, buffer, PAGE_SIZE);
		if (cc != PAGE_SIZE) {
			err(1, "write stack segment: %s",
					cc > 0 ? strerror(EIO) : strerror(errno));
		}
		addr += PAGE_SIZE;
	}
}

static void
usage(void)
{
	(void)fprintf(stderr, "usage: gcore [-s] [-c core] executable pid\n");
	exit(1);
}

static int
read_exec(argv, efd)
	char *argv[];
	int efd;
{
	int cnt;
#ifdef GCORE_ELF32
	cnt = read_elf32(argv, efd);
#endif
#ifdef GCORE_ELF32
	cnt = read_elf64(argv, efd);
#endif
#ifdef GCORE_AOUT
	cnt = read_aout(argv, efd);
#endif
	return (cnt);
}

#ifdef GCORE_AOUT
static int
read_aout(argv, efd)
	char *argv[];
	int efd;
{
	struct xexec exec;
	int cnt;

	cnt = read(efd, &exec.e, sizeof(exec.e));
	if (cnt != sizeof(exec.e)) {
		err(1, "%s aout header: %s", argv[0],
				cnt > 0 ? strerror(EIO) : strerror(errno));
	}
	data_offset = N_DATOFF(exec.e);
	return (cnt);
}
#endif

#ifdef GCORE_ELF32
static int
read_elf32(argv, efd)
	char *argv[];
	int efd;
{
	Elf32_Ehdr ex;
	Elf32_Phdr *ph;
	Elf32_Shdr *sh;
	char *shstrtab;
	int strtabix, symtabix;
	int cnt, i;

	cnt = read(efd, &ex, sizeof(ex));
	if (cnt != sizeof(ex)) {
		err(1, "%s elf32 header: %s", argv[0],
				cnt > 0 ? strerror(EIO) : strerror(errno));
	}
	/* Read the program headers... */
	ph = (Elf32_Phdr *) saveRead(cnt, ex.e_phoff, ex.e_phnum * sizeof(Elf32_Phdr), "ph");
	/* Read the section headers... */
	sh = (Elf32_Shdr *) saveRead(cnt, ex.e_shoff, ex.e_shnum * sizeof(Elf32_Shdr), "sh");
	/* Read in the section string table. */
	shstrtab = saveRead(cnt, sh[ex.e_shstrndx].sh_offset, sh[ex.e_shstrndx].sh_size, "shstrtab");
	for (i = 0; i < ex.e_shnum; i++) {
		char *name = shstrtab + sh[i].sh_name;
		if (!strcmp(name, ".symtab")) {
			symtabix = i;
		} else {
			if (!strcmp(name, ".strtab")) {
				strtabix = i;
			}
		}
	}
	data_offset = elf_datoff(cnt, sh[symtabix].sh_offset, sh[symtabix].sh_size, sh[strtabix].sh_offset, sh[strtabix].sh_size);
	return (cnt);
}
#endif

#ifdef GCORE_ELF64
static int
read_elf64(argv, efd)
	char *argv[];
	int efd;
{
	Elf64_Ehdr ex;
	Elf64_Phdr *ph;
	Elf64_Shdr *sh;
	char *shstrtab;
	int strtabix, symtabix;
	int cnt, i;

	cnt = read(efd, &ex, sizeof(ex));
	if (cnt != sizeof(ex)) {
		err(1, "%s elf64 header: %s", argv[0],
				cnt > 0 ? strerror(EIO) : strerror(errno));
	}
	/* Read the program headers... */
	ph = (Elf64_Phdr *) saveRead(cnt, ex.e_phoff, ex.e_phnum * sizeof(Elf64_Phdr), "ph");
	/* Read the section headers... */
	sh = (Elf64_Shdr *) saveRead(cnt, ex.e_shoff, ex.e_shnum * sizeof(Elf64_Shdr), "sh");
	/* Read in the section string table. */
	shstrtab = saveRead(cnt, sh[ex.e_shstrndx].sh_offset, sh[ex.e_shstrndx].sh_size, "shstrtab");
	for (i = 0; i < ex.e_shnum; i++) {
		char *name = shstrtab + sh[i].sh_name;
		if (!strcmp(name, ".symtab")) {
			symtabix = i;
		} else {
			if (!strcmp(name, ".strtab")) {
				strtabix = i;
			}
		}
	}
	data_offset = elf_datoff(cnt, sh[symtabix].sh_offset, sh[symtabix].sh_size, sh[strtabix].sh_offset, sh[strtabix].sh_size);
	return (cnt);
}
#endif

static int
elf_datoff(in, symoff, symsize, stroff, strsize)
	int     in;
	off_t   symoff, symsize;
	off_t   stroff, strsize;
{
	off_t sym, data;
	int offset;

	offset = 0;
	sym = symoff + symsize;
	data = stroff + strsize;
	if (syms < in) {
		if (data < in) {
			offset = (sym + data);
		} else {
			err(1, " elf .strtab is too large");
		}
	} else {
		err(1, "elf .symtab is too large");
	}
	if (offset > in) {
		err(1, "elf .strtab + .symtab (offset) is too large");
	}

	return (offset);
}

static char  *
saveRead(file, offset, len, name)
	int file;
	off_t offset, len;
	char *name;
{
	char   *tmp;
	int     count;
	off_t   off;

	off = lseek(file, offset, SEEK_SET);
	if (off < 0) {
		fprintf(stderr, "%s: fseek: %s\n", name, strerror(errno));
		exit(1);
	}
	if (!(tmp = (char *) malloc(len))) {
		errx(1, "%s: Can't allocate %ld bytes.", name, (long)len);
	}
	count = read(file, tmp, len);
	if (count != len) {
		fprintf(stderr, "%s: read: %s.\n",
				name, count ? strerror(errno) : "End of file reached");
		exit(1);
	}
	return (tmp);
}
