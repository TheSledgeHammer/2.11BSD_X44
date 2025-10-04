/*	$NetBSD: citrus_module.c,v 1.14 2024/06/09 18:55:00 mrg Exp $	*/

/*-
 * Copyright (c)1999, 2000, 2001, 2002 Citrus Project,
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

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*-
 * Copyright (c) 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Paul Borman at Krystal Technologies.
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
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: citrus_module.c,v 1.14 2024/06/09 18:55:00 mrg Exp $");
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "citrus_rune.h"
#include "citrus_types.h"
#include "citrus_hash.h"
#include "citrus_iconv.h"
#include "citrus_mapper.h"
#include "citrus_mapper_serial.h"
#include "citrus_module.h"

extern struct _citrus_iconv_ops _citrus_iconv_std_iconv_ops;
extern struct _citrus_mapper_ops _citrus_mapper_parallel_mapper_ops;
extern struct _citrus_mapper_ops _citrus_mapper_serial_mapper_ops;
extern struct _citrus_mapper_ops _citrus_mapper_std_mapper_ops;

struct _citrus_table {
	const char 	*ct_name;
	void	    *ct_ops;
};

/* citrus_iconv ops */
const struct _citrus_table _citrus_iconvtab[] = {
		{		/* iconv_none */
				.ct_name = "_citrus_iconv_none_iconv_ops",
				.ct_ops =  NULL,
		},
		{		/* iconv_std */
				.ct_name = "_citrus_iconv_std_iconv_ops",
				.ct_ops =  &_citrus_iconv_std_iconv_ops,
		}
};

/* citrus_mapper ops */
const struct _citrus_table _citrus_mappertab[] = {

		{		/* mapper_646 */
				.ct_name = "_citrus_mapper_646_mapper_ops",
				.ct_ops = NULL,
		},
		{		/* mapper_none */
				.ct_name = "_citrus_mapper_none_mapper_ops",
				.ct_ops = NULL,
		},
		{		/* mapper_parallel */
				.ct_name = "_citrus_mapper_parallel_mapper_ops",
				.ct_ops = &_citrus_mapper_parallel_mapper_ops,
		},
		{		/* mapper_serial */
				.ct_name = "_citrus_mapper_serial_mapper_ops",
				.ct_ops = &_citrus_mapper_serial_mapper_ops,
		},
		{		/* mapper_std */
				.ct_name = "_citrus_mapper_std_mapper_ops",
				.ct_ops = &_citrus_mapper_std_mapper_ops,
		},
		{		/* mapper_zone */
				.ct_name = "_citrus_mapper_zone_mapper_ops",
				.ct_ops = NULL,
		},
};

#define _CITRUS_ICONV_TABLE_SIZE 	(sizeof(_citrus_iconvtab) / sizeof(_citrus_iconvtab[0]))
#define _CITRUS_MAPPER_TABLE_SIZE 	(sizeof(_citrus_mappertab) / sizeof(_citrus_mappertab[0]))

void *
_citrus_find_getops(const char *modname, const char *ifname)
{
	const struct _citrus_table *handle;
	char name[PATH_MAX];
	void *p;
	int i, len = 0;

	_DIAGASSERT(modname != NULL);
	_DIAGASSERT(ifname != NULL);

	(void)snprintf(name, sizeof(name), "_citrus_%s_%s_ops", modname, ifname);
	if (strncmp(ifname, "iconv", sizeof(*ifname)) == 0) {
		handle = _citrus_iconvtab;
		len = _CITRUS_ICONV_TABLE_SIZE;
	}
	if (strncmp(ifname, "mapper", sizeof(*ifname)) == 0) {
		handle = _citrus_mappertab;
		len = _CITRUS_MAPPER_TABLE_SIZE;
	}
	if ((handle == NULL) || (len == 0)) {
		return (NULL);
	}
	for (i = 0; i < len; i++) {
		if (strncmp(name, handle[i].ct_name, sizeof(name)) == 0) {
			p = handle[i].ct_ops;
			return (p);
		}
	}
	return (NULL);
}

/*
 * getops
 */
static __inline int
_citrus_getops(void *toops, size_t tolen, void *fromops, size_t fromlen, size_t lenops, uint32_t abi_version, uint32_t expected_version)
{
	if (expected_version < abi_version || lenops < tolen) {
		return (EINVAL);
	}
	memcpy(toops, fromops, fromlen);
	return (0);
}

int
_citrus_iconv_getops(struct _citrus_iconv_ops *toops, struct _citrus_iconv_ops *fromops, size_t lenops, uint32_t expected_version)
{
	return (_citrus_getops(toops, sizeof(toops), fromops, sizeof(fromops), lenops, _CITRUS_ICONV_ABI_VERSION, expected_version));
}

int
_citrus_mapper_getops(struct _citrus_mapper_ops *toops, struct _citrus_mapper_ops *fromops, size_t lenops, uint32_t expected_version)
{
	return (_citrus_getops(toops, sizeof(toops), fromops, sizeof(fromops), lenops, _CITRUS_MAPPER_ABI_VERSION, expected_version));
}
