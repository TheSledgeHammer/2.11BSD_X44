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

/* Overlay Management Interface for kernel & vm */

#ifndef SYS_OVERLAY_H_
#define SYS_OVERLAY_H_

#include <sys/queue.h>
#include "ovl_overlay.h"

typedef enum loadoverlay {
	OVL_KERN,
	OVL_VM,
} OVLTYPE;

CIRCLEQ_HEAD(overlay_head, overlay);
struct overlay {
	CIRCLEQ_ENTRY(overlay) 	o_entries;
	OVLTYPE					o_type;
};

union overlay_generic {
	struct overlay_exec 	*ovl_exec;
	struct overlay_ops		*ovl_ops;

};

struct overlay_table {
	int 					o_type;
	u_long 					o_size;
	u_long					o_offset;
	u_long					o_area;

	union overlay_generic 	o_private;
};

struct overlay_exec {
	OVLTYPE					o_type;
	int 					o_ver;
	char					*o_name;
	u_long					o_offset;
	struct execsw			*o_exec;
};

/* overlay memory allocation ops: (see below: overlay_allocate & overlay_free)  */
struct overlay_ops {
	void (*koverlay_allocate)(unsigned long size, int type, int flags);
	void (*koverlay_free)(void *addr, int type);
	void (*voverlay_allocate)(unsigned long size, int type, int flags);
	void (*voverlay_free)(void *addr, int type);
};

struct overlay_load {
	caddr_t			ol_address;	/* overlayed address (kernel) */
	ovl_segment_t 	ol_segment;	/* overlayed segment (vm only!) */
	ovl_page_t 		ol_page;	/* overlayed page (vm only!) */

	//id, status
};

struct overlay_unload {
	caddr_t			ol_address;	/* overlayed address (kernel) */
	ovl_segment_t 	ol_segment;	/* overlayed segment (vm only!) */
	ovl_page_t 		ol_page;	/* overlayed page (vm only!) */

	//id, status
};

#endif /* SYS_OVERLAY_H_ */
