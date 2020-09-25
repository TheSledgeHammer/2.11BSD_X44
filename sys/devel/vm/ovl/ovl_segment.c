/*
 * vm_overlay.c
 *
 *  Created on: 18 Sep 2020
 *      Author: marti
 */

#include <sys/fnv_hash.h>

#include <ovl.h>
#include <ovl_segment.h>

struct ovltree 		*ovl_segment_hashtree;
int					ovl_segment_bucket_count = 0;	/* How big is array? */
int					ovl_segment_hash_mask;			/* Mask for hash function */

void
ovl_segment_startup(start, end)
	vm_offset_t	*start;
	vm_offset_t	*end;
{
	register ovl_segment_t	seg;
	register struct ovltree	*bucket;
	vm_size_t				nsegments;
	int						i;
	extern	vm_offset_t		ovle_data;
	extern	vm_size_t		ovle_data_size;

	simple_lock_init(&ovl_segment_list_free_lock);
	//simple_lock_init(&ovl_segment_list_lock);

	TAILQ_INIT(&ovl_segment_list_free);
	TAILQ_INIT(&ovl_segment_list_active);
	TAILQ_INIT(&ovl_segment_list_inactive);

	if (ovl_segment_bucket_count == 0) {
		ovl_segment_bucket_count = 1;
		while (ovl_segment_bucket_count < (*end - *start))
			ovl_segment_bucket_count <<= 1;
	}

	ovl_segment_hash_mask = ovl_segment_bucket_count - 1;

	ovl_segment_hashtree = (struct ovltree *)(ovl_segment_bucket_count * sizeof(struct ovltree));
	bucket = ovl_segment_hashtree;

	for (i = ovl_segment_bucket_count; i--;) {
		RB_INIT(bucket);
		bucket++;
	}

	while (nsegments--) {
		TAILQ_INSERT_TAIL(&ovl_segment_list_free, seg, os_segq);
		seg++;
	}
}

unsigned long
ovl_segment_hash(object, offset)
	ovl_object_t	object;
	vm_offset_t 	offset;
{
	Fnv32_t hash1 = fnv_32_buf(&object, (sizeof(&object) + offset)&ovl_segment_hash_mask, FNV1_32_INIT)%ovl_segment_hash_mask;
	Fnv32_t hash2 = (((unsigned long)object+(unsigned long)offset)&ovl_segment_hash_mask);
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
	if (seg->os_flags & SEG_TABLED)
		panic("ovl_segment_insert: already inserted");

	seg->os_object = object;
	seg->os_offset = offset;

	register struct ovltree *bucket = &ovl_segment_hashtree[ovl_segment_hash(object, offset)];

	RB_INSERT(ovltree, bucket, seg);

	seg->os_flags |= SEG_TABLED;
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

ovl_segment_t
ovl_segment_alloc(object, offset)
	ovl_object_t	object;
	vm_offset_t		offset;
{
	register ovl_segment_t	seg;
	int		spl;

	spl = splimp();				/* XXX */
	simple_lock(&ovl_segment_list_free_lock);
	if(TAILQ_FIRST(ovl_segment_list_free) == NULL) {
		simple_unlock(&ovl_segment_list_free_lock);
		return (NULL);
	}
	seg = TAILQ_FIRST(ovl_segment_list_free);
	TAILQ_REMOVE(&ovl_segment_list_free, seg, os_segq);

	simple_unlock(&ovl_segment_list_free_lock);
	splx(spl);

	OVL_SEGMENT_INIT(seg, object, offset);

	return (seg);
}

void
ovl_segment_free(seg)
	register ovl_segment_t seg;
{
	if (seg->os_flags & SEG_ACTIVE) {
		TAILQ_REMOVE(&ovl_segment_list_active, seg, os_segq);
		seg->os_flags = SEG_ACTIVE;
	}

	if (seg->os_flags & SEG_INACTIVE) {
		TAILQ_REMOVE(&ovl_segment_list_inactive, seg, os_segq);
		seg->os_flags = SEG_INACTIVE;
	}
}

