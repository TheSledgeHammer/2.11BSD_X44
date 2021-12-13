/*
 * Copyright (c) 1987, 1993
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
 *
 *	@(#)malloc.h	8.5 (Berkeley) 5/3/95
 */

#ifndef _SYS_MALLOCTYPES_H_
#define _SYS_MALLOCTYPES_H_

#include <sys/malloc.h>

/* XXX: Below are Still In Development */

#define M_VMAMAP 		73	/* VM amap structures */
#define M_VMAOBJ 		74	/* VM aobject structure */
#define M_VMPSEG		93	/* pseudo-segment structure */
#define M_OVLMAP		75	/* OVL map structures */
#define	M_OVLMAPENT		76	/* OVL map entry structures */
#define M_OVLOBJ		77	/* OVL object structure */
#define M_OVLOBJHASH	78	/* OVL object hash structure */
#define M_KTPOOLTHREAD  79	/* kernel threadpool */
#define M_UTPOOLTHREAD  80	/* user threadpool */
#define M_ITPC			81	/* inter-threadpool communication */
#define M_WORKQUEUE		82	/* workqueue */
#define M_HTBC			83	/* HTree Blockchain structure */
#define M_SA			85	/* Scheduler Activations */
#define M_HTREE			86	/* vfs htree structure */
#define M_ADVVM			88 	/* AdvVM structures structures */
#define M_MPX			92	/* multiplexor structure */


/* Overlay Malloc */
#define overlay_malloc(size, type, flags) 				\
	malloc(size, type, flags | M_OVERLAY)

#define overlay_free(addr, type)						\
	free(addr, type | M_OVERLAY)

#define OVERLAY_MALLOC(space, cast, size, type, flags)	\
	(space) = (cast)overlay_malloc((u_long)(size), type, flags)

#define OVERLAY_FREE(addr, type)						\
	overlay_free((caddr_t)(addr), type)

#endif /* _SYS_MALLOCTYPES_H_ */
