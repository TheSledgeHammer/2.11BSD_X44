/*
 * mpx.h
 *
 *  Created on: 16 May 2020
 *      Author: marti
 */

#ifndef SYS_DEVEL_MPX_MPX_H_
#define SYS_DEVEL_MPX_MPX_H_

struct mpx {

};

struct mpx_chan {

};

struct mpx_node {
	LIST_ENTRY(mpx_node)  	mpx_hash;		/* Hash chain. */
	struct vnode			*mpx_vnode;

};

#define	VTOMPX(vp) 	((struct mpx_node *)(vp)->v_data)
#define	MPXTOV(mpx) ((mpx)->mpx_vnode)

#endif /* SYS_DEVEL_MPX_MPX_H_ */
