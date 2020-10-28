/*	$NetBSD: cpufunc.h,v 1.8 1994/10/27 04:15:59 cgd Exp $	*/

/*
 * Copyright (c) 1993 Charles Hannum.
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
 *      This product includes software developed by Charles Hannum.
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

#ifndef _I386_CPUFUNC_H_
#define	_I386_CPUFUNC_H_

/*
 * Functions to provide access to i386-specific instructions.
 */

#include <sys/cdefs.h>
#include <sys/types.h>

#include <machine/pcb.h>

/*
 * Flush MMU TLB
 */
#ifndef I386_CR3PAT
#define	I386_CR3PAT	0x0
#endif

static __inline int
bdb(void)
{
	extern int bdb_exists;

	if (!bdb_exists)
		return (0);
	__asm __volatile("int $3");
	return (1);
}

static __inline void
lidt(void *p)
{
	__asm __volatile("lidt (%0)" : : "r" (p));
}

static __inline void
lldt(u_short sel)
{
	__asm __volatile("lldt %0" : : "r" (sel));
}

static __inline void
ltr(u_short sel)
{
	__asm __volatile("ltr %0" : : "r" (sel));
}

static __inline void
invd(void)
{
	__asm __volatile("invd");
}

static __inline void
load_fs(u_short sel)
{
	__asm __volatile("movw %0,%%fs" : : "rm" (sel));
}

static __inline void
load_gs(u_short sel)
{
	__asm __volatile("movw %0,%%gs" : : "rm" (sel));
}

static __inline uint64_t
rdmsr(u_int msr)
{
	uint64_t rv;

	__asm __volatile("rdmsr" : "=A" (rv) : "c" (msr));
	return (rv);
}

static __inline void
wbinvd(void)
{
	__asm __volatile("wbinvd");
}

static __inline void
wrmsr(u_int msr, uint64_t newval)
{
	__asm __volatile("wrmsr" : : "A" (newval), "c" (msr));
}

/* load cr0 */
static __inline void
lcr0(u_int val)
{
	__asm __volatile("movl %0,%%cr0" : : "r" (val));
}

static __inline u_int
rcr0(void)
{
	u_int val;
	__asm __volatile("movl %%cr0,%0" : "=r" (val));
	return val;
}

static __inline u_int
rcr2(void)
{
	u_int val;
	__asm __volatile("movl %%cr2,%0" : "=r" (val));
	return val;
}

/* load cr3 */
static __inline void
lcr3(u_int val)
{
	__asm __volatile("movl %0,%%cr3" : : "r" (val));
}

static __inline u_int
rcr3(void)
{
	u_int val;
	__asm __volatile("movl %%cr3,%0" : "=r" (val));
	return val;
}

/* load cr4 */
static __inline void
lcr4(u_int data)
{
	__asm __volatile("movl %0,%%cr4" : : "r" (data));
}

static __inline u_int
rcr4(void)
{
	u_int	data;

	__asm __volatile("movl %%cr4,%0" : "=r" (data));
	return (data);
}

static __inline u_short
rgs(void)
{
	u_short sel;
	__asm __volatile("movw %%gs,%0" : "=rm" (sel));
	return (sel);
}

static __inline void
tlbflush(void)
{
	u_int val;
	__asm __volatile("movl %%cr3,%0" : "=r" (val));
	__asm __volatile("movl %0,%%cr3" : : "r" (val));
}

#ifdef notyet
static __inline void
_cr3(void)
{
	u_long rtn;
	__asm __volatile(" movl %%cr3,%%eax; movl %%eax,%0 " : "=g" (rtn) :	: "ax");
}

static __inline void
load_cr3(u_long s)
{
	u_long val = (s) | I386_CR3PAT;
	__asm __volatile("movl %0,%%eax; movl %%eax,%%cr3" : : "g" (val) : "ax");
}

static __inline void
tlbflush(void)
{
	u_long val = u->u_pcb.pcb_ptd | I386_CR3PAT;
	__asm __volatile("movl %0,%%eax; movl %%eax,%%cr3" : : "g" (val) : "ax");
}

void	setidt	(int idx, /*XXX*/caddr_t func, int typ, int dpl);
#endif

/* XXXX ought to be in psl.h with spl() functions */
static __inline void
disable_intr(void)
{
	__asm __volatile("cli");
}

static __inline void
enable_intr(void)
{
	__asm __volatile("sti");
}

#ifdef _KERNEL
static __inline void
do_cpuid(u_int ax, u_int *p)
{
	__asm __volatile("cpuid"
	    : "=a" (p[0]), "=b" (p[1]), "=c" (p[2]), "=d" (p[3])
	    :  "0" (ax));
}

#else
static __inline void
do_cpuid(u_int ax, u_int *p)
{
	__asm __volatile(
	    "pushl\t%%ebx\n\t"
	    "cpuid\n\t"
	    "movl\t%%ebx,%1\n\t"
	    "popl\t%%ebx"
	    : "=a" (p[0]), "=DS" (p[1]), "=c" (p[2]), "=d" (p[3])
	    :  "0" (ax));
}
#endif /* _KERNEL */

static __inline u_int
read_eflags(void)
{
	u_int	ef;

	__asm __volatile("pushfl; popl %0" : "=r" (ef));
	return (ef);
}

static __inline void
write_eflags(u_int ef)
{
	__asm __volatile("pushl %0; popfl" : : "r" (ef));
}

static __inline register_t
intr_disable(void)
{
	register_t eflags;

	eflags = read_eflags();
	disable_intr();
	return (eflags);
}

static __inline void
intr_restore(register_t eflags)
{
	write_eflags(eflags);
}

static __inline u_char
read_cyrix_reg(u_char reg)
{
	outb(0x22, reg);
	return inb(0x23);
}

static __inline void
write_cyrix_reg(u_char reg, u_char data)
{
	outb(0x22, reg);
	outb(0x23, data);
}

#endif /* !_I386_CPUFUNC_H_ */
