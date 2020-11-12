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

#ifndef _DEV_CONF_DECL_H_
#define _DEV_CONF_DECL_H_

#include <sys/conf.h>

struct buf;
struct proc;
struct tty;
struct uio;
struct vnode;

/*
 * Types for d_type.
 */
#define	D_TAPE	1
#define	D_DISK	2
#define	D_TTY	3

#ifdef _KERNEL
#define	dev_type_open(n)		int n(dev_t, int, int, struct proc *)
#define	dev_type_close(n)		int n(dev_t, int, int, struct proc *)
#define	dev_type_read(n)		int n(dev_t, struct uio *, int)
#define	dev_type_write(n)		int n(dev_t, struct uio *, int)
#define	dev_type_ioctl(n) 		int n(dev_t, u_long, caddr_t, int, struct proc *)
#define	dev_type_stop(n)		void n(struct tty *, int)
#define	dev_type_tty(n)			struct tty * n(dev_t)
#define	dev_type_poll(n)		int n(dev_t, int, struct lwp *)
#define	dev_type_mmap(n)		caddr_t n(dev_t, off_t, int)
#define	dev_type_strategy(n)	void n(struct buf *)
#define	dev_type_dump(n)		int n (dev_t)
#define	dev_type_size(n)		int n (dev_t)

#define devtype(c)				__CONCAT(dev_type_, c)
#define	dev_decl(n,t) 			__CONCAT(devtype(t), __CONCAT(n, t))
#define	dev_init(c,n,t) 		((c) > 0 ? __CONCAT(n,t) : (__CONCAT(dev_type_,t)((*))) enxio)


/* bdevsw-specific initializations */
#define	dev_size_init(c,n)		(c > 0 ? __CONCAT(n,size) : 0)

/* bdevsw-specific types */
dev_type_open(bdev_open);
dev_type_close(bdev_close);
dev_type_strategy(bdev_strategy);
dev_type_ioctl(bdev_ioctl);
dev_type_dump(bdev_dump);
dev_type_size(bdev_size);

#define	bdev_decl(n) 														\
	dev_decl(n,open); dev_decl(n,close); dev_decl(n,strategy); 				\
	dev_decl(n,ioctl); dev_decl(n,dump); dev_decl(n,size)

#define	bdev_disk_init(c,n) { 												\
	dev_init(c,n,open), dev_init(c,n,close), 								\
	dev_init(c,n,strategy), dev_init(c,n,ioctl), 							\
	dev_init(c,n,dump), dev_size_init(c,n), D_DISK }

#define	bdev_swap_init(c,n) { 												\
	(dev_type_open((*))) enodev, (dev_type_close((*))) enodev, 				\
	dev_init(c,n,strategy), (dev_type_ioctl((*))) enodev, 					\
	(dev_type_dump((*))) enodev, 0 }

#define	bdev_tape_init(c,n) { 												\
	dev_init(c,n,open), dev_init(c,n,close), 								\
	dev_init(c,n,strategy), dev_init(c,n,ioctl), 							\
	dev_init(c,n,dump), 0, B_TAPE }

#define	bdev_notdef() { 													\
	(dev_type_open((*))) enodev, (dev_type_close((*))) enodev, 				\
	(dev_type_strategy((*))) enodev, (dev_type_ioctl((*))) enodev, 			\
	(dev_type_dump((*))) enodev, 0 }

/* cdevsw-specific types */
dev_type_open(cdev_open);
dev_type_close(cdev_close);
dev_type_read(cdev_read);
dev_type_write(cdev_write);
dev_type_ioctl(cdev_ioctl);
dev_type_stop(cdev_stop);
dev_type_tty(cdev_tty);
dev_type_poll(cdev_poll);
dev_type_mmap(cdev_mmap);

#define	cdev_decl(n) 														\
	dev_decl(n,open); dev_decl(n,close); dev_decl(n,read); 					\
	dev_decl(n,write); dev_decl(n,ioctl); dev_decl(n,stop); 				\
	dev_decl(n,reset); dev_decl(n,tty); dev_decl(n,select);					\
	dev_decl(n,poll); dev_decl(n,mmap);

extern struct tty __CONCAT(n,_tty);

#define	dev_tty_init(c,n)	(c > 0 ? __CONCAT(n,_tty) : 0)

/* open, read, write, ioctl, strategy */
#define	cdev_disk_init(c,n) { 												\
	dev_init(c,n,open), (dev_type_close((*))) nullop, dev_init(c,n,read), 	\
	dev_init(c,n,write), dev_init(c,n,ioctl), (dev_type_stop((*))) enodev, 	\
	(dev_type_reset((*))) nullop, 0, seltrue, (dev_type_mmap((*))) enodev, 	\
	dev_init(c,n,strategy) }

/* open, close, read, write, ioctl, strategy */
#define	cdev_tape_init(c,n) { 												\
	dev_init(c,n,open), dev_init(c,n,close), dev_init(c,n,read), 			\
	dev_init(c,n,write), dev_init(c,n,ioctl), (dev_type_stop((*))) enodev, 	\
	(dev_type_reset((*))) nullop, 0, seltrue, (dev_type_mmap((*))) enodev, 	\
	dev_init(c,n,strategy) }

