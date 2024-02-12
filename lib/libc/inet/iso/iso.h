/*	$NetBSD: iso.h,v 1.15 2004/04/22 01:01:41 matt Exp $	*/

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
 *	@(#)iso.h	8.1 (Berkeley) 6/10/93
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

#ifndef _NETISO_ISO_H_
#define _NETISO_ISO_H_

#include <sys/ansi.h>

#ifndef IN_CLASSA_NET
#include <netinet/in.h>
#endif /* IN_CLASSA_NET */

/*
 * The following looks like a sockaddr to facilitate using tree lookup
 * routines
 */
struct iso_addr {
	u_char          isoa_len;			/* length (in bytes) */
	char            isoa_genaddr[20];	/* general opaque address */
};

struct sockaddr_iso {
	u_char          siso_len;			/* length */
	sa_family_t     siso_family;		/* family */
	u_char          siso_plen;			/* presentation selector length */
	u_char          siso_slen;			/* session selector length */
	u_char          siso_tlen;			/* transport selector length */
	struct iso_addr siso_addr;			/* network address */
	u_char          siso_pad[6];		/* space for gosip v2 sels */
	/* makes struct 32 bytes long */
};
#define siso_nlen siso_addr.isoa_len
#define siso_data siso_addr.isoa_genaddr

#define TSEL(s) ((caddr_t)((s)->siso_data + (s)->siso_nlen))

#define SAME_ISOADDR(a, b) 		\
	(bcmp((a)->siso_data, (b)->siso_data, (unsigned)(a)->siso_nlen) == 0)
#define SAME_ISOIFADDR(a, b) 	\
	(bcmp((a)->siso_data, (b)->siso_data, (unsigned)((b)->siso_nlen - (b)->siso_tlen)) == 0)

#ifdef _KERNEL

#define	satosiso(sa)	((struct sockaddr_iso *)(sa))
#define	satocsiso(sa)	((const struct sockaddr_iso *)(sa))
#define	sisotosa(siso)	((struct sockaddr *)(siso))

#endif /* _KERNEL */

#endif /* _NETISO_ISO_H_ */
