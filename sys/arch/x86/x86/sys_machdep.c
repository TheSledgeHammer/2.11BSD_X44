/*
 * sys_machdep.c
 *
 *  Created on: 17 Jun 2020
 *      Author: marti
 */

#include <sysarch.h>

int x86_get_ldt (int, union descriptor *, int);
int x86_set_ldt (int, union descriptor *, int);
int x86_iopl (int);
int x86_get_ioperm (u_long *);
int x86_set_ioperm (u_long *);

#define	X86_PMC_INFO		8
#define	X86_PMC_STARTSTOP	9
#define	X86_PMC_READ		10
#define X86_GET_MTRR		11
#define X86_SET_MTRR		12
#define	X86_VM86			13
#define	X86_GET_GSBASE		14
#define	X86_GET_FSBASE		15
#define	X86_SET_GSBASE		16
#define	X86_SET_FSBASE		17

int
x86_set_sdbase(void *arg, char which)
{
#ifdef i386
	struct segment_descriptor sd;
	caddr_t base;
	int error;

	error = copyin(arg, &base, sizeof(base));
	if (error != 0)
		return error;

	sd.sd_lobase = base & 0xffffff;
	sd.sd_hibase = (base >> 24) & 0xff;
	sd.sd_lolimit = 0xffff;
	sd.sd_hilimit = 0xf;
	sd.sd_type = SDT_MEMRWA;
	sd.sd_dpl = SEL_UPL;
	sd.sd_p = 1;
	sd.sd_xx = 0;
	sd.sd_def32 = 1;
	sd.sd_gran = 1;

	kpreempt_disable();
	if (which == 'f') {
		memcpy(&curpcb->pcb_fsd, &sd, sizeof(sd));
		memcpy(&curcpu()->ci_gdt[GUFS_SEL], &sd, sizeof(sd));
	} else /* which == 'g' */ {
		memcpy(&curpcb->pcb_gsd, &sd, sizeof(sd));
		memcpy(&curcpu()->ci_gdt[GUGS_SEL], &sd, sizeof(sd));
	}
	kpreempt_enable();

	return 0;
#else
	return EINVAL;
#endif
}

int
x86_get_sdbase(void *arg, char which)
{
#ifdef i386
	struct segment_descriptor *sd;
	caddr_t base;

	switch (which) {
	case 'f':
		sd = (struct segment_descriptor *)&curpcb->pcb_fsd;
		break;
	case 'g':
		sd = (struct segment_descriptor *)&curpcb->pcb_gsd;
		break;
	default:
		panic("x86_get_sdbase");
	}

	base = sd->sd_hibase << 24 | sd->sd_lobase;
	return copyout(&base, &arg, sizeof(base));
#else
	return EINVAL;
#endif
}

int
x86_sysarch(p, uap, retval)
	struct proc *p;
	struct sysarch_args *uap;
	register_t *retval;
{
	int error = 0;

	if(i386_sysarch(p, uap, retval)) {
		switch(uap->op) {
#ifdef PERFCTRS
		case X86_PMC_INFO:
		case X86_PMC_STARTSTOP:
		case X86_PMC_READ:
#endif
		case X86_GET_MTRR:
			error = x86_get_mtrr(p, SCARG(uap, parms), retval);
			break;
		case X86_SET_MTRR:
			error = x86_set_mtrr(p, SCARG(uap, parms), retval);
			break;
		case X86_GET_GSBASE:
			error = x86_get_sdbase(SCARG(uap, parms), 'g');
			break;
		case X86_GET_FSBASE:
			error = x86_get_sdbase(SCARG(uap, parms), 'f');
			break;
		case X86_SET_GSBASE:
			error = x86_set_sdbase(SCARG(uap, parms), 'g');
			break;
		case X86_SET_FSBASE:
			error = x86_set_sdbase(SCARG(uap, parms), 'f');
			break;
		default:
			error = EINVAL;
			break;
		}
	}
	return (error);
}

/*
int
sysarch(p, uap, retval)
	struct proc *p;
	struct sysarch_args *uap;
	register_t *retval;
{
	if(MACHINE == "x86") {
		return (x86_sysarch(p, uap, retval));
	} else {
		return (i386_sysarch(p, uap, retval));
	}
}
*/
