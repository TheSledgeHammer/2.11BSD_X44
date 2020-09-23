/*
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
 *	@(#)vm.h	8.5 (Berkeley) 5/11/95
 */

#ifndef _AVM_H_
#define _AVM_H_

struct avm_anon;
typedef struct avm_anon			*avm_anon_t;

struct avm_map;
typedef struct avm_map 			*avm_map_t;

struct avm_map_entry;
typedef struct avm_map_entry 	*avm_map_entry_t;

struct avm_object;
typedef struct avm_object		*avm_object_t;

struct avm_ref;
typedef struct avm_ref			*avm_ref_t;

#include <devel/vm/include/vm.h>

/*
 * Shareable process anonymous virtual address space.
 */
struct avmspace {
	struct avm_map   avm_amap;		/* AVM address map */
	struct extent	 *avm_extent;	/* extent manager */

	int				 avm_refcnt;	/* number of references */

	segsz_t 		 avm_rssize; 	/* current resident set size in pages */
	segsz_t 		 avm_swrss;		/* resident set size before last swap */
	segsz_t 		 avm_tsize;		/* text size (pages) XXX */
	segsz_t 		 avm_dsize;		/* data size (pages) XXX */
	segsz_t 		 avm_ssize;		/* stack size (pages) */
	caddr_t			 avm_taddr;		/* user virtual address of text XXX */
	caddr_t			 avm_daddr;		/* user virtual address of data XXX */
	caddr_t 		 avm_minsaddr;	/* user VA at min stack growth */
	caddr_t 		 avm_maxsaddr;	/* user VA at max stack growth */
};

#endif /* _AVM_H_ */
