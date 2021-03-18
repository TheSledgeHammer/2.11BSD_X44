/*-
 * Copyright (c) 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * The Mach Operating System project at Carnegie-Mellon University.
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
 *	@(#)alloc.c	8.1 (Berkeley) 6/11/93
 *  
 *
 * Copyright (c) 1989, 1990, 1991 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Author: Alessandro Forin
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */
/*	$NetBSD: calloc.c,v 1.2 2009/03/18 16:00:12 cegger Exp $	*/

#include <sys/cdefs.h>
#include <sys/types.h>

#include <lib/libsa/stand.h>

/*
 *	Dynamic memory allocator
 */
struct fl {
	struct fl	*next;
	size_t		size;
} *freelist = (struct fl *)0;

extern char end[];
static char *top = end;

void *
alloc(size)
	size_t size;
{
	register struct fl *f = freelist, **prev;

	prev = &freelist;
	while (f && f->size < size) {
		prev = &f->next;
		f = f->next;
	}
	if (f == (struct fl *)0) {
		f = (struct fl *)top;
		top += (size + 3) & ~3;
	} else
		*prev = f->next;
	return ((void *)f);
}

void
free(ptr, size)
	void *ptr;
	size_t size;
{
	register struct fl *f = (struct fl *)ptr;

	f->size = (size + 3) & ~3;
	f->next = freelist;
	freelist = f;
}

void *
calloc(size1, size2)
	size_t size1, size2;
{
	size_t total_size = size1 * size2;
	void *ptr;

	if(( (ptr = alloc(total_size)) != NULL)) {
		memset(ptr, 0, total_size);
	}

	/* alloc will crib for me. */

	return(ptr);
}
