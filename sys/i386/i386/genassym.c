/*-
 * Copyright (c) 1982, 1990, 1993
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
 *	@(#)genassym.c	8.2 (Berkeley) 9/23/93
 */

#ifndef lint
static char sccsid[] = "@(#)genassym.c	8.2 (Berkeley) 9/23/93";
#endif /* not lint */

#define KERNEL
#include <sys/param.h>
#include <sys/"user.h>
#include <sys/inode.h>
#include <sys/mbuf.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/reboot.h>
#include <sys/syscall.h>
#include <sys/vm.h>

#include <machine/cpu.h>
#include <machine/trap.h>
#include <machine/psl.h>
#include <machine/reg.h>

#include <stdio.h>

struct proc proc[1];		/* satisfy proc.h and inode.h */
struct inode inode[1];
struct buf buf[1];

main()
{
	printf("#define\tI386_CR3PAT %d\n", I386_CR3PAT);

    {
        struct user *u = (struct user *)0;

        printf("#define U_STACK %o\n",u->u_stack);
        printf("#define U_SSIZE %o\n",&u->u_ssize);
    }

    {
        struct pcb *pcb = (struct pcb *)0;

        printf("#define\tPCB_CR0 %d\n", &pcb->pcb_tss.tss_cr0);
        printf("#define\tPCB_CR2 %d\n", &pcb->pcb_tss.tss_cr2);
        printf("#define\tPCB_CR3 %d\n", &pcb->pcb_tss.tss_cr3);
        printf("#define\tPCB_CR4 %d\n", &pcb->pcb_tss.tss_cr4);
	    printf("#define\tPCB_EIP %d\n", &pcb->pcb_tss.tss_eip);
	    printf("#define\tPCB_EFLAGS %d\n", &pcb->pcb_tss.tss_eflags);
	    printf("#define\tPCB_EAX %d\n", &pcb->pcb_tss.tss_eax);
	    printf("#define\tPCB_ECX %d\n", &pcb->pcb_tss.tss_ecx);
	    printf("#define\tPCB_EDX %d\n", &pcb->pcb_tss.tss_edx);
	    printf("#define\tPCB_EBX %d\n", &pcb->pcb_tss.tss_ebx);
	    printf("#define\tPCB_ESP %d\n", &pcb->pcb_tss.tss_esp);
	    printf("#define\tPCB_EBP %d\n", &pcb->pcb_tss.tss_ebp);
	    printf("#define\tPCB_ESI %d\n", &pcb->pcb_tss.tss_esi);
	    printf("#define\tPCB_EDI %d\n", &pcb->pcb_tss.tss_edi);
	    printf("#define\tPCB_ES %d\n", &pcb->pcb_tss.tss_es);
	    printf("#define\tPCB_CS %d\n", &pcb->pcb_tss.tss_cs);
	    printf("#define\tPCB_SS %d\n", &pcb->pcb_tss.tss_ss);
	    printf("#define\tPCB_DS %d\n", &pcb->pcb_tss.tss_ds);
	    printf("#define\tPCB_FS %d\n", &pcb->pcb_tss.tss_fs);
	    printf("#define\tPCB_GS %d\n", &pcb->pcb_tss.tss_gs);
	    printf("#define\tPCB_LDT %d\n", &pcb->pcb_tss.tss_ldt);
        printf("#define\tPCB_FLAGS %d\n", &pcb->pcb_flags);
	    printf("#define\tPCB_SAVEFPU %d\n", &pcb->pcb_savefpu);
        printf("#define\tPCB_SAVEEMC %d\n", &pcb->pcb_saveemc);
	    printf("#define\tPCB_CMAP2 %d\n", &pcb->pcb_cmap2);
	    printf("#define\tPCB_SIGC %d\n", pcb->pcb_sigc);
	    printf("#define\tPCB_IML %d\n", &pcb->pcb_iml);
	    printf("#define\tPCB_ONFAULT %d\n", &pcb->pcb_onfault);
    }

    struct vmmeter *vm = (struct vmmeter *)0;

}