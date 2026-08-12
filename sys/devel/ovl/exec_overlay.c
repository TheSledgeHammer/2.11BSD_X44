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

int exec_aout_prep_zmagic(struct proc *, struct exec_linker *, int, int, int); /* ZMAGIC */
int exec_aout_prep_magic1(struct proc *, struct exec_linker *, int, int, int); /* OMAGIC */
int exec_aout_prep_magic2(struct proc *, struct exec_linker *, int, int, int); /* NMAGIC */
int exec_aout_prep_magic3(struct proc *, struct exec_linker *, int, int, int); /* Separated I&D */
int exec_aout_prep_magic4(struct proc *, struct exec_linker *, int, int, int); /* Overlay */
int exec_aout_prep_magic5(struct proc *, struct exec_linker *, int, int, int); /* Auto-Overlay (Non-Separate) */
int exec_aout_prep_magic6(struct proc *, struct exec_linker *, int, int, int); /* Auto-Overlay (Separate) */
int exec_aout_prep_common(struct proc *, struct exec_linker *, int, int, int);  /* Common function used for MAGIC3 through to MAGIC6 */

int
exec_aout_prep_overlay(p, elp)
	struct proc *p;
	struct exec_linker *elp;
{
	struct exec *a_out;
	int error, sep, overlay, ovflag;

	a_out = elp->el_image_hdr;
	overlay = sep = ovflag = 0;
	switch ((int)(a_out->a_magic & 0xffff)) {
	case ZMAGIC:
		error = exec_aout_prep_zmagic(p, elp, overlay, ovflag, sep);
		break;
	case A_MAGIC1:
		error = exec_aout_prep_magic1(p, elp, overlay, ovflag, sep);
		break;
	case A_MAGIC2:
		error = exec_aout_prep_magic2(p, elp, overlay, ovflag, sep);
		break;
	case A_MAGIC3:
		error = exec_aout_prep_magic3(p, elp, overlay, ovflag, sep);
		break;
	case A_MAGIC4:
		error = exec_aout_prep_magic4(p, elp, overlay, ovflag, sep);
		break;
	case A_MAGIC5:
		error = exec_aout_prep_magic5(p, elp, overlay, ovflag, sep);
		break;
	case A_MAGIC6:
		error = exec_aout_prep_magic6(elp, overlay, ovflag, sep);
		break;
	default:
		switch ((int)(ntohl(a_out->a_magic) & 0xffff)) {
		case ZMAGIC:
			error = exec_aout_prep_zmagic(p, elp, overlay, ovflag, sep);
			break;
		case A_MAGIC1:
			error = exec_aout_prep_magic1(p, elp, overlay, ovflag, sep);
			break;
		case A_MAGIC2:
			error = exec_aout_prep_magic2(p, elp, overlay, ovflag, sep);
			break;
		case A_MAGIC3:
			error = exec_aout_prep_magic3(p, elp, overlay, ovflag, sep);
			break;
		case A_MAGIC4:
			error = exec_aout_prep_magic4(p, elp, overlay, ovflag, sep);
			break;
		case A_MAGIC5:
			error = exec_aout_prep_magic5(p, elp, overlay, ovflag, sep);
			break;
		case A_MAGIC6:
			error = exec_aout_prep_magic6(p, elp, overlay, ovflag, sep);
			break;
		default:
			error = cpu_exec_aout_linker(p, elp); /* For CPU Architecture */
		}
	}
	if (error) {
		kill_vmcmds(&elp->el_vmcmds);
	}
	return (error);
}

int
exec_aout_prep_zmagic(p, elp, overlay, ovflag, sep)
	struct proc *p;
	struct exec_linker *elp;
	int overlay, ovflag, sep;
{
	struct exec *a_out = (struct exec *)elp->el_image_hdr;

	a_out->a_magic = ZMAGIC;
	u.u_error = exec_aout_prep_zmagic(p, elp);
	return (u.u_error);
}

int
exec_aout_prep_magic1(p, elp, overlay, ovflag, sep)
	struct proc *p;
	struct exec_linker *elp;
	int overlay, ovflag, sep;
{
	struct exec *a_out = (struct exec *)elp->el_image_hdr;

	a_out->a_magic = OMAGIC;
	u.u_error = exec_aout_prep_omagic(p, elp);
	return (u.u_error);
}

int
exec_aout_prep_magic2(p, elp, overlay, ovflag, sep)
	struct proc *p;
	struct exec_linker *elp;
	int overlay, ovflag, sep;
{
	struct exec *a_out = (struct exec *)elp->el_image_hdr;

	a_out->a_magic = NMAGIC;
	u.u_error = exec_aout_prep_nmagic(p, elp);
	return (u.u_error);
}

