

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/resourcevar.h>
#include <sys/exec.h>
#include <sys/exec_linker.h>
#include <sys/exec_elf.h>
#include <sys/mman.h>
#include <sys/kernel.h>
#include <sys/sysent.h>

#include <vm/vm.h>

copyargs()
{

}

elf_check_header()
{

}

int
exec_elf_linker(elp)
	struct exec_linker *elp;
{
	return ENOEXEC;
}
