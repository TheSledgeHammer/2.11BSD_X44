/*        $NetBSD: dm.h,v 1.56 2021/08/21 22:23:33 andvar Exp $      */

/*
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Adam Hamsik.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _DM_H_
#define _DM_H_

#include <sys/errno.h>
#include <sys/atomic.h>
#include <sys/mutex.h>
#include <sys/rwlock.h>
#include <sys/queue.h>
#include <sys/vnode.h>
#include <sys/device.h>
#include <sys/disk.h>
#include <sys/disklabel.h>

#include <miscfs/specfs/specdev.h> /* for v_rdev */

#include <advvm_fileset.h>
#include <advvm_var.h>

typedef struct dm_mapping 			dm_mapping_t;
typedef struct dm_table_entry 		dm_table_entry_t;
typedef struct dm_table_head		dm_table_head_t;
typedef struct dm_pdev				dm_pdev_t;
typedef struct dm_dev				dm_dev_t;
typedef struct dm_target			dm_target_t;
typedef struct dm_table 			dm_table_t;
typedef struct target_linear_config dm_target_linear_config_t;
typedef struct target_linear_devs 	dm_target_linear_devs_t;

struct dm_mapping {
	union {
		struct dm_pdev 			*pdev;
	} data;
	TAILQ_ENTRY(dm_mapping) 	next;
};

struct dm_table_entry {
	struct dm_dev 				*dm_dev;		/* backlink */
	uint64_t 					start;
	uint64_t 					length;

	struct dm_target 			*target;      	/* Link to table target. */
	void 						*target_config; /* Target specific data. */
	SLIST_ENTRY(dm_table_entry) next;

	TAILQ_HEAD(, dm_mapping) 	pdev_maps;
};

SLIST_HEAD(dm_table, dm_table_entry);


struct dm_table_head {
	/* Current active table is selected with this. */
	int 						cur_active_table;
	dm_table_t 					tables[2];

	struct mtx					*table_mtx;

	uint32_t 					io_cnt;
};

/*
 * This structure is used to store opened vnodes for disk with name.
 * I need this because devices can be opened only once, but I can
 * have more than one device on one partition.
 */
struct dm_pdev {
	char 						name[ADVVM_NAME_LEN];
	char 						udev_name[ADVVM_NAME_LEN];
	struct vnode 				*pdev_vnode;
	uint64_t 					pdev_numsec;
	unsigned int 				pdev_secsize;
	int 						ref_cnt; 			/* reference counter for users of this pdev */

	SLIST_ENTRY(dm_pdev) 		next_pdev;
};

/*
 * This structure is called for every device-mapper device.
 * It points to SLIST of device tables and mirrored, snapshoted etc. devices.
 */
TAILQ_HEAD(dm_dev_head, dm_dev);

struct dm_dev {
	advvm_fileset_t				fileset;
	char 						name[ADVVM_NAME_LEN];
	char						uuid[ADVVM_UUID_LEN];

	struct device				*devt;			 /* pointer to autoconf device structure */
	uint64_t 					minor; 			 /* Device minor number */
	uint32_t 					flags; 			 /* store communication protocol flags */

	uint32_t 					event_nr;
	uint32_t 					ref_cnt;
	dm_table_head_t 			table_head;

	 struct dkdevice			*diskp;

	TAILQ_ENTRY(dm_dev) 		next_devlist; 	/* Major device list. */
};

/* For linear target. */
struct target_linear_config {
	dm_pdev_t 					*pdev;
	uint64_t 					offset;
	TAILQ_ENTRY(target_linear_config) entries;
};

/*
 * Striping devices are stored in a linked list, this might be inefficient
 * for more than 8 striping devices and can be changed to something more
 * scalable.
 * TODO: look for other options than linked list.
 */
TAILQ_HEAD(target_linear_devs, target_linear_config);

/* constant dm_target structures for error, zero, linear, stripes etc. */
struct dm_target {
	char 						name[ADVVM_MAX_TYPE_NAME];
	/* Initialize target_config area */
	int 						(*init)(dm_table_entry_t *, int, char **);

	/* Destroy target_config area */
	int 						(*destroy)(dm_table_entry_t *);

	int 						(*strategy)(dm_table_entry_t *, struct buf *);
	//int (*upcall)(dm_table_entry_t *, struct buf *);

