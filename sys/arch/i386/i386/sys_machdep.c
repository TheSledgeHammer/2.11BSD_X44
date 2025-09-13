/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)sys_machdep.c	8.1 (Berkeley) 6/11/93
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/mtio.h>
#include <sys/buf.h>
#include <sys/trace.h>
#include <sys/malloc.h>
#include <sys/user.h>
#include <sys/tls.h>

#include <vm/include/vm.h>
#include <vm/include/pmap.h>
#include <vm/include/vm_map.h>
#include <vm/include/vm_extern.h>
#include <vm/include/vm_kern.h>		/* for kernel_map */

#include <machine/cpu.h>
#include <machine/cpufunc.h>
#include <machine/gdt.h>
#include <machine/psl.h>
#include <machine/reg.h>
#include <machine/sysarch.h>

extern vm_map_t kernel_map;

#ifdef USER_LDT
int i386_get_ldt(struct proc *, void *, register_t *);
int i386_set_ldt(struct proc *, void *, register_t *);
#endif
int i386_get_ioperm(struct proc *, void *, register_t *);
int i386_set_ioperm(struct proc *, void *, register_t *);
int i386_set_sdbase(struct proc *, void *, char);
int i386_get_sdbase(struct proc *, void *, char);

#ifdef TRACE
int	 nvualarm;
void vdoualarm(int);

int
vtrace()
{
	register struct vtrace_args {
		syscallarg(int) request;
		syscallarg(int) value;
	} *uap = (struct vtrace_args *)u.u_ap;
	struct proc *p;

	p = u.u_procp;
	switch (SCARG(uap, request)) {
	case VTR_DISABLE:		/* disable a trace point */
	case VTR_ENABLE:		/* enable a trace point */
		if (SCARG(uap, value) < 0 || SCARG(uap, value) >= TR_NFLAGS)
			return (EINVAL);
		u.u_r.r_val1 = traceflags[SCARG(uap, value)];
		traceflags[SCARG(uap, value)] = SCARG(uap, request);
		break;

	case VTR_VALUE:		/* return a trace point setting */
		if (SCARG(uap, value) < 0 || SCARG(uap, value) >= TR_NFLAGS)
			return (EINVAL);
		u.u_r.r_val1 = traceflags[SCARG(uap, value)];
		break;

	case VTR_UALARM:	/* set a real-time ualarm, less than 1 min */
		if (SCARG(uap, value) <= 0 || SCARG(uap, value) > 60 * hz || nvualarm > 5)
			return (EINVAL);
		nvualarm++;
		timeout(vdoualarm, (caddr_t)p->p_pid, SCARG(uap, value));
		break;

	case VTR_STAMP:
		trace(TR_STAMP, SCARG(uap, value), p->p_pid);
		break;
	}
	return (0);
}

void
vdoualarm(arg)
	int arg;
{
	register struct proc *p;

	p = pfind(arg);
	if (p)
		psignal(p, 16);
	nvualarm--;
}
#endif

#ifdef USER_LDT
int
i386_get_ldt(p, args, retval)
	struct proc *p;
	void *args;
	register_t *retval;
{
	int error;
	struct pcb *pcb = &p->p_addr->u_pcb;
	int nldt, num;
	union descriptor *lp, *cp;
	struct i386_get_ldt_args ua;

	if ((error = copyin(args, &ua, sizeof(ua))) != 0) {
		return (error);
	}

#ifdef	LDT_DEBUG
	printf("i386_get_ldt: start=%d num=%d descs=%p\n", ua.start, ua.num, ua.desc);
#endif

	if (ua.start < 0 || ua.num < 0 || ua.start > 8192 || ua.num > 8192 || ua.start + ua.num > 8192) {
		return (EINVAL);
	}

	cp = malloc(ua.num * sizeof(union descriptor), M_TEMP, M_WAITOK);
	if (cp == NULL) {
		return ENOMEM;
	}

	if (pcb->pcb_flags & PMF_USER_LDT) {
		nldt = pcb->pcb_ldt_len;
		lp =  pcb->pcb_desc;
	} else {
		nldt = NLDT;
		lp = ldt;
	}

	if (ua.start > nldt) {
		free(cp, M_TEMP);
		return (EINVAL);
	}

	lp += ua.start;
	num = min(ua.num, nldt - ua.start);

	memcpy(cp, lp, num * sizeof(union descriptor));

	error = copyout(lp, ua.desc, num * sizeof(union descriptor));
	if (error) {
		*retval = num;
	}

	free(cp, M_TEMP);
	return (error);
}

