
/* Belongs in /stand/boot/common
 * TODO:
 * - Change Multiboot, not relavent
 * - Copy symbols from boot_symbols (before kern reloc) to new address (after kern reloc)
 * - Check symbols table
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/boot.h>
#include <sys/user.h>
#include <sys/cdefs_elf.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>
#include <lib/libsa/loadfile.h>
#include <i386/include/bootinfo.h>
#include "sys/ksyms.h"

#define BOOTINFO_MEMORY        	0x00000001
#define BOOTINFO_AOUT_SYMS      0x00000010
#define BOOTINFO_ELF_SYMS		0x00000020

static int 	boot_ksyms(struct preloaded_file *fp);
int 		i386_ksyms_addsyms_elf(struct bootinfo *bi);
void 		boot_ksyms_addr_set(void *ehdr, void *shdr, void *symbase);

#ifdef _LOCORE64
typedef uint64_t   locore_caddr_t;
typedef Elf64_Shdr locore_Elf_Shdr;
typedef Elf64_Word locore_Elf_Word;
typedef Elf64_Addr locore_Elf_Addr;
#else
typedef caddr_t   	locore_caddr_t;
typedef Elf_Shdr 	locore_Elf_Shdr;
typedef Elf_Word 	locore_Elf_Word;
typedef Elf_Addr 	locore_Elf_Addr;
#endif

struct file_format bootops = { boot_loadfile, boot_exec };

static int
boot_ksyms(struct preloaded_file *fp)
{
	struct bootinfo *bi;

	fp->f_flags = BOOTINFO_MEMORY;

	fp->f_mem_upper = bi->bi_bios->bi_extmem;
	fp->f_mem_lower = bi->bi_bios->bi_basemem;

	if (fp->f_marks[MARK_SYM] != 0) {
		Elf32_Ehdr ehdr;
		void *shbuf;
		size_t shlen;
		u_long shaddr;

		bcopy((void*) fp->f_marks[MARK_SYM], &ehdr, sizeof(ehdr));

		if (memcmp(&ehdr.e_ident, ELFMAG, SELFMAG) != 0)
			goto skip_ksyms;

		shaddr = fp->f_marks[MARK_SYM] + ehdr.e_shoff;

		shlen = ehdr.e_shnum * ehdr.e_shentsize;
		shbuf = alloc(shlen);

		bcopy((void*) shaddr, shbuf, shlen);
		boot_ksyms_addr_set(&ehdr, shbuf, (void*) (KERNBASE + fp->f_marks[MARK_SYM]));
		bcopy(shbuf, (void*) shaddr, shlen);

		dealloc(shbuf, shlen);

		fp->f_elfshdr_num = ehdr.e_shnum;
		fp->f_elfshdr_size = ehdr.e_shentsize;
		fp->f_elfshdr_addr = shaddr;
		fp->f_elfshdr_shndx = ehdr.e_shstrndx;

		fp->f_flags |= BOOTINFO_ELF_SYMS;
	}

skip_ksyms:
#ifdef DEBUG
	printf("Start @ 0x%lx [%ld=0x%lx-0x%lx]...\n",
	fp->f_marks[MARK_ENTRY],
	fp->f_marks[MARK_NSYM],
	fp->f_marks[MARK_SYM],
	fp->f_marks[MARK_END]);
#endif

	return (0);
}

void
boot_ksyms_addr_set(void *ehdr, void *shdr, void *symbase)
{
	int class;
	Elf32_Ehdr *ehdr32 = NULL;
	Elf64_Ehdr *ehdr64 = NULL;
	uint64_t shnum;
	int i;

	class = ((Elf_Ehdr*) ehdr)->e_ident[EI_CLASS];

	switch (class) {
	case ELFCLASS32:
		ehdr32 = (Elf32_Ehdr*) ehdr;
		shnum = ehdr32->e_shnum;
		break;
	case ELFCLASS64:
		ehdr64 = (Elf64_Ehdr*) ehdr;
		shnum = ehdr64->e_shnum;
		break;
	default:
		panic("Unexpected ELF class");
		break;
	}

	for (i = 0; i < shnum; i++) {
		Elf64_Shdr *shdrp64 = NULL;
		Elf32_Shdr *shdrp32 = NULL;
		uint64_t shtype, shaddr, shsize, shoffset;

		switch (class) {
		case ELFCLASS64:
			shdrp64 = &((Elf64_Shdr*) shdr)[i];
			shtype = shdrp64->sh_type;
			shaddr = shdrp64->sh_addr;
			shsize = shdrp64->sh_size;
			shoffset = shdrp64->sh_offset;
			break;
		case ELFCLASS32:
			shdrp32 = &((Elf32_Shdr*) shdr)[i];
			shtype = shdrp32->sh_type;
			shaddr = shdrp32->sh_addr;
			shsize = shdrp32->sh_size;
			shoffset = shdrp32->sh_offset;
			break;
		default:
			panic("Unexpected ELF class");
			break;
		}

		if (shtype != SHT_SYMTAB && shtype != SHT_STRTAB)
			continue;

		if (shaddr != 0 || shsize == 0)
			continue;

		shaddr = (uint64_t) (uintptr_t) (symbase + shoffset);

		switch (class) {
		case ELFCLASS64:
			shdrp64->sh_addr = shaddr;
			break;
		case ELFCLASS32:
			shdrp32->sh_addr = shaddr;
			break;
		default:
			panic("Unexpected ELF class");
			break;
		}
	}
	return;
}


int
i386_ksyms_addsyms_elf(struct bootinfo *bi)
{
	//struct bootinfo *bi;
	caddr_t symstart = (caddr_t) bi->bi_sym->bi_symstart;
	caddr_t strstart = (caddr_t) bi->bi_sym->bi_strstart;
	Elf_Ehdr ehdr;

	KASSERT(esym != 0);

	memset(&ehdr, 0, sizeof(ehdr));
	memcpy(ehdr.e_ident, ELFMAG, SELFMAG);
#ifdef ELFSIZE32
	ehdr.e_ident[EI_CLASS] = ELFCLASS32;
#endif
#ifdef ELFSIZE64
	ehdr.e_ident[EI_CLASS] = ELFCLASS64;
#endif
	ehdr.e_ident[EI_DATA] = ELFDATA2LSB;
	ehdr.e_ident[EI_VERSION] = EV_CURRENT;
	ehdr.e_ident[EI_OSABI] = ELFOSABI_SYSV;
	ehdr.e_ident[EI_ABIVERSION] = 0;
	ehdr.e_type = ET_EXEC;
#ifdef __amd64__
	ehdr.e_machine = EM_X86_64;
#elif __i386__
	ehdr.e_machine = EM_386;
#else
	#error "Unknwo ELF machine type"
#endif
	ehdr.e_version = 1;
	ehdr.e_ehsize = sizeof(ehdr);
	ehdr.e_entry = (Elf_Addr) & start;

	ksyms_addsyms_explicit((void*) &ehdr, (void*) symstart, bi->bi_sym->bi_symsize, (void*) strstart, bi->bi_sym->bi_strsize);

	return (0);
}


static int
boot_loadfile(char *args, uint64_t dest, struct preloaded_file *fp)
{
  #ifdef BOOT_AOUT
	error = aout_loadfile(args, dest, fp);
	if (error != 0) {
		printf("aout_loadfile failed: %d unable to load multiboot kernel\n", error);
		goto out;
	}
#endif
#ifdef BOOT_ECOFF
	error = ecoff_loadfile(args, dest, fp);
	if (error != 0) {
		printf("ecoff_loadfile failed: %d unable to load multiboot kernel\n", error);
		goto out;
	}
#endif
#ifdef BOOT_ELF32
	error = elf32_loadfile(args, dest, fp);
	if (error != 0) {
		printf("elf32_loadfile failed: %d unable to load multiboot kernel\n", error);
		goto out;
	}
#endif
#ifdef BOOT_ELF64
	error = elf64_loadfile(args, dest, fp);
	if (error != 0) {
		printf("elf64_loadfile failed: %d unable to load multiboot kernel\n", error);
		goto out;
	}
#endif

	/*
	 * f_addr is already aligned to PAGE_SIZE, make sure
	 * f_size it's also aligned so when the modules are loaded
	 * they are aligned to PAGE_SIZE.
	 */
	(*fp)->f_size = roundup((*fp)->f_size, PAGE_SIZE);

