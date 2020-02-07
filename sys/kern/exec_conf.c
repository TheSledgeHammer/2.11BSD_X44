/*
 * exec_conf.c
 *
 *  Created on: 7 Feb 2020
 *      Author: marti
 */
#include <sys/param.h>
#include <sys/exec.h>
#include <sys/exec_linker.h>
#include <sys/exec_script.h>
#include <sys/exec_aout.h>
#include <sys/exec_coff.h>
#include <sys/exec_ecoff.h>
#include <sys/exec_elf.h>

extern struct emul emul_211bsd;

struct execsw execsw[] = {
		{ sizeof(struct exec), exec_aout_linker, },		/* aout binaries */
		//{ COFF_HDR_SIZE, exec_coff_linker, },			/* coff binaries */
		{ ECOFF_HDR_SIZE, exec_ecoff_linker, },			/* ecoff binaries */
		{ sizeof(Elf32_Ehdr), exec_elf_linker },		/* 32-Bit ELF binaries */
};

int nexecs = (sizeof execsw / sizeof(*execsw));
int exec_maxhdrsz;

void	init_exec(void);

void
init_exec(void)
{
	int i;

	/*
	 * figure out the maximum size of an exec header.
	 */
	for (i = 0; i < nexecs; i++)
		if (execsw[i].ex_makecmds != NULL && execsw[i].ex_hdrsz > exec_maxhdrsz)
			exec_maxhdrsz = execsw[i].ex_hdrsz;
}