int
i386_set_ldt(p, args, retval)
	struct proc *p;
	void *args;
	register_t *retval;
{
	int error, i, n;
	struct pcb *pcb = &p->p_addr->u_pcb;
	struct i386_set_ldt_args ua;
	union descriptor *descv;
	size_t old_len, new_len, ldt_len;
	union descriptor *old_ldt, *new_ldt;

	if ((error = copyin(args, &ua, sizeof(ua))) != 0) {
		return (error);
	}

#ifdef	LDT_DEBUG
	printf("i386_set_ldt: start=%d num=%d descs=%p\n", ua.start, ua.num, ua.desc);
#endif

	if (ua.start < 0 || ua.num < 0 || ua.start > 8192 || ua.num > 8192 ||
	    ua.start + ua.num > 8192) {
		return (EINVAL);
	}

	descv = malloc(sizeof(*descv) * ua.num, M_TEMP, M_NOWAIT);

	if (descv == NULL) {
		return (ENOMEM);
	}

	if ((error = copyin(ua.desc, descv, sizeof (*descv) * ua.num)) != 0) {
		goto out;
	}

	/* Check descriptors for access violations. */
	for (i = 0, n = ua.start; i < ua.num; i++, n++) {
		union descriptor *desc = &descv[i];

		switch (desc->sd.sd_type) {
		case SDT_SYSNULL:
			desc->sd.sd_p = 0;
			break;
		case SDT_SYS286CGT:
		case SDT_SYS386CGT:
			/*
			 * Only allow call gates targeting a segment
			 * in the LDT or a user segment in the fixed
			 * part of the gdt.  Segments in the LDT are
			 * constrained (below) to be user segments.
			 */
			if (desc->gd.gd_p != 0 && !ISLDT(desc->gd.gd_selector)
					&& ((IDXSEL(desc->gd.gd_selector) >= NGDT)
							|| (gdt[IDXSEL(desc->gd.gd_selector)].sd.sd_dpl
									!= SEL_UPL))) {
				error = EACCES;
				goto out;
			}
			break;
		case SDT_MEMEC:
		case SDT_MEMEAC:
		case SDT_MEMERC:
		case SDT_MEMERAC:
			/* Must be "present" if executable and conforming. */
			if (desc->sd.sd_p == 0) {
				error = EACCES;
				goto out;
			}
			break;
		case SDT_MEMRO:
		case SDT_MEMROA:
		case SDT_MEMRW:
		case SDT_MEMRWA:
		case SDT_MEMROD:
		case SDT_MEMRODA:
		case SDT_MEME:
		case SDT_MEMEA:
		case SDT_MEMER:
		case SDT_MEMERA:
			break;
		default:
			/*
			 * Make sure that unknown descriptor types are
			 * not marked present.
			 */
			if (desc->sd.sd_p != 0) {
				error = EACCES;
				goto out;
			}
			break;
		}

		if (desc->sd.sd_p != 0) {
			/* Only user (ring-3) descriptors may be present. */
			if (desc->sd.sd_dpl != SEL_UPL) {
				error = EACCES;
				goto out;
			}
		}
	}

	/* allocate user ldt */
	if (pcb->pcb_desc == 0 || (ua.start + ua.num) > pcb->pcb_ldt_len) {

		if (pcb->pcb_flags & PMF_USER_LDT) {
			ldt_len = pcb->pcb_ldt_len;
		} else {
			ldt_len = 512;
		}
		while ((ua.start + ua.num) > pcb->pcb_ldt_len) {
			ldt_len *= 2;
		}
		new_len = ldt_len * sizeof(union descriptor);

		if (pcb->pcb_desc != NULL && ldt_len <= pcb->pcb_ldt_len) {
			kmem_free(kernel_map, (vm_offset_t) new_ldt, new_len);
			goto copy;
		}

		old_ldt = pcb->pcb_desc;
		if (old_ldt != NULL) {
			old_len = pcb->pcb_ldt_len * sizeof(union descriptor);
		} else {
			old_len = NLDT * sizeof(union descriptor);
			old_ldt = ldt;
		}
		memcpy(new_ldt, old_ldt, old_len);
		memset((caddr_t) new_ldt + old_len, 0, new_len - old_len);

		if (old_ldt != ldt) {
			kmem_free(kernel_map, (vm_offset_t) old_ldt, old_len);
		}
		pcb->pcb_desc = new_ldt;
		pcb->pcb_ldt_len = ldt_len;
		pcb->pcb_flags |= PCB_USER_LDT;

		if (pcb == curpcb) {
			lldt(pcb->pcb_ldt_sel);
		}

#ifdef LDT_DEBUG
			printf("i386_set_ldt(%d): new_ldt=%p\n", p->p_pid, new_ldt);
	#endif
	}

copy:
	/* Now actually replace the descriptors. */
	for (i = 0, n = ua.start; i < ua.num; i++, n++) {
		pcb->pcb_desc[n] = descv[i];
	}

	*retval = ua.start;

out:
	free(descv, M_TEMP);
	return (error);
}
#endif	/* USER_LDT */

