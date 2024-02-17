/*	$NetBSD: tp_param.h,v 1.14 2003/08/11 15:17:30 itojun Exp $	*/

/*-
 * Copyright (c) 1991, 1993
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
 * 3. Neither the name of the University nor the names of its contributors
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
 *	@(#)tp_param.h	8.1 (Berkeley) 6/10/93
 */

/***********************************************************
		Copyright IBM Corporation 1987

                      All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of IBM not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

IBM DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
IBM BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

/*
 * ARGO Project, Computer Sciences Dept., University of Wisconsin - Madison
 */

#ifndef _NETISO_TP_ISO_H_
#define _NETISO_TP_ISO_H_

/******************************************************
 * Some fundamental data types
 *****************************************************/

#define		TP_LOCAL		22
#define		TP_FOREIGN		33

#ifndef 	EOK
#define 	EOK 			0
#endif				/* EOK */

#define 	TP_CLASS_0 		(1<<0)
#define 	TP_CLASS_1 		(1<<1)
#define 	TP_CLASS_2 		(1<<2)
#define 	TP_CLASS_3 		(1<<3)
#define 	TP_CLASS_4 		(1<<4)

#define 	TP_FORCE 		0x1
#define 	TP_STRICT 		0x2

/*
 * These sockopt level definitions should be considered for socket.h
 */
#define	SOL_TRANSPORT		0xfffe
#define	SOL_NETWORK			0xfffd

#ifdef _KERNEL
struct isopcb;
struct mbuf;
struct sockaddr;

/* tp_iso.c */
void 	iso_getsufx(void *, u_short *, caddr_t, int);
void 	iso_putsufx(void *, caddr_t, int, int);
void 	iso_recycle_tsuffix(void *);
void 	iso_putnetaddr(void *, struct sockaddr *, int);
int 	iso_cmpnetaddr(void *, struct sockaddr *, int);
void 	iso_getnetaddr(void *, struct mbuf *, int);
void 	iso_rtchange(struct isopcb *);
#endif
#endif /* _NETISO_TP_ISO_H_ */
