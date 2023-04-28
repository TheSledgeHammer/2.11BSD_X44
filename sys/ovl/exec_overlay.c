/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)kern_exec.c	1.8 (2.11BSD) 1999/9/6
 */

/* overlay exec testing */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/mman.h>
#include <sys/exec.h>
#include <sys/exec_linker.h>
#include <sys/exec_aout.h>
#include <sys/resourcevar.h>

exec_aout_prep_(elp)
	struct exec_linker *elp;
{
	struct exec *a_out = elp->el_image_hdr;
	int error;

	switch((int)(a_out->a_magic & 0xffff)) {
	case A_MAGIC3:
	case A_MAGIC4:
	case A_MAGIC5:
	case A_MAGIC6:
		break;
	default:

	}
}

getxfile(elp)
	struct exec_linker *elp;
{
	struct exec *a_out = elp->el_image_hdr;
	struct u_ovd sovdata;

	u_int ds, ts, ss;
	u_int ovhead[NOVL + 1];
	int sep, overlay, ovflag, ovmax, resid;

	switch(a_out->a_magic) {
	case A_MAGIC3:
		sep++;
		break;
	case A_MAGIC4:
		overlay++;
		break;
	case A_MAGIC5:
		ovflag++;
		break;
	case A_MAGIC6:
		sep++;
		ovflag++;
		break;
	}

	struct vmspace *vmspace = elp->el_proc->p_vmspace;
	long dsize, bsize, baddr;

	elp->el_taddr = USRTEXT;
	elp->el_tsize = a_out->a_text;
	elp->el_daddr = elp->el_taddr + a_out->a_text;
	elp->el_dsize = a_out->a_data + a_out->a_bss;
	elp->el_entry = a_out->a_entry;

	/*
	 * if auto overlay get second header
	 */
	sovdata = u.u_ovdata;
	u.u_ovdata.uo_ovbase = 0;
	u.u_ovdata.uo_curov = 0;

	if (ovflag) {
		if (resid != 0)
			u->u_error = ENOEXEC;
		if (u->u_error) {
			u->u_ovdata = sovdata;
			return;
		}

		/* set beginning of overlay segment */
		u->u_ovdata.uo_ovbase = ctos(elp->el_tsize);

		/* 0th entry is max size of the overlays */
		ovmax = btoc(ovhead[0]);

		/* set max number of segm. registers to be used */
		u->u_ovdata.uo_nseg = ctos(ovmax);

		/* set base of data space */
		u->u_ovdata.uo_dbase = stoc(u->u_ovdata.uo_ovbase + u->u_ovdata.uo_nseg);

		/*
		 * Set up a table of offsets to each of the overlay
		 * segements. The ith overlay runs from ov_offst[i-1]
		 * to ov_offst[i].
		 */
		u->u_ovdata.uo_ov_offst[0] = elp->el_tsize;
		{
			register int t, i;

			/* check if any overlay is larger than ovmax */
			for (i = 1; i <= NOVL; i++) {
				if ((t = btoc(ovhead[i])) > ovmax) {
					u->u_error = ENOEXEC;
					u->u_ovdata = sovdata;
					return;
				}
				u->u_ovdata.uo_ov_offst[i] = t + u->u_ovdata.uo_ov_offst[i - 1];
			}
		}
	}
	if (overlay) {
		vm_xfree();
		vm_xalloc1(vp, elp);
	} else {
		if (vm_estabur()) {

		}

		vm_expand();

	}

	vm_estabur();
}


void
vm_xalloc1(vp, elp)
	struct vnode 		*vp;
	struct exec_linker 	*elp;
{
	u_long tsize;
	off_t toff;

	tsize = elp->el_tsize;
	toff = sizeof(struct exec_linker);

	vm_xalloc(vp, tsize, toff);
}
