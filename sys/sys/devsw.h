/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *
 *	@(#)conf.c	8.3 (Berkeley) 1/21/94
 */
/*
 * The 3-Clause BSD License:
 * Copyright (c) 2020 Martin Kelly
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

#ifndef _SYS_DEVSW_H_
#define _SYS_DEVSW_H_

#include <sys/conf.h>
#include <sys/queue.h>
#include <sys/uio.h>

/*
 * device switch table
 */
struct devswtable {
	dev_t							dv_major;	/* device switch major */
    void                			*dv_data;	/* device switch data */
    int								dv_type;	/* device switch type */
};

struct devswtable_head;
TAILQ_HEAD(devswtable_head, devswtable_entry);
struct devswtable_entry {
	TAILQ_ENTRY(devswtable_entry)  	dve_link;
	struct devswtable				*dve_devswtable;
};
typedef struct devswtable_entry		*devswtable_entry_t;

/* devsw size */
#define	MAXDEVSW		512	/* the maximum of major device number */
#define	BDEVSW_SIZE		(sizeof(struct bdevsw *))
#define	CDEVSW_SIZE		(sizeof(struct cdevsw *))
#define	LINESW_SIZE		(sizeof(struct linesw *))

/* flags */
#define NODEVMAJOR 		(-1)

/* types */
#define BDEVTYPE 		0x01
#define CDEVTYPE 		0x02
#define LINETYPE 		0x04

/* conversion macros */
#define DTOB(dv)  		((struct bdevsw *)(dv)->dv_data)
#define DTOC(dv)  		((struct cdevsw *)(dv)->dv_data)
#define DTOL(dv)  		((struct linesw *)(dv)->dv_data)

struct buf;
struct proc;
struct tty;
struct uio;
struct vnode;
struct bdevsw;
struct cdevsw;
struct linesw;

/*
 * Types for d_type.
 */
#define	D_TAPE	1
#define	D_DISK	2
#define	D_TTY	3
#define	D_OTHER	4

#ifdef _KERNEL
/* devswtable & devsw_io */
extern struct devswtable 		sys_devsw;

void							devswtable_init(void);
int								devswtable_configure(struct devswtable *, dev_t, struct bdevsw *, struct cdevsw *, struct linesw *);
int								devsw_io_iskmemdev(dev_t);
int								devsw_io_iszerodev(dev_t);
int								devsw_io_isdisk(dev_t, int);
dev_t							devsw_io_chrtoblk(dev_t);
dev_t							devsw_io_blktochr(dev_t);

#define iskmemdev(dev)			devsw_io_iskmemdev(dev)
#define iszerodev(dev)			devsw_io_iszerodev(dev)
#define isdisk(dev, type)		devsw_io_isdisk(dev, type)
#define chrtoblk(cdev) 			devsw_io_chrtoblk(cdev)
#define blktochr(bdev)			devsw_io_blktochr(bdev)

/* macro: machine autoconfiguration */
#define DEVSWIO_CONFIG_INIT(devsw, major, bdev, cdev, line) 	\
	(devswtable_configure(devsw, major, bdev, cdev, line))

/* dev types */
#define	dev_type_open(n)		int n(dev_t, int, int, struct proc *)
#define	dev_type_close(n)		int n(dev_t, int, int, struct proc *)
#define	dev_type_read(n)		int n(dev_t, struct uio *, int)
#define	dev_type_write(n)		int n(dev_t, struct uio *, int)
#define	dev_type_ioctl(n) 		int n(dev_t, u_long, caddr_t, int, struct proc *)
#define dev_type_start(n)		int n(struct tty *)
#define	dev_type_stop(n)		int n(struct tty *, int)
#define	dev_type_tty(n)			struct tty *n(dev_t)
#define dev_type_select(n)		int n(dev_t, int, struct proc *)
#define	dev_type_poll(n)		int n(dev_t, int, struct proc *)
#define	dev_type_mmap(n)		caddr_t n(dev_t, off_t, int)
#define	dev_type_strategy(n)	int n(dev_t, int, int, struct proc *)
#define dev_type_modem(n)		int n(struct tty *, int)
#define dev_type_rint(n)		int n(int, struct tty *)
#define dev_type_kqfilter(n)	int n(dev_t, struct knote *)
#define dev_type_discard(n)		int n(dev_t, off_t, off_t)
#define	dev_type_dump(n)		int n(dev_t)
#define	dev_type_size(n)		daddr_t n(dev_t)

/* bdevsw-specific types */
dev_type_open(bdev_open);
dev_type_close(bdev_close);
dev_type_strategy(bdev_strategy);
dev_type_ioctl(bdev_ioctl);
dev_type_dump(bdev_dump);
dev_type_size(bdev_size);
dev_type_discard(bdev_discard);

/* cdevsw-specific types */
dev_type_open(cdev_open);
dev_type_close(cdev_close);
dev_type_read(cdev_read);
dev_type_write(cdev_write);
dev_type_ioctl(cdev_ioctl);
dev_type_stop(cdev_stop);
dev_type_tty(cdev_tty);
dev_type_select(cdev_select);
dev_type_poll(cdev_poll);
dev_type_mmap(cdev_mmap);
dev_type_strategy(cdev_strategy);
dev_type_kqfilter(cdev_kqfilter);
dev_type_discard(cdev_discard);

/* linesw-specific types */
dev_type_open(line_open);
dev_type_close(line_close);
dev_type_read(line_read);
dev_type_write(line_write);
dev_type_ioctl(line_ioctl);
dev_type_rint(line_rint);
dev_type_start(line_start);
dev_type_modem(line_modem);
dev_type_poll(line_poll);

/* no dev routines */
#define	noopen				(enodev)
#define	noclose				(enodev)
#define	noread				(enodev)
#define	nowrite				(enodev)
#define	noioctl				(enodev)
#define	nostart				(enodev)
#define	nostop				(enodev)
#define	notty				(enodev)
#define	noselect			(enodev)
#define	nopoll				(enodev)
#define	nommap				(enodev)
#define	nostrategy			(enodev)
#define	nomodem				(enodev)
#define	norint				(enodev)
#define	nokqfilter			seltrue_kqfilter
#define	nodiscard			(enodev)
#define	nodump				(enodev)
#define	nosize				(enodev)

/* null dev routines */
#define	nullopen			(nullop)
#define	nullclose			(nullop)
#define	nullread			(nullop)
#define	nullwrite			(nullop)
#define	nullioctl			(nullop)
#define	nullstart			(nullop)
#define	nullstop			(nullop)
#define	nulltty				(nullop)
#define	nullselect			(nullop)
#define	nullpoll			(nullop)
#define	nullmmap			(nullop)
#define	nullstrategy		(nullop)
#define	nullmodem			(nullop)
#define	nullrint			(nullop)
#define	nullkqfilter		(nullop)
#define	nulldiscard			(nullop)
#define	nulldump			(nullop)
#define	nullsize			(nullop)
#endif /* _KERNEL */
#endif /* _SYS_DEVSW_H_ */
