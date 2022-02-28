/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the University of Utah, and William Jolitz.
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
 *	@(#)trap.c	8.4 (Berkeley) 9/23/93
 */
/*	$NetBSD: trap.c,v 1.307 2020/09/05 07:26:37 maxv Exp $	*/
/*-
 * Copyright (c) 1998, 2000, 2005, 2006, 2007, 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * 386 Trap and System call handling
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/acct.h>
#include <sys/kernel.h>
#include <sys/endian.h>
#ifdef KTRACE
#include <sys/ktrace.h>
#endif

#include <vm/include/pmap.h>
#include <vm/include/vm_param.h>
#include <vm/include/vm_map.h>

#include <dev/core/isa/isareg.h>
#include <dev/core/isa/isavar.h>

#include <machine/vmparam.h>
#include <machine/trap.h>
#include <machine/psl.h>
#include <machine/pcb.h>
#include <machine/pte.h>
#include <machine/proc.h>
#include <machine/reg.h>

#ifdef DEBUG
int	trapdebug = 0;
#endif

struct sysent sysent[];
int	nsysent;
unsigned rcr2();
extern int cpl;

void trap(struct trapframe *);
void trap_tss(struct i386tss *, int, int);
static __inline void userret(struct proc *, int, u_quad_t);

const char * const trap_type[] = {
	"privileged instruction fault",		/*  0 T_PRIVINFLT */
	"breakpoint trap",					/*  1 T_BPTFLT */
	"arithmetic trap",					/*  2 T_ARITHTRAP */
	"asynchronous system trap",			/*  3 T_ASTFLT */
	"protection fault",					/*  4 T_PROTFLT */
	"trace trap",						/*  5 T_TRCTRAP */
	"page fault",						/*  6 T_PAGEFLT */
	"alignment fault",					/*  7 T_ALIGNFLT */
	"integer divide fault",				/*  8 T_DIVIDE */
	"non-maskable interrupt",			/*  9 T_NMI */
	"overflow trap",					/* 10 T_OFLOW */
	"bounds check fault",				/* 11 T_BOUND */
	"FPU not available fault",			/* 12 T_DNA */
	"double fault",						/* 13 T_DOUBLEFLT */
	"FPU operand fetch fault",			/* 14 T_FPOPFLT */
	"invalid TSS fault",				/* 15 T_TSSFLT */
	"segment not present fault",		/* 16 T_SEGNPFLT */
	"stack fault",						/* 17 T_STKFLT */
	"machine check fault",				/* 18 T_MCA */
	"SSE FP exception",					/* 19 T_XMM */
	"reserved trap",					/* 20 T_RESERVED */
};

void
trap_tss(tss, trapno, code)
	struct i386tss *tss;
	int trapno, code;
{
	struct trapframe tf;

	tf.tf_gs = tss->tss_gs;
	tf.tf_fs = tss->tss_fs;
	tf.tf_es = tss->tss_es;
	tf.tf_ds = tss->tss_ds;
	tf.tf_edi = tss->tss_edi;
	tf.tf_esi = tss->tss_esi;
	tf.tf_ebp = tss->tss_ebp;
	tf.tf_ebx = tss->tss_ebx;
	tf.tf_edx = tss->tss_edx;
	tf.tf_ecx = tss->tss_ecx;
	tf.tf_eax = tss->tss_eax;
	tf.tf_trapno = trapno;
	tf.tf_err = code | TC_TSS;
	tf.tf_eip = tss->tss_eip;
	tf.tf_cs = tss->tss_cs;
	tf.tf_eflags = tss->tss_eflags;
	tf.tf_esp = tss->tss_esp;
	tf.tf_ss = tss->tss_ss;
	trap(&tf);
}

static void *
onfault_handler(pcb, tf)
	const struct pcb *pcb;
	const struct trapframe *tf;
{
	struct onfault_table {
		uintptr_t 	start;
		uintptr_t 	end;
		void 		*handler;
	};
	extern const struct onfault_table onfault_table[];
	const struct onfault_table *p;
	uintptr_t pc;

	if (pcb->pcb_onfault != NULL) {
		return (pcb->pcb_onfault);
	}

	pc = tf->tf_eip;
	for (p = onfault_table; p->start; p++) {
		if (p->start <= pc && pc < p->end) {
			return (p->handler);
		}
	}
	return (NULL);
}

