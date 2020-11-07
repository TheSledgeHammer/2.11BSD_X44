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

#include "ovl_overlay.h"
#include "ovl_segment.h"

/* flags */
#define OVL_LOADED 		0x001		/* overlay is loaded */
#define OVL_UNLOADED 	0x002		/* overlay is unloaded */
#define OVL_ACTIVE 		0x004		/* overlay is active */
#define OVL_INACTIVE 	0x008		/* overlay is inactive */
#define OVL_EXEC		0x010 		/* overlay is executing */

/* types */
#define OV_DFLT			-1
#define	OV_OBJECT		0
#define	OV_SEGMENT		1
#define OV_PAGE			2

/* overlay malloc types (to be place in more appropriate place i.e. malloc or tbtree) */
#define OVL_OBJECT		1	/* ovl object */
#define OVL_OBJHASH		2 	/* ovl object hash */
#define OVL_OVERLAYER	3 	/* overlayer */
#define OVL_OVOBJECT 	4 	/* overlaid object */
#define OVL_OVSEGMENT 	5 	/* overlaid segment  */
#define OVL_OVPAGE 		6 	/* overlaid page */
#define OVL_OVTABLE		7 	/* overlay table */

struct overlaylst;
CIRCLEQ_HEAD(overlaylst, overlay_struct);
struct overlay_struct {
	CIRCLEQ_ENTRY(overlay_struct) 	o_list;		/* list of overlays */

    int							    o_novl;		/* number of overlays */
	int								o_type;		/* type of overlay */
	int 							o_flags;	/* overlay flags */
};
typedef struct overlay_struct 		*ovl_overlay_t;

union overlay_generic {
    ovl_overlay_t                   *og_overlay;

    struct overlay_object           *og_object;
    struct overlay_segment          *og_segment;
    struct overlay_page             *og_page;
};

/* overlay table */
struct overlay_table {
    union overlay_generic 			o_private;	/* private data */

    ovl_map_t						o_mapper;	/* overlays map of ovlspace */
    vm_offset_t						o_start;	/* start of available space */
    vm_offset_t						o_end;		/* end of available space */
    u_long 							o_size;
	u_long							o_offset;
    u_long	                        o_area;
};

/* overlay object: holds object data */
struct overlay_object {
	ovl_object_t					oo_object;	/* overlaid object */
	ovl_overlay_t					oo_overlay;	/* overlaid object overlay */
	vm_object_t						oo_vobject;	/* overlaid vm_object */
};

/* overlay segment: holds segment data */
struct overlay_segment {
	ovl_segment_t 					os_segment;	/* overlaid segment */
	ovl_object_t					os_object;	/* overlaid object for this segment */
	vm_offset_t						os_offset;	/* overlaid segment offset */
	vm_segment_t					os_vsegment;/* overlaid vm_segment */
};

/* overlay page: holds page data */
struct overlay_page {
	ovl_page_t						op_page;	/* overlaid page */
	ovl_segment_t 					op_segment;	/* overlaid segment for this page */
	vm_offset_t						op_offset;	/* overlaid page offset */
	vm_page_t						op_vpage;	/* overlaid vm_segment */
};

/* overlay exec: holds executable data */
struct overlay_exec {

};

#endif /* SYS_OVERLAY_H_ */
