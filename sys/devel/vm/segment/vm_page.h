/*
 * vm_page.h
 *
 *  Created on: 30 Sep 2020
 *      Author: marti
 */

#ifndef _VM_PAGE_H_
#define _VM_PAGE_H_

#include "tree.h"

struct pgtree;
RB_HEAD(pgtree, vm_page);
struct vm_page {
    RB_ENTRY(vm_page)		pg_pageq;		/* queue info for FIFO queue or free list (P) */
    RB_ENTRY(vm_page)       pg_hashq;
    RB_ENTRY(vm_page)		pg_listq;		/* pages in same object (O)*/

    vm_page_table_t         pg_pgtable;
    vm_offset_t				pg_offset;		/* offset into object (PT, O) */

    u_short					pg_wire_count;	/* wired down maps refs (P) */
    u_short					pg_flags;		/* see below */

    vm_offset_t				pg_phys_addr;	/* physical address of page */
};

extern
struct pgtree	            vm_page_queue_free;		/* memory free queue */
extern
struct pgtree	            vm_page_queue_active;	/* active memory queue */
extern
struct pgtree	            vm_page_queue_inactive;	/* inactive memory queue */

#endif /* _VM_PAGE_H_ */
