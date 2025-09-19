/*	$NetBSD: citrus_mapper_std.c,v 1.3 2003/07/12 15:39:20 tshiozak Exp $	*/

/*-
 * Copyright (c)2003 Citrus Project,
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: citrus_mapper_std.c,v 1.3 2003/07/12 15:39:20 tshiozak Exp $");
#endif /* LIBC_SCCS and not lint */

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/endian.h>
#include <sys/queue.h>

#include "citrus_types.h"
#include "citrus_region.h"
#include "citrus_memstream.h"
#include "citrus_bcs.h"
#include "citrus_frune.h"
#include "citrus_mmap.h"
#include "citrus_hash.h"
#include "citrus_mapper.h"

struct _citrus_mapper_ops _citrus_mapper_std_mapper_ops = {
		.mo_abi_version = _CITRUS_MAPPER_ABI_VERSION,
		.mo_init = _citrus_mapper_std_mapper_init,
		.mo_uninit = _citrus_mapper_std_mapper_uninit,
		.mo_convert = _citrus_mapper_std_mapper_convert,
		.mo_init_state = _citrus_mapper_std_mapper_init_state
};

int
_citrus_mapper_std_mapper_getops(struct _citrus_mapper_ops *ops, size_t lenops, u_int32_t expected_version)
{
	return (_citrus_getops(op, &_citrus_mapper_std_mapper_ops, lenops, _CITRUS_MAPPER_ABI_VERSION, expected_version));
}

static int
rowcol_convert()
{

}

static int
rowcol_init()
{

}

/*ARGSUSED*/
static int
_citrus_mapper_std_mapper_init(struct _citrus_mapper_area *__restrict ma,
			       struct _citrus_mapper * __restrict cm,
			       const char * __restrict curdir,
			       const void * __restrict var, size_t lenvar,
			       struct _citrus_mapper_traits * __restrict mt,
			       size_t lenmt)
{

}

/*ARGSUSED*/
static void
_citrus_mapper_std_mapper_uninit(struct _citrus_mapper *cm)
{

}

/*ARGSUSED*/
static void
_citrus_mapper_std_mapper_init_state(struct _citrus_mapper * __restrict cm,
				     void * __restrict ps)
{

}

/*ARGSUSED*/
static int
_citrus_mapper_std_mapper_convert(struct _citrus_mapper * __restrict cm,
				  _index_t * __restrict dst, _index_t src,
				  void * __restrict ps)
{

}