/* open, close, read, write, ioctl, stop, tty */
#define	cdev_tty_init(c,n) { 												\
	dev_init(c,n,open), dev_init(c,n,close), dev_init(c,n,read), 			\
	dev_init(c,n,write), dev_init(c,n,ioctl), dev_init(c,n,stop), 			\
	(dev_type_reset((*))) nullop, dev_tty_init(c,n), ttselect, 				\
	(dev_type_mmap((*))) enodev, 0 }

#define	cdev_notdef() { \
	(dev_type_open((*))) enodev, (dev_type_close((*))) enodev, 				\
	(dev_type_read((*))) enodev, (dev_type_write((*))) enodev, 				\
	(dev_type_ioctl((*))) enodev, (dev_type_stop((*))) enodev, 				\
	(dev_type_reset((*))) nullop, 0, seltrue, 								\
	(dev_type_mmap((*))) enodev, 0 }

/* open, close, read, write, ioctl, select -- XXX should be a tty */
#define	cdev_cn_init(c,n) { 												\
	dev_init(c,n,open), dev_init(c,n,close), dev_init(c,n,read), 			\
	dev_init(c,n,write), dev_init(c,n,ioctl), (dev_type_stop((*))) nullop, 	\
	(dev_type_reset((*))) nullop, 0, dev_init(c,n,select), 					\
	(dev_type_map((*))) enodev, 0 }

/* open, read, write, ioctl, select -- XXX should be a tty */
#define	cdev_ctty_init(c,n) { 												\
	dev_init(c,n,open), (dev_type_close((*))) nullop, dev_init(c,n,read), 	\
	dev_init(c,n,write), dev_init(c,n,ioctl), (dev_type_stop((*))) nullop, 	\
	(dev_type_reset((*))) nullop, 0, dev_init(c,n,select), 					\
	(dev_type_map((*))) enodev, 0 }

/* read/write */
#define	cdev_mm_init(c,n) { \
	(dev_type_open((*))) nullop, (dev_type_close((*))) nullop, mmrw, 		\
	mmrw, (dev_type_ioctl((*))) enodev, (dev_type_stop((*))) nullop, 		\
	(dev_type_reset((*))) nullop, 0, seltrue, (dev_type_map((*))) enodev, 0 }

/* read, write, strategy */
#define	cdev_swap_init(c,n) { \
	(dev_type_open((*))) nullop, (dev_type_close((*))) nullop, rawread, 	\
	rawwrite, (dev_type_ioctl((*))) enodev, (dev_type_stop((*))) enodev, 	\
	(dev_type_reset((*))) nullop, 0, (dev_type_select((*))) enodev, 		\
	(dev_type_map((*))) enodev, dev_init(c,n,strategy) }

/* open, close, read, write, ioctl, tty, select */
#define	cdev_ptc_init(c,n) { 												\
	dev_init(c,n,open), dev_init(c,n,close), dev_init(c,n,read), 			\
	dev_init(c,n,write), dev_init(c,n,ioctl), (dev_type_stop((*))) nullop, 	\
	(dev_type_reset((*))) nullop, dev_tty_init(c,n), dev_init(c,n,select), 	\
	(dev_type_map((*))) enodev, 0 }

/* open, close, read, ioctl, select -- XXX should be a generic device */
#define	cdev_log_init(c,n) { 												\
	dev_init(c,n,open), dev_init(c,n,close), dev_init(c,n,read), 			\
	(dev_type_write((*))) enodev, dev_init(c,n,ioctl), 						\
	(dev_type_stop((*))) enodev, (dev_type_reset((*))) nullop, 0, 			\
	dev_init(c,n,select), (dev_type_map((*))) enodev, 0 }

/* open, close, read */
#define cdev_ksyms_init(c,n) { 												\
	dev_init(c,n,open), dev_init(c,n,close), dev_init(c,n,read), 			\
	(dev_type_write((*))) enodev, (dev_type_ioctl((*))) enodev, 			\
	(dev_type_stop((*))) enodev, 0, seltrue, 								\
	(dev_type_mmap((*))) enodev, 0, 0 }

/* open, close, read, write, ioctl, tty -- XXX should be a tty */
extern struct tty 	pccons;
#define	cdev_pc_init(c,n) { 												\
	dev_init(c,n,open), dev_init(c,n,close), dev_init(c,n,read), 			\
	dev_init(c,n,write), dev_init(c,n,ioctl), (dev_type_stop((*))) nullop, 	\
	(dev_type_reset((*))) nullop, &pccons, ttselect, 						\
	(dev_type_map((*))) enodev, 0 }

/* open, close, read, write, ioctl, select -- XXX should be generic device */
#define	cdev_bpf_init(c,n) { 												\
	dev_init(c,n,open), dev_init(c,n,close), dev_init(c,n,read), 			\
	dev_init(c,n,write), dev_init(c,n,ioctl), (dev_type_stop((*))) enodev, 	\
	(dev_type_reset((*))) enodev, 0, dev_init(c,n,select), 					\
	(dev_type_map((*))) enodev, 0 }

bdev_decl(wd);
bdev_decl(Fd);
bdev_decl(wt);
bdev_decl(xd);
cdev_decl(cn);
cdev_decl(ctty);
dev_type_read(mmrw);
cdev_decl(pts);
cdev_decl(ptc);
cdev_decl(log);
cdev_decl(ksyms);
cdev_decl(wd);
cdev_decl(Fd);
cdev_decl(xd);
cdev_decl(wt);
cdev_decl(com);
cdev_decl(pc);
cdev_decl(bpf);
#endif /* _KERNEL */
#endif /* _DEV_CONF_DECL_H_ */