int
i386_iopl(p, args, retval)
	struct proc *p;
	void *args;
	register_t *retval;
{
	int error;
	struct trapframe *tf = p->p_md.md_regs;
	struct i386_iopl_args ua;

	if (securelevel > 1) {
		return EPERM;
	}

	if ((error = suser1(p->p_ucred, &p->p_acflag)) != 0) {
		return error;
	}

	if ((error = copyin(args, &ua, sizeof(ua))) != 0) {
		return error;
	}

	if (ua.iopl) {
		tf->tf_eflags |= PSL_IOPL;
	} else {
		tf->tf_eflags &= ~PSL_IOPL;
	}

	return (0);
}

int
i386_get_ioperm(p, args, retval)
	struct proc *p;
	void *args;
	register_t *retval;
{
	int error;
	struct pcb *pcb = &p->p_addr->u_pcb;
	struct i386_get_ioperm_args ua;

	if ((error = copyin(args, &ua, sizeof(ua))) != 0) {
		return (error);
	}

	return (copyout(pcb->pcb_iomap, ua.iomap, sizeof(pcb->pcb_iomap)));
}

int
i386_set_ioperm(p, args, retval)
	struct proc *p;
	void *args;
	register_t *retval;
{
	int error;
	struct pcb *pcb = &p->p_addr->u_pcb;
	struct i386_set_ioperm_args ua;

	if (securelevel > 1)
		return EPERM;

	if ((error = suser1(p->p_ucred, &p->p_acflag)) != 0) {
		return error;
	}

	if ((error = copyin(args, &ua, sizeof(ua))) != 0) {
		return (error);
	}

	return (copyin(ua.iomap, pcb->pcb_iomap, sizeof(pcb->pcb_iomap)));
}

