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

#ifndef _SYS_BITLIST_H_
#define _SYS_BITLIST_H_

#include <sys/types.h>
#include <sys/queue.h>

struct bitlist_header;
LIST_HEAD(bitlist_header, bitlist);
struct bitlist {
	LIST_ENTRY(bitlist) 	entry;
    union {
        uint64_t        	value;
    };
};
typedef struct bitlist   	bitlist_t;

union cpu_top {
    uint32_t 				ct_mask;
};
typedef union cpu_top 		ctop_t;

void		ctop_set(ctop_t *, uint32_t);
uint32_t	ctop_get(ctop_t *);
int			ctop_isset(ctop_t *, uint32_t);

/* ctop macros */
#define CPU_SET(top, val) 	(ctop_set(top, val))
#define CPU_GET(top) 		(ctop_get(top))
#define CPU_ISSET(top, val)	(ctop_isset(top, val))
#define CPU_EMPTY(top)		((top) == NULL)

extern struct bitlist_header bitset[];
extern int 					bitlist_counter;
extern ctop_t 				*ctop;

void		bitlist_init(void);
void		bitlist_insert(uint64_t);
bitlist_t 	*bitlist_search(uint64_t);
void		bitlist_remove(uint64_t);

#endif /* _SYS_BITLIST_H_ */
