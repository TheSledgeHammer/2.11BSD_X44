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

/* Overlay Object Type */
typedef enum {
	OVL_KERN,
	OVL_VM
} OVL_OBJ_TYPE;

extern struct overlay_head ovlist;

/* flags */
#define OVL_LOADED 		0x001		/* overlay is loaded */
#define OVL_UNLOADED 	0x002		/* overlay is unloaded */
#define OVL_ACTIVE 		0x004		/* overlay is active */
#define OVL_INACTIVE 	0x008		/* overlay is inactive */
#define OVL_EXEC		0x010 		/* overlay is executing */


struct overlay_head;
CIRCLEQ_HEAD(overlay_head, overlay_entries);
struct overlay_entries {
	CIRCLEQ_ENTRY(overlay_entries) 	o_entries;

	int								o_nkovl;
	int								o_nvovl;
};

union overlay_generic {
	struct overlay_entries			*ovl_list;		/* list of loaded overlay objects */

	struct overlay_exec 			*ovl_exec;		/* overlay executable initializer */
	struct overlay_ops				*ovl_ops;		/* overlay ops (vm & kernel) */
	struct overlay_load				*ovl_load;		/* load into overlay list */
	struct overlay_unload			*ovl_unload;	/* unload from overlay list */
};

struct overlay_table {
	int 							o_objtype;		/* overlay object type: (see above: OVL_OBJ_TYPE) */
	u_long 							o_size;
	u_long							o_offset;
	u_long							o_area;
	u_long							o_used;

	union overlay_generic 			o_private;
};

struct overlay_exec {
	int 							o_ver;
	char							*o_name;
	u_long							o_offset;
	struct execsw					*o_exec;
};

/* overlay ops */
struct overlay_ops {
	void (*koverlay_allocate)(unsigned long size, int type, int flags);
	void (*koverlay_free)(void *addr, int type);
	void (*voverlay_allocate)(unsigned long size, int type, int flags);
	void (*voverlay_free)(void *addr, int type);
};

struct overlay_load {
	caddr_t							ol_address;	/* overlaid address */

	ovl_object_t					ol_object;	/* overlaid object */
	ovl_segment_t 					ol_segment;	/* overlaid segment */
	ovl_page_t 						ol_page;	/* overlaid page */

	//id, status
};

struct overlay_unload {
	caddr_t							ol_address;	/* overlaid address */

	ovl_object_t					ol_object;	/* overlaid object */
	ovl_segment_t 					ol_segment;	/* overlaid segment */
	ovl_page_t 						ol_page;	/* overlaid page */

	//id, status
};

#endif /* SYS_OVERLAY_H_ */
