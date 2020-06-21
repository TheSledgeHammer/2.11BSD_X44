/*
 * ksyms.c
 *
 *  Created on: 22 Jun 2020
 *      Author: marti
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cdefs_elf.h>
#include <sys/boot.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>
#include <sys/user.h>
#include <sys/ksyms.h>

#include <i386/include/bootinfo.h>
#include <new_multiboot.h>

/*
 * Symbol and string table for the loaded kernel.
 */

struct multiboot_symbols {
	void *		s_symstart;
	size_t		s_symsize;
	void *		s_strstart;
	size_t		s_strsize;
};

extern int						biosbasemem;
extern int						biosextmem;
extern int						biosmem_implicit;
extern int						boothowto;
extern struct bootinfo			bootinfo;
extern int						end;
extern int *					esym;

static char						Multiboot_Cmdline[255];
static uint8_t					Multiboot_Drives[255];
static struct multiboot_info	Multiboot_Info;
static bool						Multiboot_Loader = false;
static char						Multiboot_Loader_Name[255];
static uint8_t					Multiboot_Mmap[1024];
static struct multiboot_symbols	Multiboot_Symbols;

/* --------------------------------------------------------------------- */

/*
 * Sets up the kernel if it was booted by a Multiboot-compliant boot
 * loader.  This is executed before the kernel has relocated itself.
 * The main purpose of this function is to copy all the information
 * passed in by the boot loader to a safe place, so that it is available
 * after it has been relocated.
 *
 * WARNING: Because the kernel has not yet relocated itself to KERNBASE,
 * special care has to be taken when accessing memory because absolute
 * addresses (referring to kernel symbols) do not work.  So:
 *
 *     1) Avoid jumps to absolute addresses (such as gotos and switches).
 *     2) To access global variables use their physical address, which
 *        can be obtained using the RELOC macro.
 */
#define RELOC(type, x) ((type)((caddr_t)(x) - KERNBASE))
void
multiboot1_pre_reloc(struct multiboot_info *mi)
{
	struct multiboot_info *midest = RELOC(struct multiboot_info*,
			&Multiboot_Info);

	*RELOC(bool*, &Multiboot_Loader) = true;
	memcpy(midest, mi, sizeof(Multiboot_Info));

	if (mi->mi_flags & MULTIBOOT_INFO_CMDLINE) {
		strncpy(RELOC(void*, Multiboot_Cmdline), mi->mi_cmdline,
				sizeof(Multiboot_Cmdline));
		midest->mi_cmdline = (char*) &Multiboot_Cmdline;
	}

	if (mi->mi_flags & MULTIBOOT_INFO_BOOT_LOADER_NAME) {
		strncpy(RELOC(void*, Multiboot_Loader_Name), mi->mi_loader_name,
				sizeof(Multiboot_Loader_Name));
		midest->mi_loader_name = (char*) &Multiboot_Loader_Name;
	}

	if (mi->mi_flags & MULTIBOOT_INFO_MEM_MAP) {
		memcpy(RELOC(void*, Multiboot_Mmap), (void*) mi->mi_mmap_addr,
				mi->mi_mmap_length);
		midest->mi_mmap_addr = (caddr_t) &Multiboot_Mmap;
	}

	if (mi->mi_flags & MULTIBOOT_INFO_DRIVE_INFO) {
		memcpy(RELOC(void*, Multiboot_Drives), (void*) mi->mi_drives_addr,
				mi->mi_drives_length);
		midest->mi_drives_addr = (caddr_t) &Multiboot_Drives;
	}

	copy_syms(mi);
#undef RELOC
}

/* --------------------------------------------------------------------- */

/*
 * Sets up the kernel if it was booted by a Multiboot-compliant boot
 * loader.  This is executed just after the kernel has relocated itself.
 * At this point, executing any kind of code is safe, keeping in mind
 * that no devices have been initialized yet (not even the console!).
 */
void
multiboot1_post_reloc(void)
{
	struct multiboot_info *mi;

	if (! Multiboot_Loader)
		return;

	mi = &Multiboot_Info;
	bootinfo.bi_nentries = 0;

	setup_memory(mi);
	setup_console(mi);
	setup_howto(mi);
	setup_bootpath(mi);
	setup_biosgeom(mi);
	setup_bootdisk(mi);
	setup_memmap(mi);
}

/*
 * Copies the symbol table and the strings table passed in by the boot
 * loader after the kernel's image, and sets up 'esym' accordingly so
 * that this data is properly copied into upper memory during relocation.
 *
 * WARNING: This code runs before the kernel has relocated itself.  See
 * the note in multiboot1_pre_reloc() for more information.
 */
