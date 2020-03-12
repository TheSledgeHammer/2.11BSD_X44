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
/*
#include <sys/malloc.h>

#include <vm/include/vm.h>
#include <vm/include/pmap.h>
#include <vm/include/vm_map.h>
#include <vm/include/vm_extern.h>
#include <vm/include/vm_kern.h>		*/ /* for kernel_map */
/*
#include <machine/cpu.h>
#include <machine/pcb.h>
#include <machine/proc.h>
#include <machine/sysarch.h>


int
sysarch(p, uap)
	struct proc *p;
	struct sysarch_args *uap;
{
	int error;
	union descriptor *lp;
	union {
		struct i386_ldt_args largs;
		struct i386_ioperm_args iargs;
		struct i386_get_xfpustate xfpu;
	} kargs;
	uint32_t base;
	struct segment_descriptor *sdp;

	switch (uap->op) {
	case I386_GET_IOPERM:
	case I386_SET_IOPERM:
	case I386_GET_LDT:
	case I386_SET_LDT:
	case I386_GET_XFPUSTATE:
	default:
		break;
	}

	switch (uap->op) {
	case I386_GET_LDT:
	case I386_SET_LDT:
	case I386_GET_IOPERM:
	case I386_SET_IOPERM:
	case I386_VM86:
	case I386_GET_FSBASE:
	case I386_SET_FSBASE:
	case I386_GET_GSBASE:
	case I386_SET_GSBASE:
	case I386_GET_XFPUSTATE:
	default:
		break;
	}
	return (error);
}
*/

void
fill_based_sd(sdp, base)
 	 struct segment_descriptor *sdp;
 	 uint32_t base;
{
 	sdp->sd_lobase = base & 0xffffff;
 	sdp->sd_hibase = (base >> 24) & 0xff;
 	sdp->sd_lolimit = 0xffff;	/* 4GB limit, wraps around */
 	sdp->sd_hilimit = 0xf;
 	sdp->sd_type = SDT_MEMRWA;
 	sdp->sd_dpl = SEL_UPL;
 	sdp->sd_p = 1;
 	sdp->sd_xx = 0;
 	sdp->sd_def32 = 1;
 	sdp->sd_gran = 1;
}

/*
 * Construct special descriptors for "base" selectors.  Store them in
 * the PCB for later use by cpu_switch().  Store them in the GDT for
 * more immediate use.  The GDT entries are part of the current
 * context.  Callers must load related segment registers to complete
 * setting up the current context.
 */
void
set_fsbase(p, base)
	struct proc *p;
	uint32_t base;
{
	struct segment_descriptor sd;
	fill_based_sd(&sd, base);
	//critical_enter()
	p->p_addr->u_pcb.pcb_fsd = sd;
	//critical_exit()
}

void
set_gsbase(p, base)
	struct proc *p;
	uint32_t base;
{
	struct segment_descriptor sd;
	fill_based_sd(&sd, base);
	//critical_enter()
	p->p_addr->u_pcb.pcb_gsd = sd;
	//critical_exit()
}

int
i386_extend_pcb(p)
	struct proc *p;
{
	int i, offset;
	u_long *addr;
	struct pcb_ext *ext;

	return (0);
}

int
i386_set_ioperm(p, uap)
	struct proc *p;
	struct i386_ioperm_args *uap;
{
	char *iomap;
	u_int i;
	int error;

	return (0);
}

int
i386_get_ioperm(p, uap)
	struct proc *p;
	struct i386_ioperm_args *uap;
{
	int i, state;
	char *iomap;

	return (0);
}


int
i386_get_ldt(p, uap)
	struct proc *p;
	struct i386_ldt_args *uap;
{
	struct proc_ldt *pldt;
	char *data;
	u_int nldt, num;
	int error;

	return (error);
}

int
i386_set_ldt(p, uap, descs)
	struct proc *p;
	struct i386_ldt_args *uap;
    union descriptor *descs;
{
	struct mdproc *mdp;
	struct proc_ldt *pldt;
	union descriptor *dp;
	u_int largest_ld, i;
	int error;

#ifdef DEBUG
	printf("i386_set_ldt: start=%u num=%u descs=%p\n",
	    uap->start, uap->num, (void *)uap->descs);
#endif
	error = 0;
	mdp = &p->p_md;