out:
  return (error);
}

static int
boot_exec(struct preloaded_file *fp)
{
  struct bootinfo 		*bi;
  int 					i, len;
  char 					*cmdline;
  int				 	error;
  vm_offset_t			entry, bootinfop, kernend, last_addr;

  fp->f_flags = BOOTINFO_MEMORY;

  fp->f_mem_upper = bi->bi_bios->bi_extmem;
  fp->f_mem_lower = bi->bi_bios->bi_basemem;

  #ifdef BOOT_AOUT
	fp = file_findfile(NULL, AOUT_KERNELTYPE);
#endif
#ifdef BOOT_ECOFF
	fp = file_findfile(NULL, ECOFF_KERNELTYPE);
#endif
#ifdef BOOT_ELF32 && ELFSIZE == 32
	fp = file_findfile(NULL, ELF32_KERNELTYPE);
#endif
#ifdef BOOT_ELF64 && ELFSIZE == 64
	fp = file_findfile(NULL, ELF64_KERNELTYPE);
#endif

	if (fp == NULL) {
		error = EINVAL;
		goto error;
	}

	last_addr = roundup(max_addr(), PAGE_SIZE);

	error = bi_load_stage1(fp, fp->f_args, last_addr, 0);
	if (error != 0) {
		printf("bi_load_stage1 failed: %d\n", error);
		goto error;
	}
	dev_cleanup();
	__exec((void *)entry, boothowto, bootdev, 0, 0, 0, bootinfop, kernend, (void *)VTOP(bi));

    panic("exec returned");

error:
  return (error);
}

static vm_offset_t
max_addr(void)
{
	struct preloaded_file	*fp;
	vm_offset_t		 addr = 0;

	for (fp = file_findfile(NULL, NULL); fp != NULL; fp = fp->f_next) {
		if (addr < (fp->f_addr + fp->f_size))
			addr = fp->f_addr + fp->f_size;
	}

	return (addr);
}
