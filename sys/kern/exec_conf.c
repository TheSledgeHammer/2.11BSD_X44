/*	$NetBSD: exec_conf.c,v 1.93 2006/08/30 14:41:06 cube Exp $	*/

/*
 * Copyright (c) 1993, 1994 Christopher G. Demetriou
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
 *      This product includes software developed by Christopher G. Demetriou.
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
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/exec_linker.h>
#include <sys/exec.h>

#ifdef EXEC_SCRIPT
#include <sys/exec_script.h>
#endif
#ifdef EXEC_AOUT
#include <sys/exec_aout.h>
#endif
#ifdef EXEC_COFF
#include <sys/exec_coff.h>
#endif
#ifdef EXEC_ECOFF
#include <sys/exec_ecoff.h>
#endif
#ifdef EXEC_MACHO
#include <sys/exec_macho.h>
#endif
#ifdef EXEC_PECOFF
#include <sys/exec_pecoff.h>
#endif
#ifdef EXEC_ELF
#include <sys/exec_elf.h>
#endif
#ifdef EXEC_XCOFF
#include <sys/exec_xcoff.h>
#endif

#ifdef SYSCALL_DEBUG
extern char *syscallnames[];
#endif

const struct execsw execsw[] = {
		/* shell scripts */
#ifdef EXEC_SCRIPT
		{
				.ex_hdrsz = SCRIPT_HDR_SIZE,
				.ex_makecmds = exec_script_linker,
				.ex_emul = NULL,
				.ex_prio = EXECSW_PRIO_ANY,
				.ex_arglen = 0,
				.ex_copyargs = NULL,
				.ex_setup_stack = exec_setup_stack
		},
#endif
		/* a.out binaries */
#ifdef EXEC_AOUT
		{
				.ex_hdrsz = AOUT_HDR_SIZE,
				.ex_makecmds = exec_aout_linker,
				.ex_emul = &emul_211bsd,
				.ex_prio = EXECSW_PRIO_FIRST,
				.ex_arglen = 0,
				.ex_copyargs = copyargs,
				.ex_setup_stack = exec_setup_stack
		},
#endif
		/* coff binaries */
#ifdef EXEC_COFF
		{
				.ex_hdrsz = COFF_HDR_SIZE,
				.ex_makecmds = exec_coff_linker,
				.ex_emul = &emul_211bsd,
				.ex_prio = EXECSW_PRIO_ANY,
				.ex_arglen = 0,
				.ex_copyargs = copyargs,
				.ex_setup_stack = exec_setup_stack
		},
#endif
		/* ecoff binaries */
#ifdef EXEC_ECOFF
		{
				.ex_hdrsz = ECOFF_HDR_SIZE,
				.ex_makecmds = exec_ecoff_linker,
				.ex_emul = &emul_211bsd,
				.ex_prio = EXECSW_PRIO_ANY,
				.ex_arglen = 0,
				.ex_copyargs = copyargs,
				.ex_setup_stack = exec_setup_stack
		},
#endif
		/* pecoff binaries */
#ifdef EXEC_PECOFF
		{
				.ex_hdrsz = PECOFF_HDR_SIZE,
				.ex_makecmds = exec_pecoff_linker,
				.ex_emul = &emul_211bsd,
				.ex_prio = EXECSW_PRIO_ANY,
				.ex_arglen = PECOFF_AUXSIZE,
				.ex_copyargs = pecoff_copyargs,
				.ex_setup_stack = exec_setup_stack
		},
#endif
		/* mach-o binaries */
#ifdef EXEC_MACHO
		{
				.ex_hdrsz = MACHO_HDR_SIZE,
				.ex_makecmds = exec_macho_linker,
				.ex_emul = &emul_211bsd,
				.ex_prio = EXECSW_PRIO_ANY,
				.ex_arglen = MACHO_AUXSIZE,
				.ex_copyargs = macho_copyargs,
				.ex_setup_stack = exec_setup_stack
		},
#endif
		/* 32-Bit ELF binaries */
