/*
 * ovl_pager.h
 *
 *  Created on: 12 Oct 2020
 *      Author: marti
 */

#ifndef OVL_PAGER_H_
#define OVL_PAGER_H_

struct ovlpager {
	int				ovp_flags;	/* ovl flags */
	vm_size_t		ovp_size;	/* ovl current size */
};
typedef struct ovlpager	*ovl_pager_t;

#endif /* OVL_PAGER_H_ */