	/*
	 * Optional routines.
	 */
	/*
	 * Info/table routine are called to get params string, which is target
	 * specific. When dm_table_status_ioctl is called with flag
	 * DM_STATUS_TABLE_FLAG I have to sent params string back.
	 */
	char 						*(*info)(void *);
	char 						*(*table)(void *);
	int 						(*sync)(dm_table_entry_t *);
	int 						(*secsize)(dm_table_entry_t *, unsigned int *);

	uint32_t 					version[3];
	uint32_t 					ref_cnt;
	int 						max_argc;

	TAILQ_ENTRY(dm_target) 		dm_target_next;
};

/* device-mapper */
void dmgetproperties(struct disk *, dm_table_head_t *);

/* Generic function used to convert char to string */
uint64_t atoi64(const char *);

/* dm_target.c */
dm_target_t* dm_target_alloc(const char *);
dm_target_t* dm_target_autoload(const char *);
int dm_target_destroy(void);
int dm_target_insert(dm_target_t *);
//prop_array_t dm_target_prop_list(void);
dm_target_t* dm_target_lookup(const char *);
int dm_target_rem(const char *);
void dm_target_unbusy(dm_target_t *);
void dm_target_busy(dm_target_t *);
int dm_target_init(void);

#define DM_MAX_PARAMS_SIZE 	1024

/* dm_target_linear.c */
int dm_target_linear_init(dm_table_entry_t *, int, char **);
char *dm_target_linear_table(void *);
int dm_target_linear_strategy(dm_table_entry_t *, struct buf *);
int dm_target_linear_sync(dm_table_entry_t *);
int dm_target_linear_destroy(dm_table_entry_t *);
//int dm_target_linear_upcall(dm_table_entry_t *, struct buf *);
int dm_target_linear_secsize(dm_table_entry_t *, unsigned int *);

/* dm_target_error.c */
int dm_target_error_init(dm_table_entry_t*, int, char **);
int dm_target_error_strategy(dm_table_entry_t *, struct buf *);
int dm_target_error_destroy(dm_table_entry_t *);
//int dm_target_error_upcall(dm_table_entry_t *, struct buf *);

/* dm_target_zero.c */
int dm_target_zero_init(dm_table_entry_t *, int, char **);
int dm_target_zero_strategy(dm_table_entry_t *, struct buf *);
int dm_target_zero_destroy(dm_table_entry_t *);
//int dm_target_zero_upcall(dm_table_entry_t *, struct buf *);

/* dm_table.c  */
#define DM_TABLE_ACTIVE 	0
#define DM_TABLE_INACTIVE 	1

int dm_table_destroy(dm_table_head_t *, uint8_t);
uint64_t dm_table_size(dm_table_head_t *);
uint64_t dm_inactive_table_size(dm_table_head_t *);
void dm_table_disksize(dm_table_head_t *, uint64_t *, unsigned int *);
dm_table_t *dm_table_get_entry(dm_table_head_t *, uint8_t);
int dm_table_get_target_count(dm_table_head_t *, uint8_t);
void dm_table_release(dm_table_head_t *, uint8_t s);
void dm_table_switch_tables(dm_table_head_t *);
void dm_table_head_init(dm_table_head_t *);
void dm_table_head_destroy(dm_table_head_t *);
int dm_table_add_deps(dm_table_entry_t *, dm_pdev_t *);

/* dm_dev.c */
dm_dev_t* dm_dev_alloc(void);
void dm_dev_busy(dm_dev_t *);
int dm_dev_destroy(void);
dm_dev_t* dm_dev_detach(struct dkdevice *);
int dm_dev_free(dm_dev_t *);
int dm_dev_init(void);
int dm_dev_insert(dm_dev_t *);
dm_dev_t* dm_dev_lookup(const char *, const char *, int);
//prop_array_t dm_dev_prop_list(void);
dm_dev_t* dm_dev_rem(const char *, const char *, int);
/*int dm_dev_test_minor(int);*/
void dm_dev_unbusy(dm_dev_t *);

/* dm_pdev.c */
int dm_pdev_decr(dm_pdev_t *);
int dm_pdev_destroy(void);
int dm_pdev_init(void);
dm_pdev_t* dm_pdev_insert(const char *);
#endif /* _DM_H_ */
