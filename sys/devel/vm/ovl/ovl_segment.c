/*
 * vm_overlay.c
 *
 *  Created on: 18 Sep 2020
 *      Author: marti
 */

#include <sys/fnv_hash.h>
#include <ovl.h>

struct ovl_segment;
typedef struct ovl_segment 	*ovl_segment_t;

RB_HEAD(ovltree, ovl_segment);
struct ovl_segment {
	RB_ENTRY(ovl_segment)		os_entry;
	unsigned long           	os_hindex;		/* hash index */

	ovl_object_t				os_object;		/* which object am I in (O,P)*/
	vm_offset_t					os_offset;		/* offset into object (O,P) */
};

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

struct ovltree 		*ovl_segment_hashtree;
int					ovl_segment_bucket_count = 0;	/* How big is array? */
int					ovl_segment_hash_mask;			/* Mask for hash function */

void
ovl_segment_startup()
{

}

unsigned long
ovl_segment_hash(object, offset)
	ovl_object_t	object;
	vm_offset_t 	offset;
{
	Fnv32_t hash1 = fnv_32_buf(&object, (sizeof(&object) + atop(offset))&ovl_segment_hash_mask, FNV1_32_INIT)%ovl_segment_hash_mask;
	Fnv32_t hash2 = (((unsigned long)object+(unsigned long)atop(offset))&ovl_segment_hash_mask);
    return (hash1^hash2);
}

int
ovl_segment_rb_compare(seg1, seg2)
	ovl_segment_t seg1, seg2;
{
	if(seg1->os_hindex < seg2->os_hindex) {
		return(-1);
	} else if(seg1->os_hindex > seg2->os_hindex) {
		return(1);
	}
	return (0);
}

RB_PROTOTYPE(ovltree, ovl_segment, os_entry, ovl_segment_rb_compare);
RB_GENERATE(ovltree, ovl_segment, os_entry, ovl_segment_rb_compare);

void
ovl_segment_insert(seg, object, offset)
	register ovl_segment_t	seg;
	register ovl_object_t	object;
	register vm_offset_t	offset;
{
	seg->os_object = object;
	seg->os_offset = offset;

	register struct ovltree *bucket = &ovl_segment_hashtree[ovl_segment_hash(object, offset)];

	RB_INSERT(ovltree, bucket, seg);

	//seg->flags |= PG_TABLED;
}

void
ovl_segment_remove(seg)
	ovl_segment_t seg;
{
	register struct ovltree *bucket = &ovl_segment_hashtree[ovl_segment_hash(seg->os_object, seg->os_offset)];

	if(bucket) {
		RB_REMOVE(ovltree, bucket, seg);
	}
}

ovl_segment_t
ovl_segment_lookup(object, offset)
	register ovl_object_t	object;
	register vm_offset_t	offset;
{
	register ovl_segment_t	seg;
	register struct ovltree *bucket;

	bucket = &ovl_segment_hashtree[ovl_segment_hash(object, offset)];
	for(seg = RB_FIRST(ovltree, bucket); seg != NULL; seg = RB_NEXT(ovltree, root, seg)) {
		if(RB_FIND(ovltree, bucket, seg)->os_hindex == ovl_segment_hash(object, offset)) {
			if(RB_FIND(ovltree, bucket, seg)->os_object == object && RB_FIND(ovltree, bucket, seg)->os_offset == offset) {
				return (seg);
			}
		}
	}

	return (NULL);
}
