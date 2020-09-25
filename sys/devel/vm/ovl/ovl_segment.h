/*
 * ovl_segment.h
 *
 *  Created on: 25 Sep 2020
 *      Author: marti
 */

#ifndef _OVL_SEGMENT_H_
#define _OVL_SEGMENT_H_

#include <ovl.h>

struct kern_overlay {
	ovl_map_t 		ovl_map;
	ovl_map_entry_t ovl_entry;
	ovl_object_t 	ovl_object;

	vm_offset_t		phys_addr;
};

struct vm_overlay {
	ovl_map_t 		ovl_map;
	ovl_map_entry_t ovl_entry;
	ovl_object_t 	ovl_object;

	vm_offset_t		phys_addr;
};


struct ovl_segment;
typedef struct ovl_segment 	*ovl_segment_t;

RB_HEAD(ovltree, ovl_segment);
TAILQ_HEAD(ovllist, ovl_segment);
struct ovl_segment {
	RB_ENTRY(ovl_segment)		os_entry;				/* segment entries */
	TAILQ_ENTRY(ovl_segment)	os_segq;				/* segment list or free list */

	unsigned long           	os_hindex;				/* hash index */

	ovl_object_t				os_object;				/* which object am I in (O,P)*/
	vm_offset_t					os_offset;				/* offset into object (O,P) */

	u_short						os_flags;				/* see below */
};

/* segment flags */
#define	SEG_INACTIVE			0x0001					/* segment is in inactive list (P) */
#define	SEG_ACTIVE				0x0002					/* segment is in active list (P) */
#define	SEG_TABLED				0x0004					/* segment is in ovl tree (O) */
#define	SEG_CLEAN				0x0006					/* segment has not been modified */
#define	SEG_BUSY				0x0008					/* segment is in transit (O) */

extern
struct ovllist					ovl_segment_list_free;		/* memory free list */
extern
struct ovllist					ovl_segment_list_active;	/* active memory list */
extern
struct ovllist					ovl_segment_list_inactive;	/* inactive memory list */

extern
simple_lock_data_t				ovl_segment_list_free_lock; /* lock on free segment list */

#define	OVL_SEGMENT_INIT(seg, object, offset) { 		\
	(seg)->os_flags = SEG_BUSY | SEG_CLEAN; 			\
	ovl_segment_insert((seg), (object), (offset)); 		\

#endif /* _OVL_SEGMENT_H_ */
