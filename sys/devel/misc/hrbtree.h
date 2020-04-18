/*
 * hrbtree.h
 *
 *  Created on: 18 Apr 2020
 *      Author: marti
 */

#ifndef SYS_DEVEL_MISC_HRBTREE_H_
#define SYS_DEVEL_MISC_HRBTREE_H_

#include <sys/tree.h>

#define NBUCKETS 32
extern struct hrbh hrbh_root_base[];
extern struct hrbe hrbe_entry_base[];

RB_HEAD(hrbtree, hrbe);
struct hrbh {
	struct hrbtree hrbh_root;
};

struct hrbe {
	RB_ENTRY(hrbe) hrbe_entry;
	u_long 		   hrbe_hash;
};

extern struct hrbh 	*hrbh_root(struct hrbh *head);
extern struct hrbe 	*hrbe_left(struct hrbe *entry);
extern struct hrbe 	*hrbe_right(struct hrbe *entry);
extern struct hrbe 	*hrbe_parent(struct hrbe *entry);
extern int 			hrbe_color(struct hrbe *entry);
extern void			hrbe_hash(struct hrbe *entry, uint32_t elm);

#define HRBT_ROOT(head) 		hrbh_root(head)
#define HRBT_LEFT(entry) 		hrbe_left(entry)
#define HRBT_RIGHT(entry) 		hrbe_right(entry)
#define HRBT_PARENT(entry) 		hrbe_parent(entry)
#define HRBT_COLOR(entry) 		hrbe_color(entry)
#define HRBT_HASH(entry, elm) 	hrbe_hash(entry, elm)



#endif /* SYS_DEVEL_MISC_HRBTREE_H_ */
