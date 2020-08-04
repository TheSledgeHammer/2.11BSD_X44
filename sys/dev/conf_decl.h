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

#ifndef _SYS_CONF_DECL_H_
#define _SYS_CONF_DECL_H_

#include <sys/conf.h>

#ifdef _KERNEL
#define	dev_type_open(n)		int n(dev_t, int, int, struct proc *)
#define	dev_type_close(n)		int n(dev_t, int, int, struct proc *)
#define	dev_type_strategy(n)	void n(struct buf *)
#define	dev_type_ioctl(n) 		int n(dev_t, u_long, caddr_t, int, struct proc *)

#define	dev_decl(n,t)	__CONCAT(dev_type_,t)(__CONCAT(n,t))
#define	dev_init(c,n,t) \
	((c) > 0 ? __CONCAT(n,t) : (__CONCAT(dev_type_,t)((*))) enxio)

/* bdevsw-specific types */
#define	dev_type_dump(n)	int n(dev_t, daddr_t, caddr_t, size_t)
#define	dev_type_size(n)	daddr_t n(dev_t)

/* bdevsw-specific initializations */
#define	dev_size_init(c,n)	(c > 0 ? __CONCAT(n,size) : 0)

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

#define	bdev_notdef() { \
	(dev_type_open((*))) enodev, (dev_type_close((*))) enodev, 				\
	(dev_type_strategy((*))) enodev, (dev_type_ioctl((*))) enodev, 			\
	(dev_type_dump((*))) enodev, 0 }

/* cdevsw-specific types */
#define	dev_type_read(n)		int n(dev_t, struct uio *, int)
#define	dev_type_write(n)		int n(dev_t, struct uio *, int)
#define	dev_type_stop(n)		int n(struct tty *, int)
#define	dev_type_reset(n)		int n(int)
#define	dev_type_tty(n)			struct tty *n(dev_t)
#define	dev_type_select(n)		int n(dev_t, int, struct proc *)
#define	dev_type_poll(n)		int n(dev_t, int, struct proc *)
#define	dev_type_mmap(n)		int n()

#define	cdev_decl(n) 														\
	dev_decl(n,open); dev_decl(n,close); dev_decl(n,read); 					\
	dev_decl(n,write); dev_decl(n,ioctl); dev_decl(n,stop); 				\
	dev_decl(n,reset); dev_decl(n,tty); dev_decl(n,select);					\
	dev_decl(n,poll); dev_decl(n,mmap);

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

#endif /* _KERNEL */
#endif /* _SYS_CONF_DECL_H_ */