void
fill_based_sd(sdp, base)
	struct segment_descriptor *sdp;
	uint32_t base;
{
	sdp->sd_lobase = base & 0xffffff;
	sdp->sd_hibase = (base >> 24) & 0xff;
	sdp->sd_lolimit = 0xffff;
	sdp->sd_hilimit = 0xf;
	sdp->sd_type = SDT_MEMRWA;
	sdp->sd_dpl = SEL_UPL;
	sdp->sd_p = 1;
	sdp->sd_xx = 0;
	sdp->sd_def32 = 1;
	sdp->sd_gran = 1;
}

int
i386_set_sdbase(p, arg, which)
	struct proc *p;
	void *arg;
	char which;
{
	struct segment_descriptor sd;
	uint32_t base;
	int error;

	error = copyin(arg, &base, sizeof(base));
	if (error != 0) {
		return (error);
	}

	fill_based_sd(&sd, base);

	switch(which) {
	case 'f':
		p->p_addr->u_pcb.pcb_fsd = sd;
		p->p_addr->u_frame->tf_fs = GSEL(GUFS_SEL, SEL_UPL);
		break;
	case 'g':
		p->p_addr->u_pcb.pcb_gsd = sd;
		p->p_addr->u_frame->tf_fs = GSEL(GUGS_SEL, SEL_UPL);
		break;
	default:
		panic("i386_set_sdbase");
	}

	return (0);
}

int
i386_get_sdbase(p, arg, which)
	struct proc *p;
	void *arg;
	char which;
{
	struct segment_descriptor *sd;
	uint32_t base;
	int error;

	switch (which) {
	case 'f':
		sd = &p->p_addr->u_pcb.pcb_fsd;
		break;
	case 'g':
		sd = &p->p_addr->u_pcb.pcb_gsd;
		break;
	default:
		panic("i386_get_sdbase");
	}

	base = sd->sd_hibase << 24 | sd->sd_lobase;
	error = copyout(&base, &arg, sizeof(base));
	return (error);
}

int
sysarch()
{
	register struct sysarch_args {
		syscallarg(int) 	op;
		syscallarg(void *) 	parms;
	} *uap = (struct sysarch_args *) u.u_ap;
	struct proc *p;
	register_t *retval;
	int error;

	p = u.u_procp;
	retval = (register_t *)&u.u_r.r_val1;
	error = 0;

	switch (SCARG(uap, op)) {
#ifdef	USER_LDT
	case I386_GET_LDT:
		error = i386_get_ldt(p, SCARG(uap, parms), retval);
		break;
	case I386_SET_LDT:
		error = i386_set_ldt(p, SCARG(uap, parms), retval);
		break;
#endif
	case I386_GET_IOPERM:
		error = i386_get_ioperm(p, SCARG(uap, parms), retval);
		break;
	case I386_SET_IOPERM:
		error = i386_set_ioperm(p, SCARG(uap, parms), retval);
		break;
	case I386_VM86:
		error = vm86_sysarch(p, SCARG(uap, parms), retval);
		break;
	case I386_GET_GSBASE:
		error = i386_get_sdbase(p, SCARG(uap, parms), 'g');
		break;
	case I386_GET_FSBASE:
		error = i386_get_sdbase(p, SCARG(uap, parms), 'f');
		break;
	case I386_SET_GSBASE:
		error = i386_set_sdbase(p, SCARG(uap, parms), 'g');
		break;
	case I386_SET_FSBASE:
		error = i386_set_sdbase(p, SCARG(uap, parms), 'f');
		break;
	default:
		error = EINVAL;
		break;
	}
	return (error);
}

/* MD tls (set and get) */
int
cpu_get_tls_tcb(p, arg, which)
	struct proc *p;
	void *arg;
	char which;
{
	return (i386_get_sdbase(p, arg, which));
}

int
cpu_set_tls_tcb(p, arg, which)
	struct proc *p;
	void *arg;
	char which;
{
	return (i386_set_sdbase(p, arg, which));
}

void *
cpu_get_tls_addr(void)
{
	void *tmp;

	__asm volatile("movl %%gs:0, %0" : "=r" (tmp));
	return (tmp);
}
