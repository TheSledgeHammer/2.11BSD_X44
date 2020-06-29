/*
 * initmain.c
 *
 *  Created on: 22 Jun 2020
 *      Author: marti
 */
#include <i386/include/bootinfo.h>
#include "sys/ksyms.h"

main(framep)
{
	/* add after startup */
#if ((NKSYMS > 0) || (NDDB > 0))
	ksyms_init();
#endif
}

/* part of start_init with kern_environment added */

/*
 * List of paths to try when searching for "init".
 */
static const  char * const initpaths[] = {
	"/sbin/init",
	"/sbin/oinit",
	"/sbin/init.bak",
	NULL,
};

static void
start_init(p, framep)
	struct proc *p;
	void *framep;
{
	char *var;
	char *free_initpaths, *tmp_initpaths;

	if ((var = kern_getenv("init_path")) != NULL) {
		strlcpy(initpaths, var, sizeof(initpaths));
		freeenv(var);
	}
	free_initpaths = tmp_initpaths = strdup(initpaths, M_TEMP);

	for (pathp = &initpaths[0]; (path = *pathp) != NULL; pathp++) {

		error = execve(p, &args, retval);
		if (error == 0 || error == EJUSTRETURN)
			free(free_initpaths, M_TEMP);
			return;
		if (error != ENOENT)
			printf("exec %s: error %d\n", path, error);
	}
	free(free_initpaths, M_TEMP);
	printf("init: not found\n");
	panic("no init");
}

/* belongs in machdep init386 */

int
init386()
{
	vm_offset_t addend;
	if (i386_ksyms_addsyms_elf(bootinfo)) {
		init386_ksyms(bootinfo);
	} else {
		if (bootinfo.bi_envp.bi_environment != 0) {
			addend = (caddr_t)bootinfo.bi_envp.bi_environment < KERNBASE ? PMAP_MAP_LOW : 0;
			init_static_kenv((char *)bootinfo.bi_envp.bi_environment + addend, 0);
		} else {
			init_static_kenv(NULL, 0);
		}
	}
}

static void
init386_ksyms(bootinfo)
	struct bootinfo *bootinfo;
{
	extern int 		end;
	vm_offset_t 	addend;

	if (bootinfo->bi_envp.bi_environment != 0) {
		ksyms_addsyms_elf(*(int*) &end, ((int*) &end) + 1, esym);
		addend = (caddr_t) bootinfo->bi_envp.bi_environment < KERNBASE ?	PMAP_MAP_LOW : 0;
		init_static_kenv((char*) bootinfo->bi_envp.bi_environment + addend, 0);
	} else {
		ksyms_addsyms_elf(*(int*) &end, ((int*) &end) + 1, esym);
		init_static_kenv(NULL, 0);
	}

	bootinfo->bi_envp->bi_symtab += KERNBASE;
	bootinfo->bi_envp->bi_esymtab += KERNBASE;
	ksyms_addsyms_elf(bootinfo->bi_envp->bi_nsymtab, (int*) bootinfo->bi_envp->bi_symtab, (int*) bootinfo->bi_envp->bi_esymtab);
}
