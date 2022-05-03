/*
 * Copyright (c) 1992, 1993, 1994, 1995
 *	The Regents of the University of California.  All rights reserved.
 *
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
#	This product includes software developed by the University of
#	California, Berkeley and its contributors.
# 4. Neither the name of the University nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
 *
 * from: NetBSD: vnode_if.sh,v 1.7 1994/08/25 03:04:28 cgd Exp $
 */

#ifndef _SYS_VNODEOPV_H_
#define _SYS_VNODEOPV_H_

#include <sys/buf.h>
#include <sys/stddef.h>

/* TODO:
 * - add: default vnodeop_desc
 * - re-add: vp_offsets to each vnodeop_desc
 * - re-design: vnodeopv_entry_desc to register each vnodeops operation (int (*opve_op)(void *))
 *  	with its vnodeop_desc
 * - change: vnodeopv_desc to work with vnodeopv_entry_desc changes
 *
 */
//#include <sys/vnode.h>

typedef int (*opve_impl_t)(void *);
typedef int (***opv_desc_vector_t)(void *);

struct vnodeopv_entry_desc {
	struct vnodeop_desc 		*opve_op;  			/* which operation this is */
	opve_impl_t					opve_impl;			/* code implementing this operation */
};

struct vnodeopv_desc_list;
LIST_HEAD(vnodeopv_desc_list, vnodeopv_desc);
struct vnodeopv_desc {
	LIST_ENTRY(vnodeopv_desc)	opv_entry;
    const char                  *opv_fsname;
    int                         opv_voptype;
    opv_desc_vector_t			opv_desc_vector_p;	/* ptr to the ptr to the vector where op should go */
	struct vnodeopv_entry_desc 	opv_desc_ops;   	/* null terminated list */
};

/* vnodeops voptype */
#define D_NOOPS  	0   /* vops not set */
#define D_VNODEOPS  1   /* vops vnodeops */
#define D_SPECOPS   2   /* vops specops */
#define D_FIFOOPS   3   /* vops fifoops */

void 						vnodeopv_desc_create(struct vnodeopv_desc *, const char *, int, struct vnodeops *, struct vnodeop_desc *);
struct vnodeopv_desc 		*vnodeopv_desc_lookup(struct vnodeopv_desc *, const char *, int);
union vnodeopv_entry_desc	vnodeopv_entry_desc(struct vnodeopv_desc *, const char *, int);
struct vnodeops 			*vnodeopv_entry_desc_get_vnodeops(struct vnodeopv_desc *, const char *, int);
struct vnodeop_desc 		*vnodeopv_entry_desc_get_vnodeop_desc(struct vnodeopv_desc *, const char *, int);

#define VNODEOPV_DESC_NAME(name, voptype)         name##_##voptype##_opv_desc
#define VNODEOPV_DESC_STRUCT(name, voptype) \
		struct vnodeopv_desc VNODEOPV_DESC_NAME(name, voptype);

extern struct vnodeopv_desc_list vfs_opv_descs;

#endif /* _SYS_VNODEOPV_H_ */
