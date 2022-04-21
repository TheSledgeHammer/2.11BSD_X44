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
#define DTOB(dv)  		((const struct bdevsw *)(dv)->dv_data)
#define DTOC(dv)  		((const struct cdevsw *)(dv)->dv_data)
#define DTOL(dv)  		((const struct linesw *)(dv)->dv_data)

struct buf;
struct proc;
struct tty;
struct uio;
struct vnode;
struct bdevsw;
struct cdevsw;
struct linesw;

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

dev_t							bdevsw_lookup_major(const struct bdevsw *);
dev_t							cdevsw_lookup_major(const struct cdevsw *);
dev_t							linesw_lookup_major(const struct linesw *);
const struct bdevsw 			*bdevsw_lookup(dev_t);
const struct cdevsw 			*cdevsw_lookup(dev_t);
const struct linesw 			*linesw_lookup(dev_t);

#define iskmemdev(dev)			devsw_io_iskmemdev(dev)
#define iszerodev(dev)			devsw_io_iszerodev(dev)
#define isdisk(dev, type)		devsw_io_isdisk(dev, type)
#define chrtoblk(cdev) 			devsw_io_chrtoblk(cdev)
#define blktochr(bdev)			devsw_io_blktochr(bdev)

/* macro: machine autoconfiguration */
#define DEVSWIO_CONFIG_INIT(devsw, major, bdev, cdev, line) 	\
	(devswtable_configure(devsw, major, bdev, cdev, line))
#endif /* _KERNEL */

/* machine/conf.c */
extern void conf_init(struct devswtable *);

/* kernel */
extern void kernel_init(struct devswtable *);
extern void	log_init(struct devswtable *);
extern void	swap_init(struct devswtable *);
extern void	tty_init(struct devswtable *);

/* devices */
extern void device_init(struct devswtable *);
extern void	audio_init(struct devswtable *);
extern void	console_init(struct devswtable *);
extern void	core_init(struct devswtable *);
extern void	disk_init(struct devswtable *);
extern void	misc_init(struct devswtable *);
extern void	usb_init(struct devswtable *);
extern void	video_init(struct devswtable *);
extern void	wscons_init(struct devswtable *);

/* network */
extern void	network_init(struct devswtable *);
#endif /* _SYS_DEVSW_H_ */
