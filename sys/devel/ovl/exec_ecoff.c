/*	$NetBSD: exec_ecoff.c,v 1.11 2000/04/11 04:37:50 chs Exp $	*/

/*
 * Copyright (c) 1994 Adam Glass
 * Copyright (c) 1993, 1994, 1996, 1999 Christopher G. Demetriou
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Christopher G. Demetriou
 *      for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/mman.h>
#include <sys/exec.h>
#include <sys/exec_linker.h>
#include <sys/exec_ecoff.h>
#include <sys/resourcevar.h>

/*
 * Notes:
 * To try and maintain some stability and consistency from 2.11BSD, whose
 * a.out format is most similar to OMAGIC. Hence a.out MAGIC3 through to MAGIC6
 * are also setup based around OMAGIC.
 */

static int exec_ecoff_prep_common(struct proc *, struct exec_linker *, struct exec_ovdata *, struct ecoff_exechdr *, struct ecoff_aouthdr *, struct vnode *);

int
exec_ecoff_prep_magic3(p, elp, ecoff, vp)
	struct proc *p;
	struct exec_linker *elp;
	struct ecoff_exechdr *ecoff;
	struct vnode *vp;
{
	struct ecoff_aouthdr *ecoff_aout = &ecoff->a;

	elp->el_taddr = ECOFF_SEGMENT_ALIGN(ecoff, ecoff_aout->text_start);
	elp->el_tsize = ecoff_aout->tsize;
	elp->el_daddr = ECOFF_SEGMENT_ALIGN(ecoff, ecoff_aout->data_start);
	elp->el_dsize = ecoff_aout->dsize + ecoff_aout->bsize;
	elp->el_entry = ecoff_aout->entry;
	elp->el_flags |= EXEC_IDSEP;
	u.u_error = exec_ecoff_prep_common(p, elp, &elp->el_ovdata, ecoff, ecoff_aout, vp);
	elp->el_flags &= ~EXEC_IDSEP;
	return (u.u_error);
}

int
exec_ecoff_prep_magic4(p, elp, ecoff, vp)
	struct proc *p;
	struct exec_linker *elp;
	struct ecoff_exechdr *ecoff;
	struct vnode *vp;
{
	struct ecoff_aouthdr *ecoff_aout = &ecoff->a;

	elp->el_taddr = ECOFF_SEGMENT_ALIGN(ecoff, ecoff_aout->text_start);
	elp->el_tsize = ecoff_aout->tsize;
	elp->el_daddr = ECOFF_SEGMENT_ALIGN(ecoff, ecoff_aout->data_start);
	elp->el_dsize = ecoff_aout->dsize + ecoff_aout->bsize;
	elp->el_entry = ecoff_aout->entry;
	elp->el_flags |= EXEC_OVERLAY;
	u.u_error = exec_ecoff_prep_common(p, elp, &elp->el_ovdata, ecoff, ecoff_aout, vp);
	elp->el_flags &= ~EXEC_OVERLAY;
	return (u.u_error);
}

int
exec_ecoff_prep_magic5(p, elp, ecoff, vp)
	struct proc *p;
	struct exec_linker *elp;
	struct ecoff_exechdr *ecoff;
	struct vnode *vp;
{
	struct ecoff_aouthdr *ecoff_aout = &ecoff->a;

	elp->el_taddr = ECOFF_SEGMENT_ALIGN(ecoff, ecoff_aout->text_start);
	elp->el_tsize = ecoff_aout->tsize;
	elp->el_daddr = ECOFF_SEGMENT_ALIGN(ecoff, ecoff_aout->data_start);
	elp->el_dsize = ecoff_aout->dsize + ecoff_aout->bsize;
	elp->el_entry = ecoff_aout->entry;
	elp->el_flags |= EXEC_OVFLAG;
	u.u_error = exec_ecoff_prep_common(p, elp, &elp->el_ovdata, ecoff, ecoff_aout, vp);
	elp->el_flags &= ~EXEC_OVFLAG;
	return (u.u_error);
}

int
exec_ecoff_prep_magic6(p, elp, ecoff, vp)
	struct proc *p;
	struct exec_linker *elp;
	struct ecoff_exechdr *ecoff;
	struct vnode *vp;
{
	struct ecoff_aouthdr *ecoff_aout = &ecoff->a;

	elp->el_taddr = ECOFF_SEGMENT_ALIGN(ecoff, ecoff_aout->text_start);
	elp->el_tsize = ecoff_aout->tsize;
	elp->el_daddr = ECOFF_SEGMENT_ALIGN(ecoff, ecoff_aout->data_start);
	elp->el_dsize = ecoff_aout->dsize + ecoff_aout->bsize;
	elp->el_entry = ecoff_aout->entry;
	elp->el_flags |= (EXEC_OVFLAG | EXEC_IDSEP);
	u.u_error = exec_ecoff_prep_common(p, elp, &elp->el_ovdata, ecoff, ecoff_aout, vp);
	elp->el_flags &= ~(EXEC_OVFLAG & EXEC_IDSEP);
	return (u.u_error);
}

/*
 * Read in and set up memory for executed file.
 * u.u_error set on error
 */
static int
exec_ecoff_prep_common(p, elp, eovd, ecoff, ecoff_aout, vp)
	struct proc *p;
	struct exec_linker *elp;
	struct exec_ovdata *eovd;
	struct ecoff_exechdr *ecoff;
	struct ecoff_aouthdr *ecoff_aout;
	struct vnode *vp;
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
				(VM_PROT_READ | VM_PROT_EXECUTE), vp, ECOFF_TXTOFF(ecoff));
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
		if ((((elp->el_flags & EXEC_IDSEP) == 0) && ctos(elp->el_tsize) != ctos(ecoff_aout->tsize)) || elp->el_argc) {
			error = ENOMEM;
			goto bad;
		}
		elp->el_dsize = u.u_dsize;
		elp->el_ssize = u.u_ssize;
		sep = u.u_sep;
		vm_xfree();
		vm_xalloc(p, elp->el_tsize, ECOFF_TXTOFF(ecoff));
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
			baddr = ECOFF_SEGMENT_ALIGN(ecoff, ecoff_aout->bss_start);
			bsize = ecoff_aout->bsize;
			if (bsize > 0) {
				NEW_VMCMD(&elp->el_vmcmds, vmcmd_map_zero, bsize, baddr,
					(VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE),
					(VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE), NULL, 0);
			}
			bzero(p->p_daddr + baddr, bsize);
		}
		vm_expand(elp->el_ssize, S_STACK);
		bzero(p->p_saddr, elp->el_ssize);
		vm_xalloc(p, elp->el_tsize, ECOFF_TXTOFF(ecoff));

		/*
		 * read in data segment
		 */
		(void)vm_estabur(p, 0, elp->el_dsize, 0, 0, SEG_RO);

		offset = ECOFF_DATOFF(ecoff) - ecoff_aout->tsize;
		if ((elp->el_flags & EXEC_OVFLAG) == 0) {
			offset += sizeof(ovhead);
			offset += ((eovd->eo_ov_offset[NOVL]) << 6);
		} else {
			offset += ecoff_aout->tsize;
		}

		/* set up for data segment */
		NEW_VMCMD(&elp->el_vmcmds, vmcmd_map_readvn, ecoff_aout->dsize,
			elp->el_daddr, (VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE),
			(VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE), vp,
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
