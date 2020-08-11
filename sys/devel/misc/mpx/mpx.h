/*
 * mpx.h
 *
 *  Created on: 16 May 2020
 *      Author: marti
 */

#ifndef _MPX_MPX_H_
#define _MPX_MPX_H_

struct mpx {

};

struct mpx_chan {

};

struct mpx_node {

	struct vnode			*mpx_vnode;
	struct mount			*mpx_mount;
};

#define	VTOMPX(vp) 	((struct mpx_node *)(vp)->v_data)
#define	MPXTOV(mpx) ((mpx)->mpx_vnode)

#endif /* SYS_DEVEL_MPX_MPX_H_ */
