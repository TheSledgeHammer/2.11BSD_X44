/*
 * vnode.h
 *
 *  Created on: 30 Jan 2022
 *      Author: marti
 */

#ifndef _SYS_VNODE_H_
#define _SYS_VNODE_H_
//#endif /* KERNEL */

/*
 * Flags for vdesc_flags:
 */
#define	VDESC_MAX_VPS		16
/* Low order 16 flag bits are reserved for willrele flags for vp arguments. */
#define VDESC_VP0_WILLRELE	0x0001
#define VDESC_VP1_WILLRELE	0x0002
#define VDESC_VP2_WILLRELE	0x0004
#define VDESC_VP3_WILLRELE	0x0008
#define VDESC_NOMAP_VPP		0x0100
#define VDESC_VPP_WILLRELE	0x0200

/*
 * VDESC_NO_OFFSET is used to identify the end of the offset list
 * and in places where no such field exists.
 */
#define VDESC_NO_OFFSET 	-1

/*
 * This structure describes the vnode operation taking place.
 */
struct vnodeop_desc {
	int		vdesc_offset;				/* offset in vector--first for speed */
	char    *vdesc_name;				/* a readable name for debugging */
	int		vdesc_flags;				/* VDESC_* flags */

	/*
	 * These ops are used by bypass routines to map and locate arguments.
	 * Creds and procs are not needed in bypass routines, but sometimes
	 * they are useful to (for example) transport layers.
	 * Nameidata is useful because it has a cred in it.
	 */
	int		*vdesc_vp_offsets;			/* list ended by VDESC_NO_OFFSET */
	int		vdesc_vpp_offset;			/* return vpp location */
	int		vdesc_cred_offset;			/* cred location, if any */
	int		vdesc_proc_offset;			/* proc location, if any */
	int		vdesc_componentname_offset; /* if any */
};

#ifdef _KERNEL
/*
 * A list of all the operation descs.
 */
extern struct vnodeop_desc *vnodeop_descs[];

/*
 * This macro is very helpful in defining those offsets in the vdesc struct.
 *
 * This is stolen from X11R4.  I ingored all the fancy stuff for
 * Crays, so if you decide to port this to such a serious machine,
 * you might want to consult Intrisics.h's XtOffset{,Of,To}.
 */
#define VOPARG_OFFSET(p_type,field) 				\
	((int) (((char *) (&(((p_type)NULL)->field))) - ((char *) NULL)))
#define VOPARG_OFFSETOF(s_type,field) 				\
	VOPARG_OFFSET(s_type*,field)
//#define	VOPARG_OFFSETOF(s_type, field)	            offsetof((s_type)*, field)
#define VOPARG_OFFSETTO(S_TYPE,S_OFFSET,STRUCT_P) 	\
	((S_TYPE)(((char*)(STRUCT_P))+(S_OFFSET)))


/*
 * A generic structure.
 * This can be used by bypass routines to identify generic arguments.
 */
struct vop_generic_args {
	struct vnodeops 	*a_ops;
	struct vnodeop_desc *a_desc;
	/* other random data follows, presumably */
};

#define VDESC(OP) 		(&(OP##_desc))
#define VOFFSET(OP) 	(VDESC(OP)->vdesc_offset)

/*
 * Finally, include the default set of vnode operations.
 */
#include <vnode_if.h>

/*
 * A default routine which just returns an error.
 */
int vn_default_error(void);
#endif /* _SYS_VNODE_H_ */
