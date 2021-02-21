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
 *
 *	@(#)advvm_extent.h 1.0 	20/02/21
 */

#ifndef _ADVVM_EXTENT_H_
#define _ADVVM_EXTENT_H_

#include <sys/extent.h>
#include <sys/tree.h>

struct advvm_storage {
	struct extent 				*adp_extent;
	u_long						adp_start;				/* start of extent */
	u_long						adp_end;				/* end of extent */
	caddr_t 					adp_storage;			/* fixed storage */
	size_t						adp_storagesize;		/* fixed storage size */
	int 						adp_flags;				/* see below */

	u_long						adp_pool;				/* sub-region resulting pool */

	SPLAY_HEAD(,advvm_pool) 	adp_root;
};
typedef struct advvm_storage 	advvm_storage_t;

struct advvm_region {
	SPLAY_ENTRY(advvm_region) 	adr_entry;
};

/* advvm flags */
#define ADVVM_NOEXTENT			0x01

/* prototypes */

void 			advvm_storage_create(char *, u_long, u_long, caddr_t, size_t, int);
int				advvm_allocate_region(advvm_storage_t *, u_long, u_long, int);
int				advvm_allocate_subregion(advvm_storage_t *, u_long, u_long, u_long, int);
int				advvm_free(advvm_storage_t *, u_long, u_long, int);
void			advvm_destroy(advvm_storage_t *);
u_long 			*advvm_get_storage_pool(advvm_storage_t *);
/* generic malloc & free */
void			advvm_malloc(void *, u_long);
void			advvm_free(void *);

#endif /* _ADVVM_EXTENT_H_ */
