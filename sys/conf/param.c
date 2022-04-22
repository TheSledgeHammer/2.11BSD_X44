/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)param.c	2.2 (2.11BSD GTE) 1997/2/14
 */
/*
 * Copyright (c) 1980, 1986, 1989, 1993
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
 *	@(#)param.c	8.3 (Berkeley) 8/20/94
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/socket.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/callout.h>
#include <sys/clist.h>
#include <sys/mbuf.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/map.h>

#include <ufs/ufs/quota.h>

/*
 * System parameter formulae.
 *
 * This file is copied into each directory where we compile
 * the kernel; it should be modified there to suit local taste
 * if necessary.
 *
 * Compiled with -DHZ=xx -DTIMEZONE=x -DDST=x -DMAXUSERS=xx
 */

#ifndef HZ
#define	HZ 100
#endif
int	hz = 			HZ;
int mshz = 			(1000000L + 60 - 1)/60;
int	tick = 			1000000 / HZ;
int	tickadj = 		30000 / (60 * HZ);			/* can adjust 30ms in 60s */
struct	timezone tz = { TIMEZONE, DST };
#define	NPROC 		(20 + 16 * MAXUSERS)
int	maxproc = 		NPROC;
int nproc = 		maxproc;
#define	NTEXT 		(80 + NPROC / 8)			/* actually the object cache */
int	ntext = 		NTEXT;
#define	NVNODE 		(NPROC + NTEXT + 100)
int	desiredvnodes = NVNODE;
#define NFILE		(3 * (NPROC + MAXUSERS) + 80)
int	maxfiles = 		NFILE;
#define NCLIST 		(60 + 12 * MAXUSERS)
int	nclist = 		NCLIST;
int	nmbclusters = 	NMBCLUSTERS;
int	fscale = 		FSCALE;						/* kernel uses `FSCALE', user uses `fscale' */

/*
 * These are initialized at bootstrap time
 * to values dependent on memory size
 */
int	nbuf, nswbuf;

/*
 * These have to be allocated somewhere; allocating
 * them here forces loader errors if this file is omitted
 * (if they've been externed everywhere else; hah!).
 */
struct 	callout 	*callout;
struct  cblock		*cfree;
struct	buf 		*buf, *swbuf;
char				*buffers;

#define CMAPSIZ		NPROC						/* size of core allocation map */
#define SMAPSIZ		((9 * NPROC) / 10)			/* size of swap allocation map */
int cmapsiz = 		CMAPSIZ;
int smapsiz = 		SMAPSIZ;

struct mapent		_coremap[CMAPSIZ];
struct map coremap[1] = {
		.m_map 		= _coremap,
		.m_limit 	= &_coremap[cmapsiz],
		.m_name 	= "coremap",
		.m_type		= M_COREMAP,
		.m_vmmap[1] = {
				{ .m_name = "buffer_map",   .m_vmmap = (vm_map_t) &buffer_map },
				{ .m_name = "exec_map",     .m_vmmap = (vm_map_t) &exec_map },
				{ .m_name = "kernel_map",   .m_vmmap = (vm_map_t) &kernel_map },
				{ .m_name = "kmem_map",     .m_vmmap = (vm_map_t) &kmem_map },
				{ .m_name = "mb_map",       .m_vmmap = (vm_map_t) &mb_map },
				{ .m_name = "phys_map",     .m_vmmap = (vm_map_t) &phys_map },
		},
		/*.m_ovlmap[1] = {
				{ .m_name = "overlay_map",  .m_ovlmap = (ovl_map_t) &overlay_map },
				{ .m_name = "omem_map",   	.m_ovlmap = (ovl_map_t) &omem_map },
		},*/
};

struct mapent	    _swapmap[SMAPSIZ];
struct map swapmap[1] = {
		.m_map 		= _swapmap,
		.m_limit 	= &_swapmap[smapsiz],
		.m_name 	= "swapmap",
		.m_type		= M_SWAPMAP,
		.m_vmmap    = NULL,
//		.m_ovlmap = NULL
};