	if (descs == NULL) {
		/* Free descriptors */
		if (uap->start == 0 && uap->num == 0) {
			/*
			 * Treat this as a special case, so userland needn't
			 * know magic number NLDT.
			 */
			uap->start = NLDT;
			uap->num = MAX_LD - NLDT;
		}
		if ((pldt = mdp->md_ldt) == NULL || uap->start >= pldt->ldt_len) {
			return (0);
		}
		largest_ld = uap->start + uap->num;
		if (largest_ld > pldt->ldt_len)
			largest_ld = pldt->ldt_len;
		return (0);
	}

	if (uap->start != LDT_AUTO_ALLOC || uap->num != 1) {
		/* verify range of descriptors to modify */
		largest_ld = uap->start + uap->num;
		if (uap->start >= MAX_LD || largest_ld > MAX_LD)
			return (EINVAL);
	}

	/* Check descriptors for access violations */
	for (i = 0; i < uap->num; i++) {
		dp = &descs[i];


		switch (dp->sd.sd_type) {
		case SDT_SYSNULL:	/* system null */
			dp->sd.sd_p = 0;
			break;
		case SDT_SYS286TSS: /* system 286 TSS available */
		case SDT_SYSLDT:    /* system local descriptor table */
		case SDT_SYS286BSY: /* system 286 TSS busy */
		case SDT_SYSTASKGT: /* system task gate */
		case SDT_SYS286IGT: /* system 286 interrupt gate */
		case SDT_SYS286TGT: /* system 286 trap gate */
		case SDT_SYSNULL2:  /* undefined by Intel */
		case SDT_SYS386TSS: /* system 386 TSS available */
		case SDT_SYSNULL3:  /* undefined by Intel */
		case SDT_SYS386BSY: /* system 386 TSS busy */
		case SDT_SYSNULL4:  /* undefined by Intel */
		case SDT_SYS386IGT: /* system 386 interrupt gate */
		case SDT_SYS386TGT: /* system 386 trap gate */
		case SDT_SYS286CGT: /* system 286 call gate */
		case SDT_SYS386CGT: /* system 386 call gate */
			return (EACCES);


		/* memory segment types */
		case SDT_MEMEC:   /* memory execute only conforming */
		case SDT_MEMEAC:  /* memory execute only accessed conforming */
		case SDT_MEMERC:  /* memory execute read conforming */
		case SDT_MEMERAC: /* memory execute read accessed conforming */
			 /* Must be "present" if executable and conforming. */
			if (dp->sd.sd_p == 0)
				return (EACCES);
			break;
		case SDT_MEMRO:   /* memory read only */
		case SDT_MEMROA:  /* memory read only accessed */
		case SDT_MEMRW:   /* memory read write */
		case SDT_MEMRWA:  /* memory read write accessed */
		case SDT_MEMROD:  /* memory read only expand dwn limit */
		case SDT_MEMRODA: /* memory read only expand dwn lim accessed */
		case SDT_MEMRWD:  /* memory read write expand dwn limit */
		case SDT_MEMRWDA: /* memory read write expand dwn lim acessed */
		case SDT_MEME:    /* memory execute only */
		case SDT_MEMEA:   /* memory execute only accessed */
		case SDT_MEMER:   /* memory execute read */
		case SDT_MEMERA:  /* memory execute read accessed */
			break;
		default:
			return (EINVAL);
		}

		/* Only user (ring-3) descriptors may be present. */
		if (dp->sd.sd_p != 0 && dp->sd.sd_dpl != SEL_UPL)
			return (EACCES);
	}


	return (error);
}

#ifdef TRACE
int	nvualarm;

struct vtrace_args {
	int	request;
	int	value;
};
vtrace(p, uap, retval)
	struct proc *p;
	register struct args *uap;
	int *retval;
{
	int vdoualarm();

	switch (uap->request) {

	case VTR_DISABLE:		/* disable a trace point */
	case VTR_ENABLE:		/* enable a trace point */
		if (uap->value < 0 || uap->value >= TR_NFLAGS)
			return (EINVAL);
		*retval = traceflags[uap->value];
		traceflags[uap->value] = uap->request;
		break;

	case VTR_VALUE:		/* return a trace point setting */
		if (uap->value < 0 || uap->value >= TR_NFLAGS)
			return (EINVAL);
		*retval = traceflags[uap->value];
		break;

	case VTR_UALARM:	/* set a real-time ualarm, less than 1 min */
		if (uap->value <= 0 || uap->value > 60 * hz || nvualarm > 5)
			return (EINVAL);
		nvualarm++;
		timeout(vdoualarm, (caddr_t)p->p_pid, uap->value);
		break;

	case VTR_STAMP:
		trace(TR_STAMP, uap->value, p->p_pid);
		break;
	}
	return (0);
}

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
