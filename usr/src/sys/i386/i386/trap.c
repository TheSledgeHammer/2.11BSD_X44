/* UNIX V7 source code: see /COPYRIGHT or www.tuhs.org for details. */
/* Changes: Copyright (c) 1999 Robert Nordier. All rights reserved. */

#include <../include/machparam.h>
#include <../sys/types.h>
#include "../sys/param.h"
#include "../sys/systm.h"
#include "../sys/dir.h"
#include "../sys/user.h"
#include "../sys/proc.h"
#include <../include/reg.h>
#include <../include/seg.h>

#define	EBIT	1		/* user error bit in PS: C-bit */
#define	USER	0200		/* user-mode flag added to dev */

/*
 * Offsets of the user's registers relative to
 * the saved r0. See reg.h
 */
char	regloc[10] =
        {
                EAX, EDX, ECX, EBX, ESI, EDI, EBP, ESP, EIP, EFL
        };

/*
 * Called from mch.s when a processor trap occurs.
 * The arguments are the words saved on the system stack
 * by the hardware and software during the trap processing.
 * Their order is dictated by the hardware and the details
 * of C's calling sequence. They are peculiar in that
 * this call is not 'by value' and changed user registers
 * get copied back on return.
 * dev is the kind of trap that occurred.
 */
trap(tr)
struct trap tr;
{
register i;
register *a;
register struct sysent *callp;
time_t syst;
int osp;

syst = u.u_stime;
u.u_fpsaved = 0;
if (USERMODE(tr.cs))
tr.dev |= USER;
u.u_ar0 = &tr.eax;
switch(minor(tr.dev)) {

/*
 * Trap not expected.
 * Usually a kernel mode bus error.
 */
default:
printf("eax=%x ecx=%x edx=%x ebx=%x\n",
u.u_ar0[EAX], u.u_ar0[ECX], u.u_ar0[EDX], u.u_ar0[EBX]);
printf("esp=%x ebp=%x esi=%x edi=%x\n",
tr.esp, u.u_ar0[EBP], u.u_ar0[ESI], u.u_ar0[EDI]);
printf("cr0=%x  cr2=%x  cr3=%x\n",
ld_cr0(), ld_cr2(), ld_cr3());
printf("eip=%x  eflags=%x  err=%x\n",
tr.eip, tr.efl, tr.err);
printf("trap type %x\n", tr.dev);
panic("trap");

case 0+USER: /* divide error */
case 4+USER: /* overflow exception */
case 5+USER: /* bound range exceeded */
i = SIGFPT;
break;

case 1+USER: /* debug exception */
case 3+USER: /* breakpoint exception */
i = SIGTRC;
tr.efl &= ~TBIT;
break;

case 6+USER: /* invalid opcode */
case 9+USER: /* coprocessor segment overrun */
i = SIGINS;
break;

case 7+USER: /* device not available */
panic("fpu not available"); /* XXX */
break;

case 8+USER: /* double fault */
case 10+USER: /* invalid tss */
case 11+USER: /* segment not present */
case 12+USER: /* stack fault exception */
case 13+USER: /* general protection exception */
i = SIGBUS;
break;

/*
 * If the user SP is below the stack segment,
 * grow the stack automatically.
 */
case 14+USER: /* page fault */
osp = tr.esp;
if (grow((unsigned)osp))
goto out;
i = SIGSEG;
break;

/*
 * Allow process switch
 */
case 15+USER:
goto out;

/*
 * Since the floating exception is an
 * imprecise trap, a user generated
 * trap may actually come from kernel
 * mode. In this case, a signal is sent
 * to the current process to be picked
 * up later.
 */
case 16: /* floating point error */
stst(&u.u_fper);	/* save error code */
psignal(u.u_procp, SIGFPT);
return;

case 16+USER:
i = SIGFPT;
stst(&u.u_fper);
break;

case 48+USER: /* sys call */
u.u_error = 0;
tr.efl &= ~EBIT;
a = (int *)(tr.esp + 4);
callp = &sysent[tr.eax & 077];
for(i = 0; i<callp->sy_narg; i++)
u.u_arg[i] = fuword((caddr_t)a++);
u.u_dirp = (caddr_t)u.u_arg[0];
u.u_r.r_val1 = 0;
u.u_r.r_val2 = 0;
u.u_ap = u.u_arg;
if (save(u.u_qsav)) {
if (u.u_error==0)
u.u_error = EINTR;
} else {
(*callp->sy_call)();
}
if(u.u_error) {
tr.efl |= EBIT;
u.u_ar0[EAX] = u.u_error;
} else {
u.u_ar0[EAX] = u.u_r.r_val1;
u.u_ar0[EDX] = u.u_r.r_val2;
}
goto out;
}
/*
 * If you run elaborate /bin/sh scripts, you'll
 * probably want to disable the following line.
 */
printf("** SIGNAL %d **\n", i);
psignal(u.u_procp, i);

out:
if(issig()) {
psig();
}
curpri = setpri(u.u_procp);
if (runrun)
qswtch();
if(u.u_prof.pr_scale)
addupc((caddr_t)tr.eip, &u.u_prof, (int)(u.u_stime-syst));
if (u.u_fpsaved)
restfp(&u.u_fps);
}

/*
 * stray interrupt
 */
stray(dev)
        {
                printf("stray interrupt %d ignored\n", dev & ~0x20);
        }

/*
 * nonexistent system call-- set fatal error code.
 */
nosys()
{
    u.u_error = EINVAL;
}

/*
 * Ignored system call
 */
nullsys()
{
}