static void
trap_print(frame, p)
	const struct trapframe *frame;
	const struct proc *p;
{
	const int type = frame->tf_trapno;

	if (frame->tf_trapno < trap_types) {
		printf("fatal %s", trap_type[type]);
	} else {
		printf("unknown trap %d", type);
	}
	printf(" in %s mode\n", (type & T_USER) ? "user" : "supervisor");

	printf("trap type %d code %#x eip %#x cs %#x eflags %#x cr2 %#lx esp %#x\n", type, frame->tf_err, frame->tf_eip, frame->tf_cs, frame->tf_eflags, (long) rcr2(), frame->tf_esp);
	printf("cr2 %x cpl %x\n", rcr2(), cpl);
}

static __inline void
userret(p, pc, oticks)
	register struct proc *p;
	int pc;
	u_quad_t oticks;
{
	int sig, s;

	/* take pending signals */
	while ((sig = CURSIG(p)) != 0) {
		postsig(sig);
	}
	p->p_pri = setpri(p);
	if (want_resched(p)) {
		/*
		 * Since we are curproc, clock will normally just change
		 * our priority without moving us from one queue to another
		 * (since the running process is not on a queue.)
		 * If that happened after we put ourselves on the run queue
		 * but before we switched, we might not be on the queue
		 * indicated by our priority.
		 */
		s = splstatclock();
		setrq(p);
		p->p_stats->p_ru.ru_nivcsw++;
		swtch();
		splx(s);
		while ((sig = CURSIG(p)) != 0) {
			postsig(sig);
		}
	}

	/*
	 * If profiling, charge recent system time to the trapped pc.
	 */
	if (p->p_flag & P_PROFIL) {
		extern int psratio;
		addupc_task(p, pc, (int)(p->p_sticks - oticks) * psratio);
	}
	curpri = p->p_pri;
}

/*
 * trap(frame):
 *	Exception, fault, and trap interface to BSD kernel. This
 * common code is called from assembly language IDT gate entry
 * routines that prepare a suitable stack frame, and restore this
 * frame after the exception has been processed. Note that the
 * effect is as if the arguments were passed call by reference.
 */
void
trap(frame)
	struct trapframe frame;
{
	register struct proc *p = curproc;
	struct pcb *pcb;
	register int i;
	u_quad_t sticks;
	void *onfault;
	int type, error = 0;
	int ucode;
	uint32_t cr2;
	bool pfail;
	extern int cold;

	if (__predict_true(p != NULL)) {
		pcb = curpcb;
	} else {
		pcb = NULL;
	}
	type = frame->tf_trapno;
#ifdef DEBUG
	if (trapdebug) {
		trap_print(frame, p);
	}
#endif
	if ((type != T_NMI && ISPL(frame->tf_cs) == SEL_UPL) || ((frame->tf_eflags & PSL_VM) && (!pcb->pcb_flags & PCB_VM86CALL))) {
		type |= T_USER;
		p->p_md.md_regs = frame;
		pcb->pcb_cr2 = 0;
		sticks = p->p_sticks;
	}

	ucode = 0;