int
exec_aout_prep_magic3(p, elp, overlay, ovflag, sep)
	struct proc *p;
	struct exec_linker *elp;
	int overlay, ovflag, sep;
{
	sep++;
	u.u_error = exec_aout_prep_common(p, elp, ovflag, overlay, sep);
	return (u.u_error);
}

int
exec_aout_prep_magic4(p, elp, overlay, ovflag, sep)
	struct proc *p;
	struct exec_linker *elp;
	int overlay, ovflag, sep;
{
	overlay++;
	u.u_error = exec_aout_prep_common(p, elp, ovflag, overlay, sep);
	return (u.u_error);
}

int
exec_aout_prep_magic5(p, elp, overlay, ovflag, sep)
	struct proc *p;
	struct exec_linker *elp;
	int overlay, ovflag, sep;
{
	ovflag++;
	u.u_error = exec_aout_prep_common(p, elp, ovflag, overlay, sep);
	return (u.u_error);
}

int
exec_aout_prep_magic6(p, elp, overlay, ovflag, sep)
	struct proc *p;
	struct exec_linker *elp;
	int overlay, ovflag, sep;
{
	sep++;
	ovflag++;
	u.u_error = exec_aout_prep_common(p, elp, ovflag, overlay, sep);
	return (u.u_error);
}

/*
 * Read in and set up memory for executed file.
 * u.u_error set on error
 */
int
exec_aout_prep_common(p, elp, ovflag, overlay, sep)
	struct proc *p;
	struct exec_linker *elp;
	int ovflag, overlay, sep;
{
	struct exec *a_out = (struct exec *)elp->el_image_hdr;
	struct u_ovd sovdata;
	u_long ovhead[NOVL], ovoffset[NOVL];
	u_long ovbase, ovmax, curov, dbase, offset;
	long num;

	elp->el_taddr = USRTEXT;
	elp->el_tsize = a_out->a_text;
	elp->el_daddr = elp->el_taddr + a_out->a_text;
	elp->el_dsize = a_out->a_data + a_out->a_bss;
	elp->el_entry = a_out->a_entry;

	/*
	 * if auto overlay get second header
	 */
	sovdata = u.u_ovdata;
	u.u_ovdata.uo_ovbase = ovbase = 0;
	u.u_ovdata.uo_curov = curov = 0;
	if (ovflag) {
		/* set up for text */
		NEW_VMCMD(&elp->el_vmcmds, vmcmd_map_readvn, sizeof(ovhead),
			ovhead, (VM_PROT_READ | VM_PROT_EXECUTE),
			(VM_PROT_READ | VM_PROT_EXECUTE), elp->el_vnodep, sizeof(struct exec));
		if (vmcmds_proc_error(p, &elp->el_vmcmds) != 0) {
			u.u_ovdata = sovdata;
			return (ENOEXEC);
		}
		/* set beginning of overlay segment */
		ovbase = ctos(elp->el_tsize);
		/* 0th entry is max size of the overlays */
		ovmax = btoc(ovhead[0]);
		/* set max number of segm. registers to be used */
		num = ctos(ovmax);
		/* set base of data space */
		dbase = stoc(ovbase + num);
		/*
		 * Set up a table of offsets to each of the overlay
		 * segements. The ith overlay runs from ov_offst[i-1]
		 * to ov_offst[i].
		 */
		ovoffset[0] = elp->el_tsize;

		u.u_ovdata.uo_ovbase = ovbase;
		u.u_ovdata.uo_nseg = num;
		u.u_ovdata.uo_dbase = dbase;
		u.u_ovdata.uo_ov_offst[0] = ovoffset[0];
		{
			int i;
			u_long t;

			/* check if any overlay is larger than ovmax */
			for (i = 1; i <= NOVL; i++) {
				t = btoc(ovhead[i]);
				if (t > ovmax) {
					u.u_ovdata = sovdata;
					return (ENOEXEC);
				}
				ovoffset[i] = t + ovoffset[i - 1];
				u.u_ovdata.uo_ov_offst[i] = ovoffset[i];
			}
		}
	}
	if (overlay) {
		if ((sep == 0 && ctos(elp->el_tsize) != ctos(u.u_tsize)) || elp->el_argc) {
			return (ENOMEM);
		}
		elp->el_dsize = u.u_dsize;
		elp->el_ssize = u.u_ssize;
		sep = u.u_sep;
		vm_xfree();
		vm_xalloc(p, elp->el_tsize, sizeof(struct exec));
		//u.u_ar0[PC] = a_out->a_entry & ~01;
	} else {
		if (vm_estabur(p, elp->el_tsize, elp->el_dsize, elp->el_ssize, sep, SEG_RO)) {
			u.u_ovdata = sovdata;
			goto out;
		}

		/*
		 * allocate and clear core at this point, committed
		 * to the new image
		 */
		u.u_prof.pr_scale = 0;
		if (p->p_flag & P_SVFORK) {
			endvfork();
		} else {
			xfree();
		}

		vm_expand(elp->el_dsize, S_DATA);
		{
			long bsize, baddr;

			/* set up for bss segment */
			baddr = roundup(elp->el_daddr + a_out->a_data, NBPG);
			bsize = elp->el_daddr + elp->el_dsize - baddr;
			if (bsize > 0) {
				NEW_VMCMD(&elp->el_vmcmds, vmcmd_map_zero, bsize, baddr,
					(VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE),
					(VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE), NULL, 0);
			}
			bzero(p->p_daddr + baddr, bsize);
		}
		vm_expand(elp->el_ssize, S_STACK);
		bzero(p->p_saddr, elp->el_ssize);
		vm_xalloc(ip, elp->el_tsize, sizeof(struct exec));

		/*
		 * read in data segment
		 */
		(void)vm_estabur(p, 0, elp->el_dsize, 0, 0, SEG_RO);

		offset = sizeof(struct exec);
		if (ovflag) {
			offset += sizeof(ovhead);
			offset += ((u.u_ovdata.uo_ov_offst[NOVL]) << 6);
		} else {
			offset += a_out->a_text;
		}

		/* set up for data segment */
		NEW_VMCMD(&elp->el_vmcmds, vmcmd_map_readvn, a_out->a_data,
			elp->el_daddr, (VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE),
			(VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE), elp->el_vnodep,
			offset);
	}

	u.u_tsize = elp->el_tsize;
	u.u_dsize = elp->el_dsize;
	u.u_ssize = elp->el_ssize;
	u.u_sep = sep;
	(void)vm_estabur(p, elp->el_tsize, elp->el_dsize, elp->el_ssize, sep, SEG_RO);

out:
	return (*elp->el_esch->ex_setup_stack)(elp);
}


