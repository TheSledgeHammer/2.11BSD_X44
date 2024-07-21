/*	$NetBSD: puffs_msgif.c,v 1.8 2006/11/21 01:53:33 pooka Exp $	*/

/*
 * Copyright (c) 2005, 2006  Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by the
 * Google Summer of Code program and the Ulla Tuominen Foundation.
 * The Google SoC project was mentored by Bill Studenmund.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: puffs_msgif.c,v 1.8 2006/11/21 01:53:33 pooka Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/devsw.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/socketvar.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/lock.h>
#include <sys/poll.h>

#include <fs/puffs/puffs_msgif.h>
#include <fs/puffs/puffs_mount.h>
#include <fs/puffs/puffs.h>

int	puffscdopen(dev_t, int, int, struct proc *);
int	puffscdclose(dev_t, int, int, struct proc *);

/*
 * Device routines
 */

dev_type_open(puffscdopen);
dev_type_close(puffscdclose);
//dev_type_ioctl(puffscdioctl);

/* dev */
const struct cdevsw puffs_cdevsw = {
		.d_open = puffscdopen,
		.d_close = puffscdclose,
		.d_read = noread,
		.d_write = nowrite,
		.d_ioctl = noioctl,
		.d_stop = nostop,
		.d_tty = notty,
		.d_poll = nopoll,
		.d_mmap = nommap,
		.d_kqfilter = nokqfilter,
		.d_discard = nodiscard,
		.d_type = D_OTHER
};

static int puffs_fop_rw(struct file *, struct uio *, struct ucred *);
static int puffs_fop_read(struct file *, struct uio *, struct ucred *);
static int puffs_fop_write(struct file *, struct uio *, struct ucred *);
static int puffs_fop_ioctl(struct file*, u_long, void *, struct proc *);
static int puffs_fop_poll(struct file *, int, struct proc *);
static int puffs_fop_stat(struct file *, struct stat *, struct proc *);
static int puffs_fop_close(struct file *, struct proc *);
static int puffs_fop_kqfilter(struct file *, struct knote *);

/* fd routines, for cloner */
struct fileops puffs_fileops = {
		.fo_rw = puffs_fop_rw,
		.fo_read = puffs_fop_read,
		.fo_write = puffs_fop_write,
		.fo_ioctl = puffs_fop_ioctl,
		.fo_poll = puffs_fop_poll,
		.fo_stat = puffs_fop_stat,
		.fo_close = puffs_fop_close,
		.fo_kqfilter = puffs_fop_kqfilter,
};

/*
 * puffs instance structures.  these are always allocated and freed
 * from the context of the device node / fileop code.
 */
struct puffs_instance {
	pid_t 	pi_pid;
	int 	pi_idx;
	int 	pi_fd;
	struct puffs_mount *pi_pmp;
	struct selinfo pi_sel;

	TAILQ_ENTRY(puffs_instance) pi_entries;
};

#define PMP_EMBRYO 	((struct puffs_mount *)-1)	/* before mount	*/
#define PMP_DEAD 	((struct puffs_mount *)-2)	/* goner	*/

static TAILQ_HEAD(, puffs_instance) puffs_ilist = TAILQ_HEAD_INITIALIZER(puffs_ilist);