	switch (type) {
	default:
	we_re_toast:

		if (kdb_trap(type, 0, frame)) {
			return;
		}

		trap_print(frame, p);

		panic("trap");
		/*NOTREACHED*/

	case T_PROTFLT:				/* general protection fault */
	case T_SEGNPFLT:
	case T_ALIGNFLT:
	case T_STKFLT:				/* stack fault */
		if (frame->tf_eflags & PSL_VM) {
			i = vm86_emulate((struct vm86frame *)&frame);
			if (i != 0) {
				/*
				 * returns to original process
				 */
				vm86_trap((struct vm86frame *)&frame);
				goto out;
			}
			break;
		}
		break;
		/* FALLTHROUGH */

	case T_TSSFLT:						/* invalid TSS fault */
		if (p == NULL) {
			goto we_re_toast;
		}
		/* Check for copyin/copyout fault. */
		onfault = onfault_handler(pcb, frame);
		if (onfault != NULL) {
copyefault:
			error = EFAULT;
copyfault:
			frame->tf_eip = (uintptr_t) onfault;
			frame->tf_eax = error;
			return;
		}
		break;

	case T_DOUBLEFLT:				/* double fault */
		ucode = frame->tf_err + BUS_SEGM_FAULT;
		i = SIGBUS;
		break;

	case T_SEGNPFLT|T_USER:
	case T_STKFLT|T_USER:			/* 386bsd */
	case T_PROTFLT|T_USER:			/* protection fault */
		ucode = frame->tf_err + BUS_SEGM_FAULT;
		i = SIGBUS;
		break;

	case T_ALIGNFLT|T_USER:
	case T_PRIVINFLT|T_USER:		/* privileged instruction fault */
	case T_RESADFLT|T_USER:			/* reserved addressing fault */
	case T_RESOPFLT|T_USER:			/* reserved operand fault */
	case T_FPOPFLT|T_USER:
		/* coprocessor operand fault */
		ucode = type &~ T_USER;
		i = SIGILL;
		break;

	case T_ASTFLT|T_USER:
		if(p->p_pflag & MDP_OWEUPC) {
			p->p_pflag &= ~MDP_OWEUPC;
			ADDUPROF(p);
		}
		goto out;
	case T_ASTFLT:
	case T_DNA|T_USER:
		ucode = FPE_FPU_NP_TRAP;
		i = SIGFPE;
		break;

	case T_BOUND|T_USER:
		ucode = FPE_SUBRNG_TRAP;
		i = SIGFPE;
		break;

	case T_OFLOW|T_USER:
		ucode = FPE_INTOVF_TRAP;
		i = SIGFPE;
		break;

	case T_DIVIDE|T_USER:
		ucode = FPE_INTDIV_TRAP;
		i = SIGFPE;
		break;

	case T_PAGEFLT:
		/* allow page faults in kernel mode */
#if defined(I586_CPU) && !defined(NO_F00F_HACK)
		if (i == -2) {
			/*
			 * The f00f hack workaround has triggered, so
			 * treat the fault as an illegal instruction
			 * (T_PRIVINFLT) instead of a page fault.
			 */
			type = frame->tf_trapno = T_PRIVINFLT;

			/* Proceed as in that case. */
			ucode = type;
			i = SIGILL;
			break;
		}
#endif
		if (__predict_false(p == NULL)) {
			goto we_re_toast;
		}

		cr2 = rcr2();

		if (frame->tf_err & PGEX_I) {
			/* SMEP might have brought us here */
			if (cr2 > VM_MIN_ADDRESS && cr2 <= VM_MAXUSER_ADDRESS) {
				printf("prevented execution of %p (SMEP)\n", (void *)cr2);
				goto we_re_toast;
			}
		}


		if ((frame->tf_err & PGEX_P) && cr2 < VM_MAXUSER_ADDRESS) {
			/* SMAP might have brought us here */
			if (onfault_handler(pcb, frame) == NULL) {
				printf("prevented access to %p (SMAP)\n", (void *)cr2);
				goto we_re_toast;
			}
		}

		goto faultcommon;
		/* fall into */

	case T_PAGEFLT|T_USER:
		/* page fault */
		register vm_offset_t va;
		register struct vmspace *vm;
		register vm_map_t map;
		int rv;
		vm_prot_t ftype;
		extern vm_map_t kernel_map;

		cr2 = rcr2();

faultcommon:
		vm = p->p_vmspace;
		if (__predict_false(vm == NULL)) {
			goto we_re_toast;
		}
		pcb->pcb_cr2 = cr2;
		va = trunc_page((vm_offset_t)cr2);
		/*
		 * It is only a kernel address space fault iff:
		 * 	1. (type & T_USER) == 0  and
		 * 	2. pcb_onfault not set or
		 *	3. pcb_onfault set but supervisor space fault
		 * The last can occur during an exec() copyin where the
		 * argument space is lazy-allocated.
		 */
		if (type == T_PAGEFLT && va >= VM_MIN_KERNEL_ADDRESS) {
			map = kernel_map;
		} else {
			map = &vm->vm_map;
		}
		if (frame->tf_err & PGEX_W) {
			ftype = VM_PROT_WRITE;
		} else if (frame->tf_err & PGEX_I) {
			ftype = VM_PROT_EXECUTE;
		} else {
			ftype = VM_PROT_READ;
		}

		rv = user_page_fault(p, map, va, ftype, type);
		if (rv == KERN_SUCCESS) {
			if (type == T_PAGEFLT) {
				return;
			}
			goto out;
		}
#ifdef DIAGNOSTIC
		if (map == kernel_map && va == 0) {
			printf("trap: bad kernel access at %lx\n", va);
			goto we_re_toast;
		}
#endif
		/* Fault the original page in. */
		onfault = pcb->pcb_onfault;
		pcb->pcb_onfault = NULL;
		error = vm_fault(map, va, ftype, FALSE);
		pcb->pcb_onfault = onfault;
		if (error == 0) {
			if (map != kernel_map && (void *)va >= vm->vm_maxsaddr) {
				grow(p, va);
			}
			pfail = FALSE;
			goto out;
		}

		if (type == T_PAGEFLT) {
			onfault = onfault_handler(pcb, frame);
			if (onfault != NULL) {
				goto copyfault;
			}
			printf("vm_fault(%x, %x, %x, 0) -> %x\n", map, va, ftype, rv);
			printf("type %x, code %x\n", type, frame->tf_err);
			goto we_re_toast;
		}
		i = (rv == KERN_PROTECTION_FAILURE) ? SIGBUS : SIGSEGV;
		break;

	case T_TRCTRAP:
		frame->tf_eflags &= ~PSL_T;

		return;
	case T_BPTFLT|T_USER:		/* bpt instruction fault */
	case T_TRCTRAP|T_USER:		/* trace trap */
		frame->tf_eflags &= ~PSL_T;
		i = SIGTRAP;
		break;

#if	NISA > 0
	case T_NMI:
		if (kdb_trap(type, 0, frame)) {
			return;
		}

	case T_NMI|T_USER:
		/* machine/parity/power fail/"kitchen sink" faults */
		if(isa_nmi(frame->tf_err) == 0) {
			return;
		} else {
			goto we_re_toast;
		}
#endif
	}

