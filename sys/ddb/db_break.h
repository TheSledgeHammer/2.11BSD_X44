/*	$NetBSD: db_break.h,v 1.11.6.2 1999/04/12 21:27:07 pk Exp $	*/

/* 
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 *
 *	Author: David B. Golub, Carnegie Mellon University
 *	Date:	7/90
 */

#ifndef	_DDB_DB_BREAK_H_
#define	_DDB_DB_BREAK_H_

#include <vm/include/vm.h>
#include <vm/include/vm_extern.h>

/*
 * Breakpoints.
 */
typedef struct db_breakpoint {
	vm_map_t map;			/* in this map */
	db_addr_t address;		/* set here */
	int	init_count;		/* number of times to skip bkpt */
	int	count;			/* current count */
	int	flags;			/* flags: */
#define	BKPT_SINGLE_STEP	0x2	    /* to simulate single step */
#define	BKPT_TEMP		0x4	    /* temporary */
	db_expr_t bkpt_inst;		/* saved instruction at bkpt */
	struct db_breakpoint *link;	/* link in in-use or free chain */
} *db_breakpoint_t;

db_breakpoint_t db_breakpoint_alloc(void);
void 			db_breakpoint_free(db_breakpoint_t);
void 			db_set_breakpoint(vm_map_t, db_addr_t, int);
void 			db_delete_breakpoint(vm_map_t, db_addr_t);
db_breakpoint_t db_find_breakpoint(vm_map_t, db_addr_t);
db_breakpoint_t db_find_breakpoint_here(db_addr_t);
void 			db_set_breakpoints(void);
void 			db_clear_breakpoints(void);
void 			db_list_breakpoints(void);
void 			db_delete_cmd(db_expr_t, int, db_expr_t, char *);
void 			db_breakpoint_cmd(db_expr_t, int, db_expr_t, char *);
void 			db_listbreak_cmd(db_expr_t, int, db_expr_t, char *);
boolean_t 		db_map_equal(vm_map_t, vm_map_t);
boolean_t 		db_map_current(vm_map_t);
vm_map_t 		db_map_addr(vm_offset_t);

#endif	/* _DDB_DB_BREAK_H_ */