#ifdef EXEC_ELF
#if defined(ELFSIZE) && (ELFSIZE == 32)
		{
				.ex_hdrsz = ELF32_HDR_SIZE,
				.ex_makecmds = exec_elf_linker,
				.ex_emul = &emul_211bsd,
				.ex_prio = EXECSW_PRIO_ANY,
				.ex_arglen = ELF32_AUXSIZE,
				.ex_copyargs = elf_copyargs,
				.ex_setup_stack = exec_setup_stack
		},
#endif
		/* 64-Bit ELF binaries */
#if defined(ELFSIZE) && (ELFSIZE == 64)
		{
				.ex_hdrsz = ELF64_HDR_SIZE,
				.ex_makecmds = exec_elf_linker,
				.ex_emul = &emul_211bsd,
				.ex_prio = EXECSW_PRIO_ANY,
				.ex_arglen = ELF64_AUXSIZE,
				.ex_copyargs = elf_copyargs,
				.ex_setup_stack = exec_setup_stack
		},
#endif
#endif
		/* 32-Bit xcoff binaries */
#ifdef EXEC_XCOFF
#if defined(XCOFFSIZE) && (XCOFFSIZE == 32)
		{
				.ex_hdrsz = XCOFF32_HDR_SIZE,
				.ex_makecmds = exec_xcoff_linker,
				.ex_emul = &emul_211bsd,
				.ex_prio = EXECSW_PRIO_ANY,
				.ex_arglen = 0,
				.ex_copyargs = copyargs,
				.ex_setup_stack = exec_setup_stack
		},
#endif
		/* 64-Bit xcoff binaries */
#if defined(XCOFFSIZE) && (XCOFFSIZE == 64)
		{
				.ex_hdrsz = XCOFF64_HDR_SIZE,
				.ex_makecmds = exec_xcoff_linker,
				.ex_emul = &emul_211bsd,
				.ex_prio = EXECSW_PRIO_ANY,
				.ex_arglen = 0,
				.ex_copyargs = copyargs,
				.ex_setup_stack = exec_setup_stack
		},
#endif
#endif
};

int nexecs = (sizeof execsw / sizeof(struct execsw));
int exec_maxhdrsz;

static void
link_exec(listp, exp)
	struct execsw_entry **listp;
	const struct execsw *exp;
{
	struct execsw_entry *et, *e1;

	et = (struct execsw_entry *)malloc(sizeof(struct execsw_entry), M_TEMP, M_WAITOK);
	et->next = NULL;
	et->ex = exp;
	if (*listp == NULL) {
		*listp = et;
		return;
	}
	switch(et->ex->ex_prio) {
	case EXECSW_PRIO_FIRST:
		/* put new entry as the first */
		et->next = *listp;
		*listp = et;
		break;
	case EXECSW_PRIO_ANY:
		/* put new entry after all *_FIRST and *_ANY entries */
		for (e1 = *listp; e1->next && e1->next->ex->ex_prio != EXECSW_PRIO_LAST;
				e1 = e1->next) {
			;
		}
		et->next = e1->next;
		e1->next = et;
		break;
	case EXECSW_PRIO_LAST:
		/* put new entry as the last one */
		for (e1 = *listp; e1->next; e1 = e1->next) {
			;
		}
		e1->next = et;
		break;
	default:
#ifdef DIAGNOSTIC
		panic("execw[] entry with unknown priority %d found",
			et->ex->ex_prio);
#else
		free(et, M_TEMP);
#endif
		break;
	}
}

void
exec_init(void)
{
	struct execsw_entry	*list;
	int	i;

	/*
	 * Build execsw[] array from builtin entries and entries added
	 * at runtime.
	 */
	list = NULL;
	for (i = 0; i < nexecs; i++) {
		link_exec(&list, &execsw[i]);
	}

	/*
	 * figure out the maximum size of an exec header.
	 */
	exec_maxhdrsz = 0;
	for (i = 0; i < nexecs; i++) {
		if ((execsw[i].ex_makecmds != NULL)
				&& (execsw[i].ex_hdrsz > exec_maxhdrsz)) {
			exec_maxhdrsz = execsw[i].ex_hdrsz;
		}
	}
}