	trapsignal(p, i, ucode);
	if ((type & T_USER) == 0) {
		return;
	}

out:
	curpri = p->p_pri;
	userret(p, frame->tf_eip, sticks);
	curpcb->pcb_flags &= ~FM_TRAP;			/* used by sendsig */
}

/*
 * Compensate for 386 brain damage (missing URKR).
 * This is a little simpler than the pagefault handler in trap() because
 * it the page tables have already been faulted in and high addresses
 * are thrown out early for other reasons.
 */
int
trapwrite(addr)
	unsigned addr;
{
	struct proc *p;
	vm_offset_t va, v;
	struct vmspace *vm;
	int rv;

	va = trunc_page((vm_offset_t)addr);
	/*
	 * XXX - MAX is END.  Changed > to >= for temp. fix.
	 */
	if (va >= VM_MAXUSER_ADDRESS)
		return (1);

	p = curproc;
	vm = p->p_vmspace;

	++p->p_lock;

	if ((caddr_t) va >= vm->vm_maxsaddr && (caddr_t) va < (caddr_t) USRSTACK) {
		if (!grow(p, va)) {
			--p->p_lock;
			return (1);
		}
	}

	v = trunc_page(vtopte(va));

	/*
	 * wire the pte page
	 */
	if (va < USRSTACK) {
		vm_map_pageable(&vm->vm_map, v, round_page(v + 1), FALSE);
	}

	/*
	 * fault the data page
	 */
	rv = vm_fault(&vm->vm_map, va, VM_PROT_READ | VM_PROT_WRITE, FALSE);

	/*
	 * unwire the pte page
	 */
	if (va < USRSTACK) {
		vm_map_pageable(&vm->vm_map, v, round_page(v + 1), TRUE);
	}

	--p->p_lock;

	if (rv != KERN_SUCCESS) {
		return 1;
	}

	return (0);
}

/*
 * syscall(frame):
 *	System call request from POSIX system call gate interface to kernel.
 * Like trap(), argument is call by reference.
 */
/*ARGSUSED*/
void
syscall(frame)
    struct trapframe *frame;
{
    register caddr_t params;
	register int i;
    register struct sysent *callp;
    register struct proc *p;
    u_quad_t sticks;
    int error, nsys, opc;
    register_t code;
    int args[8], rval[2];
    short argsize;

    sticks = p->p_sticks;
	if (ISPL(frame->tf_cs) != SEL_UPL) {
		panic("syscall");
	}

    p = curproc;
    p->p_md.md_regs = frame;
    opc = frame->tf_eip;
    code = frame->tf_eax;

    nsys =  p->p_emul->e_nsysent;
    callp = p->p_emul->e_sysent;

    curpcb->pcb_flags &= ~FM_TRAP;	                /* used by sendsig */
	params = (caddr_t)frame->tf_esp + sizeof (int);

