/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)kern_exec.c	1.8 (2.11BSD) 1999/9/6
 */

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
 * Notes:
 * To try and maintain some stability and consistency from 2.11BSD, whose
 * a.out format is most similar to OMAGIC. Hence a.out MAGIC3 through to MAGIC6
 * are also setup based around OMAGIC.
 */

int exec_aout_prep_zmagic(struct proc *, struct exec_linker *); /* ZMAGIC */
int exec_aout_prep_magic1(struct proc *, struct exec_linker *); /* OMAGIC */
int exec_aout_prep_magic2(struct proc *, struct exec_linker *); /* NMAGIC */
int exec_aout_prep_magic3(struct proc *, struct exec_linker *); /* Separated I&D */
int exec_aout_prep_magic4(struct proc *, struct exec_linker *); /* Overlay */
int exec_aout_prep_magic5(struct proc *, struct exec_linker *); /* Auto-Overlay (Non-Separate) */
int exec_aout_prep_magic6(struct proc *, struct exec_linker *); /* Auto-Overlay (Separate) */
int exec_aout_prep_common(struct proc *, struct exec_linker *, struct exec_ovdata *, struct exec *);  /* Common function used for MAGIC3 through to MAGIC6 */

int
exec_aout_prep_overlay(p, elp)
	struct proc *p;
	struct exec_linker *elp;
{
	struct exec *a_out;
	int error;

