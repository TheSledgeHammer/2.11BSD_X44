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
 */

#ifndef _SYS_CPU_H_
#define _SYS_CPU_H_

#include <devel/sys/bitmap.h>

union cpu_top {
    uint32_t 			ct_mask;
};
typedef union cpu_top 	ctop_t;

ctop_t 				 	*ctop;

static __inline void
ctop_init()
{
	ctop = (ctop_t *)malloc(sizeof(ctop_t *));
	bitmap_init();
}

static __inline void
ctop_set(top, val)
	ctop_t 	*top;
	uint32_t val;
{
    top = ctop;
    top->ct_mask = val;
    bitmap_insert(val);
}

static __inline uint32_t
ctop_get(top)
	ctop_t *top;
{
    top = ctop;
    return (bitmap_search(top->ct_mask)->value);
}

static __inline void
ctop_remove(top)
	ctop_t *top;
{
    top = ctop;
    bitmap_remove(top->ct_mask);
}

static __inline int
ctop_isset(top, val)
	ctop_t *top;
	uint32_t val;
{
	if(top->ct_mask != val) {
		return (1);
	}
	return (0);
}

/* ctop macros */
#define CPU_SET(top, val) 	(ctop_set(top, val))
#define CPU_GET(top) 		(ctop_get(top))
#define CPU_ISSET(top, val)	(ctop_isset(top, val))
#define CPU_EMPTY(top)		((top) == NULL)

#endif /* _SYS_CPU_H_ */