    if (callp == sysent) {
    	code = fuword(params + _QUAD_LOWWORD * sizeof(int));
    	params += sizeof(quad_t);
		break;
    }
    if (code < 0 || code >= nsys) {
        callp += p->p_emul->e_nosys;
    } else {
        callp += code;
    }
    argsize = callp->sy_argsize;
    if(argsize && (error = copyin(params, args, argsize))) {
        frame->tf_eax = error;
		frame->tf_eflags |= PSL_C;	/* carry bit */
#ifdef KTRACE
		if (KTRPOINT(p, KTR_SYSCALL))
			ktrsyscall(p->p_tracep, code, callp->sy_narg, &args);
#endif
        goto done;
    }

#ifdef KTRACE
	if (KTRPOINT(p, KTR_SYSCALL))
		ktrsyscall(p->p_tracep, code, callp->sy_narg, &args);
#endif
    rval[0] = 0;
	rval[1] = frame->tf_edx;

    error = (*callp->sy_call)(p, args, rval);
    if (error == ERESTART) {
		frame->tf_eip = opc;
    } else if (error != EJUSTRETURN) {
		if (error) {
			frame->tf_eax = error;
			frame->tf_eflags |= PSL_C;	/* carry bit */
		} else {
			frame->tf_eax = rval[0];
			frame->tf_edx = rval[1];
			frame->tf_eflags &= ~PSL_C;	/* carry bit */
		}
	}

done:
	/*
	 * Reinitialize proc pointer `p' as it may be different
	 * if this is a child returning from fork syscall.
	 */
	p = curproc;
	userret(p, frame->tf_eip, sticks);
#ifdef KTRACE
	if (KTRPOINT(p, KTR_SYSRET)) {
		ktrsysret(p->p_tracep, code, error, rval[0]);
	}
#endif
}

int
user_page_fault(p, map, addr, ftype, type)
	struct proc *p;
	vm_map_t map;
	caddr_t addr;
	vm_prot_t ftype;
	int type;
{
	struct vmspace *vm;
	vm_offset_t va;
	int rv;
	extern vm_map_t kernel_map;
	unsigned nss, v;

	vm = p->p_vmspace;

	va = trunc_page((vm_offset_t)addr);

	/*
	 * XXX: rude hack to make stack limits "work"
	 */
	nss = 0;
	if ((caddr_t)va >= vm->vm_maxsaddr && map != kernel_map) {
		nss = clrnd(btoc(USRSTACK - (unsigned)va));
		if (nss > btoc(p->p_rlimit[RLIMIT_STACK].rlim_cur)) {
			return (KERN_FAILURE);
		}
	}

	/* check if page table is mapped, if not, fault it first */
	v = trunc_page(vtopte(va));
	if ((rv = vm_fault(map, v, ftype, FALSE)) != KERN_SUCCESS) {
		return (rv);
	}

	/* check if page table fault, increment wiring */
	vm_map_pageable(map, v, round_page(v+1), FALSE);

	if ((rv = vm_fault(map, va, ftype, FALSE)) != KERN_SUCCESS) {
		return (rv);
	}

	/*
	 * XXX: continuation of rude stack hack
	 */
	if (nss > vm->vm_ssize) {
		vm->vm_ssize = nss;
	}
	va = trunc_page(vtopte(va));
	/*
	 * for page table, increment wiring
	 * as long as not a page table fault as well
	 */
	if (!v && type != T_PAGEFLT) {
		vm_map_pageable(map, va, round_page(va+1), FALSE);
	}
	return (KERN_SUCCESS);
}

int
user_write_fault (addr)
	void *addr;
{
	if (user_page_fault (curproc, &curproc->p_vmspace->vm_map, addr, VM_PROT_READ | VM_PROT_WRITE, T_PAGEFLT) == KERN_SUCCESS) {
		return (0);
	} else {
		return (EFAULT);
	}
}

void
child_return(arg)
	void *arg;
{
    struct proc *p = (struct proc *)arg;
	struct trapframe *tf = p->p_md.md_regs;

	tf->tf_eax = 0;
	tf->tf_eflags &= ~PSL_C;
	userret(p, tf->tf_eip, 0);
#ifdef KTRACE
	if (KTRPOINT(p, KTR_SYSRET))
		ktrsysret(p->p_tracep, SYS_fork, 0, 0);
#endif
}