	a_out = elp->el_image_hdr;
	switch ((int)(a_out->a_magic & 0xffff)) {
	case ZMAGIC:
		error = exec_aout_prep_zmagic(p, elp);
		break;
	case A_MAGIC1:
		error = exec_aout_prep_magic1(p, elp);
		break;
	case A_MAGIC2:
		error = exec_aout_prep_magic2(p, elp);
		break;
	case A_MAGIC3:
		error = exec_aout_prep_magic3(p, elp);
		break;
	case A_MAGIC4:
		error = exec_aout_prep_magic4(p, elp);
		break;
	case A_MAGIC5:
		error = exec_aout_prep_magic5(p, elp);
		break;
	case A_MAGIC6:
		error = exec_aout_prep_magic6(p, elp);
		break;
	default:
		switch ((int)(ntohl(a_out->a_magic) & 0xffff)) {
		case ZMAGIC:
			error = exec_aout_prep_zmagic(p, elp);
			break;
		case A_MAGIC1:
			error = exec_aout_prep_magic1(p, elp);
			break;
		case A_MAGIC2:
			error = exec_aout_prep_magic2(p, elp);
			break;
		case A_MAGIC3:
			error = exec_aout_prep_magic3(p, elp);
			break;
		case A_MAGIC4:
			error = exec_aout_prep_magic4(p, elp);
			break;
		case A_MAGIC5:
			error = exec_aout_prep_magic5(p, elp);
			break;
		case A_MAGIC6:
			error = exec_aout_prep_magic6(p, elp);
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
exec_aout_prep_zmagic(p, elp)
	struct proc *p;
	struct exec_linker *elp;
{
	struct exec *a_out = (struct exec *)elp->el_image_hdr;

	u.u_error = exec_aout_prep_zmagic(p, elp);
	return (u.u_error);
}

int
exec_aout_prep_magic1(p, elp)
	struct proc *p;
	struct exec_linker *elp;
{
	struct exec *a_out = (struct exec *)elp->el_image_hdr;

	u.u_error = exec_aout_prep_omagic(p, elp);
	return (u.u_error);
}

int
exec_aout_prep_magic2(p, elp)
	struct proc *p;
	struct exec_linker *elp;
{
	struct exec *a_out = (struct exec *)elp->el_image_hdr;

	u.u_error = exec_aout_prep_nmagic(p, elp);
	return (u.u_error);
}

int
exec_aout_prep_magic3(p, elp)
	struct proc *p;
	struct exec_linker *elp;
{
	struct exec *a_out = (struct exec*) elp->el_image_hdr;

	elp->el_taddr = USRTEXT;
	elp->el_tsize = a_out->a_text;
	elp->el_daddr = elp->el_taddr + a_out->a_text;
	elp->el_dsize = a_out->a_data + a_out->a_bss;
	elp->el_entry = a_out->a_entry;
	elp->el_flags |= EXEC_IDSEP;
	u.u_error = exec_aout_prep_common(p, elp, &elp->el_ovdata, a_out);
	elp->el_flags &= ~EXEC_IDSEP;
	return (u.u_error);
}

int
exec_aout_prep_magic4(p, elp)
	struct proc *p;
	struct exec_linker *elp;
{
	struct exec *a_out = (struct exec*) elp->el_image_hdr;

	elp->el_taddr = USRTEXT;
	elp->el_tsize = a_out->a_text;
	elp->el_daddr = elp->el_taddr + a_out->a_text;
	elp->el_dsize = a_out->a_data + a_out->a_bss;
	elp->el_entry = a_out->a_entry;
	elp->el_flags |= EXEC_OVERLAY;
	u.u_error = exec_aout_prep_common(p, elp, &elp->el_ovdata, a_out);
	elp->el_flags &= ~EXEC_OVERLAY;
	return (u.u_error);
}

int
exec_aout_prep_magic5(p, elp)
	struct proc *p;
	struct exec_linker *elp;
{
	struct exec *a_out = (struct exec*) elp->el_image_hdr;

	elp->el_taddr = USRTEXT;
	elp->el_tsize = a_out->a_text;
	elp->el_daddr = elp->el_taddr + a_out->a_text;
	elp->el_dsize = a_out->a_data + a_out->a_bss;
	elp->el_entry = a_out->a_entry;
	elp->el_flags |= EXEC_OVFLAG;
	u.u_error = exec_aout_prep_common(p, elp, &elp->el_ovdata, a_out);
	elp->el_flags &= ~EXEC_OVFLAG;
	return (u.u_error);
}

int
exec_aout_prep_magic6(p, elp)
	struct proc *p;
	struct exec_linker *elp;
{
	struct exec *a_out = (struct exec*) elp->el_image_hdr;

	elp->el_taddr = USRTEXT;
	elp->el_tsize = a_out->a_text;
	elp->el_daddr = elp->el_taddr + a_out->a_text;
	elp->el_dsize = a_out->a_data + a_out->a_bss;
	elp->el_entry = a_out->a_entry;
	elp->el_flags |= (EXEC_OVFLAG | EXEC_IDSEP);
	u.u_error = exec_aout_prep_common(p, elp, &elp->el_ovdata, a_out);
	elp->el_flags &= ~(EXEC_OVFLAG & EXEC_IDSEP);
	return (u.u_error);
}

void
exec_check_ovflags(elp, ovflag, overlay, sep)
	struct exec_linker *elp;
	int *ovflag, *overlay, *sep;
{
	ovflag = overlay = sep = 0;

	if ((elp->el_flags & EXEC_OVFLAG) != 0) {
		ovflag++;
	}
	if ((elp->el_flags & EXEC_OVERLAY) != 0) {
		overlay++;
	}
	if ((elp->el_flags & EXEC_IDSEP) != 0) {
		sep++;
	}
}

/*
 * Read in and set up memory for executed file.
 * u.u_error set on error
 */
int
exec_aout_prep_common(p, elp, eovd, a_out)
	struct proc *p;
	struct exec_linker *elp;
	struct exec_ovdata *eovd;
	struct exec *a_out;
{
	struct exec_ovdata sovdata;
	u_long *ovhead, ovmax;
	int error;
	int ovflag, overlay, sep;

	exec_check_ovflags(&ovflag, &overlay, &sep);

	error = exec_alloc_ovdata(eovd);
	if (error != 0) {
		goto bad;
	}
	sovdata = *eovd;
	eovd->eo_ovbase = 0;
    eovd->eo_curov = 0;
	if ((elp->el_flags & EXEC_OVFLAG) == 0) {
		/* set up for text segment */
		NEW_VMCMD(&elp->el_vmcmds, vmcmd_map_readvn, sizeof(ovhead),
				ovhead, (VM_PROT_READ | VM_PROT_EXECUTE),
				(VM_PROT_READ | VM_PROT_EXECUTE), elp->el_vnodep, sizeof(struct exec));
		if (vmcmds_proc_error(p, &elp->el_vmcmds) != 0) {
			error = ENOEXEC;
			goto bad;

		}
		eovd->eo_ovbase = ctos(elp->el_tsize);
		ovmax = btoc(ovhead[0]);
		eovd->eo_nseg = ctos(ovmax);
        eovd->eo_dbase = (eovd->eo_ovbase + eovd->eo_nseg);
        eovd->eo_ov_offset[0] = elp->el_tsize;
        {
            int i;
            u_long t = 0;

            /* check if any overlay is larger than ovmax */
            for (i = 1; i <= NOVL; i++) {
                t = btoc(ovhead[i]);
                if (t > ovmax) {
					error = ENOEXEC;
                    goto bad;
                }
                eovd->eo_ov_offset[i] = t + eovd->eo_ov_offset[i - 1];
            }
        }
	}

	if ((elp->el_flags & EXEC_OVERLAY) == 0) {
		if ((((elp->el_flags & EXEC_IDSEP) == 0) && ctos(elp->el_tsize) != ctos(a_out->a_text)) || elp->el_argc) {
			error = ENOMEM;
			goto bad;
		}
		elp->el_dsize = u.u_dsize;
		elp->el_ssize = u.u_ssize;
		sep = u.u_sep;
		vm_xfree();
		vm_xalloc(p, elp->el_tsize, sizeof(struct exec));
	} else {
		if (vm_estabur(p, elp->el_tsize, elp->el_dsize, elp->el_ssize, sep, SEG_RO)) {
			goto bad;
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
		vm_xalloc(p, elp->el_tsize, sizeof(struct exec));

		/*
		 * read in data segment
		 */
		(void)vm_estabur(p, 0, elp->el_dsize, 0, 0, SEG_RO);

		offset = sizeof(struct exec);
		if ((elp->el_flags & EXEC_OVFLAG) == 0) {
			offset += sizeof(ovhead);
			offset += ((eovd->eo_ov_offset[NOVL]) << 6);
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
	return (*elp->el_esch->ex_setup_stack)(p, elp);

bad:
	*eovd = sovdata;
	return (ENOEXEC);
}

int
vmcmd_map_ovdata(p, cmd)
	struct proc *p;
	struct exec_vmcmd *cmd;
{
	struct vmspace *vmspace;
	int error;

	vmspace = p->p_vmspace;

	if (cmd->ev_size == 0) {
		return (0);
	}
	error = vm_allocate(&vmspace->vm_map, &cmd->ev_addr, cmd->ev_size, 0);
	if (error) {
		return (error);
	}
	return (vmcmd_ovdata(p, cmd));
}

int
vmcmd_ovdata(p, cmd)
	struct proc *p;
	struct exec_vmcmd *cmd;
{
	struct vmspace *vmspace;
	int error;
	vm_prot_t prot, maxprot;
	size_t resid;

	vmspace = p->p_vmspace;
	error = vn_rdwr(UIO_READ, cmd->ev_vnodep, (caddr_t) cmd->ev_addr,
			cmd->ev_size, cmd->ev_offset, UIO_SYSSPACE, IO_UNIT, p->p_ucred,
			&resid, p);
	if (error != 0) {
		return (error);
	}
	if (resid != 0) {
		return (ENOEXEC);
	}
	prot = cmd->ev_prot;
	maxprot = VM_PROT_ALL;

	if (maxprot != VM_PROT_ALL) {
		error = vm_map_protect(&vmspace->vm_map, trunc_page(cmd->ev_addr),
				round_page(cmd->ev_addr + cmd->ev_size), maxprot, TRUE);
		if (error) {
			return (error);
		}
	}

	if (prot != maxprot) {
		error = vm_map_protect(&vmspace->vm_map, trunc_page(cmd->ev_addr),
				round_page(cmd->ev_addr + cmd->ev_size), prot, FALSE);
		if (error) {
			return (error);
		}
	}
	return (0);
}

#ifdef notyet
int
exec_aout_prep_common(p, elp, a_out, ovflag, overlay, sep)
	struct proc *p;
	struct exec_linker *elp;
	struct exec *a_out;
	int ovflag, overlay, sep;
{
	struct u_ovd sovdata;
	u_long ovhead[NOVL], ovoffset[NOVL];
	u_long ovbase, ovmax, curov, dbase, offset;
	long num;

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
		vm_xalloc(p, elp->el_tsize, sizeof(struct exec));

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
	return (*elp->el_esch->ex_setup_stack)(p, elp);
}
#endif
