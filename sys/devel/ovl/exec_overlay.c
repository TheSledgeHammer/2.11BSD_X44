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

/*
 * TODO:
 * - setup vmcmds for overlay space access
 */

int exec_aout_prep_zmagic(struct exec_linker *, int, int, int);	/* ZMAGIC */
int exec_aout_prep_magic1(struct exec_linker *, int, int, int);	/* OMAGIC */
int exec_aout_prep_magic2(struct exec_linker *, int, int, int); /* NMAGIC */
int exec_aout_prep_magic3(struct exec_linker *, int, int, int); /* Separated I&D */
int exec_aout_prep_magic4(struct exec_linker *, int, int, int); /* Overlay */
int exec_aout_prep_magic5(struct exec_linker *, int, int, int); /* Auto-Overlay (Non-Separate) */
int exec_aout_prep_magic6(struct exec_linker *, int, int, int); /* Auto-Overlay (Separate) */
void getxfile(struct exec_linker *, u_long, int, int, int);

int
exec_aout_prep_overlay(elp)
	struct exec_linker *elp;
{
	struct exec *a_out;
	int error, sep, overlay, ovflag;

	a_out = elp->el_image_hdr;
	overlay = sep = ovflag = 0;
	switch ((int)(a_out->a_magic & 0xffff)) {
	case ZMAGIC:
		error = exec_aout_prep_zmagic(elp, overlay, ovflag, sep);
		break;
	case A_MAGIC1:
		error = exec_aout_prep_magic1(elp, overlay, ovflag, sep);
		break;
	case A_MAGIC2:
		error = exec_aout_prep_magic2(elp, overlay, ovflag, sep);
		break;
	case A_MAGIC3:
		error = exec_aout_prep_magic3(elp, overlay, ovflag, sep);
		break;
	case A_MAGIC4:
		error = exec_aout_prep_magic4(elp, overlay, ovflag, sep);
		break;
	case A_MAGIC5:
		error = exec_aout_prep_magic5(elp, overlay, ovflag, sep);
		break;
	case A_MAGIC6:
		error = exec_aout_prep_magic6(elp, overlay, ovflag, sep);
		break;
	default:
		switch ((int)(ntohl(a_out->a_magic) & 0xffff)) {
		case ZMAGIC:
			error = exec_aout_prep_zmagic(elp, overlay, ovflag, sep);
			break;
		case A_MAGIC1:
			error = exec_aout_prep_magic1(elp, overlay, ovflag, sep);
			break;
		case A_MAGIC2:
			error = exec_aout_prep_magic2(elp, overlay, ovflag, sep);
			break;
		case A_MAGIC3:
			error = exec_aout_prep_magic3(elp, overlay, ovflag, sep);
			break;
		case A_MAGIC4:
			error = exec_aout_prep_magic4(elp, overlay, ovflag, sep);
			break;
		case A_MAGIC5:
			error = exec_aout_prep_magic5(elp, overlay, ovflag, sep);
			break;
		case A_MAGIC6:
			error = exec_aout_prep_magic6(elp, overlay, ovflag, sep);
			break;
		default:
			error = cpu_exec_aout_linker(elp); /* For CPU Architecture */
		}
	}
	if (error) {
		kill_vmcmds(&elp->el_vmcmds);
	}
	return (error);
}

int
exec_aout_prep_zmagic(elp, overlay, ovflag, sep)
	struct exec_linker *elp;
	int overlay, ovflag, sep;
{
	struct exec *a_out;

	a_out = elp->el_image_hdr;
	a_out->a_magic = ZMAGIC;
	return (exec_aout_prep_zmagic(elp));
}

int
exec_aout_prep_magic1(elp, overlay, ovflag, sep)
	struct exec_linker *elp;
	int overlay, ovflag, sep;
{
	struct exec *a_out;

	a_out = elp->el_image_hdr;
	a_out->a_magic = OMAGIC;
	return (exec_aout_prep_omagic(elp));
}

int
exec_aout_prep_magic2(elp, overlay, ovflag, sep)
	struct exec_linker *elp;
	int overlay, ovflag, sep;
{
	struct exec *a_out;

	a_out = elp->el_image_hdr;
	a_out->a_magic = NMAGIC;
	return (exec_aout_prep_nmagic(elp));
}

int
exec_aout_prep_magic3(elp, overlay, ovflag, sep)
	struct exec_linker *elp;
	int overlay, ovflag, sep;
{
	sep++;
	getxfile(elp, A_MAGIC3, overlay, ovflag, sep);
	return (*elp->el_esch->ex_setup_stack)(elp);
}

int
exec_aout_prep_magic4(elp, overlay, ovflag, sep)
	struct exec_linker *elp;
	int overlay, ovflag, sep;
{
	overlay++;
	getxfile(elp, A_MAGIC4, overlay, ovflag, sep);
	return (*elp->el_esch->ex_setup_stack)(elp);
}