#define RELOC(type, x) ((type)((caddr_t)(x) - KERNBASE))
static void
copy_syms(struct multiboot_info *mi)
{
	int i;
	struct multiboot_symbols *ms;
	Elf32_Shdr *symtabp, *strtabp;
	Elf32_Word symsize, strsize;
	Elf32_Addr symaddr, straddr;
	Elf32_Addr symstart, strstart;

	/*
	 * Check if the Multiboot information header has symbols or not.
	 */
	if (!(mi->mi_flags & MULTIBOOT_INFO_ELF_SYMS))
		return;

	ms = RELOC(struct multiboot_symbols*, &Multiboot_Symbols);

	/*
	 * Locate a symbol table and its matching string table in the
	 * section headers passed in by the boot loader.  Set 'symtabp'
	 * and 'strtabp' with pointers to the matching entries.
	 */
	symtabp = strtabp = NULL;
	for (i = 0; i < mi->mi_elfshdr_num && symtabp == NULL && strtabp == NULL;
			i++) {
		Elf32_Shdr *shdrp;

		shdrp = &((Elf32_Shdr*) mi->mi_elfshdr_addr)[i];

		if ((shdrp->sh_type == SHT_SYMTAB) && shdrp->sh_link != SHN_UNDEF) {
			Elf32_Shdr *shdrp2;

			shdrp2 = &((Elf32_Shdr*) mi->mi_elfshdr_addr)[shdrp->sh_link];

			if (shdrp2->sh_type == SHT_STRTAB) {
				symtabp = shdrp;
				strtabp = shdrp2;
			}
		}
	}
	if (symtabp == NULL || strtabp == NULL)
		return;

	symaddr = symtabp->sh_addr;
	straddr = strtabp->sh_addr;
	symsize = symtabp->sh_size;
	strsize = strtabp->sh_size;

	/*
	 * Copy the symbol and string tables just after the kernel's
	 * end address, in this order.  Only the contents of these ELF
	 * sections are copied; headers are discarded.  esym is later
	 * updated to point to the lowest "free" address after the tables
	 * so that they are mapped appropriately when enabling paging.
	 *
	 * We need to be careful to not overwrite valid data doing the
	 * copies, hence all the different cases below.  We can assume
	 * that if the tables start before the kernel's end address,
	 * they will not grow over this address.
	 */
	if ((void*) symtabp < RELOC(void*, &end)
			&& (void*) strtabp < RELOC(void*, &end)) {
		symstart = RELOC(Elf32_Addr, &end);
		strstart = symstart + symsize;
		memcpy((void*) symstart, (void*) symaddr, symsize);
		memcpy((void*) strstart, (void*) straddr, strsize);
	} else if ((void*) symtabp > RELOC(void*, &end)
			&& (void*) strtabp < RELOC(void*, &end)) {
		symstart = RELOC(Elf32_Addr, &end);
		strstart = symstart + symsize;
		memcpy((void*) symstart, (void*) symaddr, symsize);
		memcpy((void*) strstart, (void*) straddr, strsize);
	} else if ((void*) symtabp < RELOC(void*, &end)
			&& (void*) strtabp > RELOC(void*, &end)) {
		strstart = RELOC(Elf32_Addr, &end);
		symstart = strstart + strsize;
		memcpy((void*) strstart, (void*) straddr, strsize);
		memcpy((void*) symstart, (void*) symaddr, symsize);
	} else {
		/* symtabp and strtabp are both over end */
		if (symtabp < strtabp) {
			symstart = RELOC(Elf32_Addr, &end);
			strstart = symstart + symsize;
			memcpy((void*) symstart, (void*) symaddr, symsize);
			memcpy((void*) strstart, (void*) straddr, strsize);
		} else {
			strstart = RELOC(Elf32_Addr, &end);
			symstart = strstart + strsize;
			memcpy((void*) strstart, (void*) straddr, strsize);
			memcpy((void*) symstart, (void*) symaddr, symsize);
		}
	}

	*RELOC(int*, &esym) = (int) (symstart + symsize + strsize + KERNBASE);

	ms->s_symstart = (void*) (symstart + KERNBASE);
	ms->s_symsize = symsize;
	ms->s_strstart = (void*) (strstart + KERNBASE);
	ms->s_strsize = strsize;
#undef RELOC
}

/*
 * Sets up the initial kernel symbol table.  Returns true if this was
 * passed in by Multiboot; false otherwise.
 */
bool
multiboot1_ksyms_addsyms_elf(void)
{
	struct multiboot_info *mi = &Multiboot_Info;
	struct multiboot_symbols *ms = &Multiboot_Symbols;

	if (! Multiboot_Loader)
		return false;

	if (mi->mi_flags & MULTIBOOT_INFO_ELF_SYMS) {
		Elf32_Ehdr ehdr;

		KASSERT(esym != 0);

		memcpy(ehdr.e_ident, ELFMAG, SELFMAG);
		ehdr.e_ident[EI_CLASS] = ELFCLASS32;
		ehdr.e_ident[EI_DATA] = ELFDATA2LSB;
		ehdr.e_ident[EI_VERSION] = EV_CURRENT;
		ehdr.e_type = ET_EXEC;
		ehdr.e_machine = EM_386;
		ehdr.e_version = 1;
		ehdr.e_ehsize = sizeof(ehdr);

		ksyms_addsyms_explicit((void *)&ehdr,
		    ms->s_symstart, ms->s_symsize,
		    ms->s_strstart, ms->s_strsize);
	}

	return mi->mi_flags & MULTIBOOT_INFO_ELF_SYMS;
}