struct exec_ovdata {
	u_long eo_ovbase;
	u_long eo_curov;
	u_long eo_dbase;
	u_long eo_ov_offset[NOVL];
	u_long eo_ovmax;
	long eo_nseg;

	u_long eo_ovhead[NOVL];
};

int
exec_setup_ovdata(elp, ovflag)
	struct exec_linker *elp;
	int ovflag;
{
	struct u_ovd sovdata;
	u_long ovhead[NOVL], ovoffset[NOVL];
	u_long ovbase, ovmax, curov, dbase;
	long num;
	int error;

	sovdata = u.u_ovdata;
	ovbase = 0;
	curov = 0;
	if (ovflag) {
		ovhead = kmem_alloc_wait(exec_map, sizeof(ovhead));
		/* set beginning of overlay segment */
		ovbase = ctos(elp->el_tsize);
		/* 0th entry is max size of the overlays */
		ovmax = btoc(ovhead[0]);
		/* set max number of segm. registers to be used */
		num = ctos(ovmax);
		/* set base of data space */
		dbase = stoc(ovbase + num);
		/*
		 * Set up a table of offsets to each of the overlay
		 * segements. The ith overlay runs from ov_offst[i-1]
		 * to ov_offst[i].
		 */
		ovoffset[0] = elp->el_tsize;

		u.u_ovdata.uo_ovbase = ovbase;
		u.u_ovdata.uo_nseg = num;
		u.u_ovdata.uo_dbase = dbase;
		u.u_ovdata.uo_ov_offst[0] = ovoffset[0];

		{
			int i, t;

			/* check if any overlay is larger than ovmax */
			for (i = 1; i <= NOVL; i++) {
				t = btoc(ovhead[i]);
				if (t > ovmax) {
					error = ENOEXEC;
					u.u_ovdata = sovdata;
					u.u_error = error;
					return (error);
				}
				ovoffset[i] = t + ovoffset[i - 1];
				u.u_ovdata.uo_ov_offst[i] = ovoffset[i];
			}
		}
	} else {
		u.u_ovdata.uo_ovbase = ovbase;
		u.u_ovdata.uo_curov = curov;
	}
	return (0);
}