int
exec_aout_prep_magic5(elp, overlay, ovflag, sep)
	struct exec_linker *elp;
	int overlay, ovflag, sep;
{
	ovflag++;
	getxfile(elp, A_MAGIC5, overlay, ovflag, sep);
	return (*elp->el_esch->ex_setup_stack)(elp);
}

int
exec_aout_prep_magic6(elp, overlay, ovflag, sep)
	struct exec_linker *elp;
	int overlay, ovflag, sep;
{
	sep++;
	ovflag++;
	getxfile(elp, A_MAGIC6, overlay, ovflag, sep);
	return (*elp->el_esch->ex_setup_stack)(elp);
}

/*
 * Read in and set up memory for executed file.
 * u.u_error set on error
 */
void
getxfile(elp, a_magic, overlay, ovflag, sep)
	struct exec_linker *elp;
	u_long a_magic;
	int overlay, ovflag, sep;
{
	struct exec *a_out;
	struct vnode *vp;
	struct u_ovd sovdata;
	u_long lsize;
	off_t offset;
	u_int ds, ts, ss;
	u_int ovhead[NOVL + 1];
	int ovmax;//, resid;

	a_out = elp->el_image_hdr;
	elp->el_taddr = USRTEXT;
	elp->el_tsize = a_out->a_text;
	elp->el_daddr = elp->el_taddr + a_out->a_text;
	elp->el_dsize = a_out->a_data + a_out->a_bss;
	elp->el_entry = a_out->a_entry;

	ts = btoc(a_out->a_text);
	ds = btoc(elp->el_dsize);
	ss = SSIZE + btoc(elp->el_argc);

	/*
	 * if auto overlay get second header
	 */
	sovdata = u.u_ovdata;
	u.u_ovdata.uo_ovbase = 0;
	u.u_ovdata.uo_curov = 0;

	if (ovflag) {
	//	u.u_error = rdwri(UIO_READ, ip, ovhead, sizeof(ovhead), (off_t)sizeof(struct exec), UIO_SYSSPACE, IO_UNIT, &resid);
		NEW_VMCMD(&elp->el_vmcmds, vmcmd_map_readvn, ovhead, sizeof(ovhead),
				(VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE),
				(VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE),
				elp->el_vnodep, sizeof(struct exec));
		/*
		if (resid != 0) {
			u->u_error = ENOEXEC;
		}
		*/
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
		if (((u.u_sep == 0) && (ctos(ts) != ctos(u.u_tsize))) || elp->el_argc) {
			u.u_error = ENOMEM;
			return;
		}
		ds = u.u_dsize;
		ss = u.u_ssize;
		sep = u.u_sep;
		vm_xfree();
		vm_xalloc(elp->el_vnodep, elp->el_tsize, sizeof(struct exec_linker));
		//u.u_ar0[PC] = ep->a_entry & ~01;
	} else {
		if (vm_estabur(elp->el_proc, ds, ss, ts, sep, SEG_RO)) {
			u.u_ovdata = sovdata;
			return;
		}

		/*
		 * allocate and clear core at this point, committed
		 * to the new image
		 */
		u.u_prof.pr_scale = 0;
		if (elp->el_proc->p_flag & P_SVFORK) {
			endvfork();
		} else {
			vm_xfree();
		}
		vm_expand(elp->el_proc, ds, S_DATA);
		{
			register u_int numc, startc;

			startc = btoc(a_out->a_data); /* clear BSS only */
			if (startc != 0) {
				startc--;
			}
			numc = ds - startc;
			bzero(elp->el_proc->p_daddr + startc, numc);
		}
		vm_expand(elp->el_proc, ss, S_STACK);
		bzero(elp->el_proc->p_saddr, ss);
		vm_xalloc(elp->el_vnodep, elp->el_tsize, sizeof(struct exec_linker));

		/*
		 * read in data segment
		 */
		vm_estabur(elp->el_proc, ds, 0, 0, 0, SEG_RO);
		offset = sizeof(struct exec);
		if (ovflag) {
			offset += sizeof(ovhead);
			offset += (((long)u.u_ovdata.uo_ov_offst[NOVL]) << 6);
		} else {
			offset += a_out->a_text;
		}
		NEW_VMCMD(&elp->el_vmcmds, vmcmd_map_readvn, a_out->a_data,
				elp->el_daddr, (VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE),
				(VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE),
				elp->el_vnodep, offset);
#ifdef notyet
		/*
		 * set SUID/SGID protections, if no tracing
		 */

		if ((u.u_procp->p_flag & P_TRACED)==0) {
			u.u_uid = uid;
			u.u_procp->p_uid = uid;
			u.u_groups[0] = gid;
		} else {
			psignal(u.u_procp, SIGTRAP);
		}
		u.u_svuid = u.u_uid;
		u.u_svgid = u.u_groups[0];
		u.u_acflag &= ~ASUGID;	/* start fresh setuid/gid priv use */
#endif
	}
	u.u_procp = elp->el_proc;
	u.u_tsize = ts;
	u.u_dsize = ds;
	u.u_ssize = ss;
	u.u_sep = sep;
	vm_estabur(elp->el_proc, ds, ss, ts, sep, SEG_RO);
}
