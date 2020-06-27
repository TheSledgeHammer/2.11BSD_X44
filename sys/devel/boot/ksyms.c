
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
#include <new_multiboot.h>
#include "sys/ksyms.h"

static int 	boot_ksyms(struct preloaded_file *fp);
int 		boot_ksyms_addsyms_elf(void);
void 		boot_ksyms_addr_set(void *ehdr, void *shdr, void *symbase);
static void boot_copy_syms(struct multiboot_info *mi);

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

extern int			end;
extern int *		esym;

struct boot_symbols {
	void *		bs_symstart;
	size_t		bs_symsize;
	void *		bs_strstart;
	size_t		bs_strsize;
};

static int
boot_ksyms(struct preloaded_file *fp)
{
	struct multiboot_package 	*mbp;
	struct multiboot_info		*mbi;
	int				 			error;

	if (mbp->mbp_marks[MARK_SYM] != 0) {
		Elf32_Ehdr ehdr;
		void *shbuf;
		size_t shlen;
		u_long shaddr;

		bcopy((void*) mbp->mbp_marks[MARK_SYM], &ehdr, sizeof(ehdr));

		if (memcmp(&ehdr.e_ident, ELFMAG, SELFMAG) != 0)
			goto skip_ksyms;

		shaddr = mbp->mbp_marks[MARK_SYM] + ehdr.e_shoff;

		shlen = ehdr.e_shnum * ehdr.e_shentsize;
		shbuf = alloc(shlen);

		bcopy((void*) shaddr, shbuf, shlen);
		boot_ksyms_addr_set(&ehdr, shbuf, (void*) (KERNBASE + mbp->mbp_marks[MARK_SYM]));
		bcopy(shbuf, (void*) shaddr, shlen);

		dealloc(shbuf, shlen);

		mbi->mi_elfshdr_num = ehdr.e_shnum;
		mbi->mi_elfshdr_size = ehdr.e_shentsize;
		mbi->mi_elfshdr_addr = shaddr;
		mbi->mi_elfshdr_shndx = ehdr.e_shstrndx;

		mbi->mi_flags |= MULTIBOOT_INFO_ELF_SYMS;
	}

skip_ksyms:
#ifdef DEBUG
	printf("Start @ 0x%lx [%ld=0x%lx-0x%lx]...\n",
	mbp->mbp_marks[MARK_ENTRY],
	mbp->mbp_marks[MARK_NSYM],
	mbp->mbp_marks[MARK_SYM],
	mbp->mbp_marks[MARK_END]);
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

	class = ((Elf_Ehdr *)ehdr)->e_ident[EI_CLASS];

        switch (class) {
        case ELFCLASS32:
                ehdr32 = (Elf32_Ehdr *)ehdr;
                shnum = ehdr32->e_shnum;
                break;
        case ELFCLASS64:
                ehdr64 = (Elf64_Ehdr *)ehdr;
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

		switch(class) {
		case ELFCLASS64:
			shdrp64 = &((Elf64_Shdr *)shdr)[i];
			shtype = shdrp64->sh_type;
			shaddr = shdrp64->sh_addr;
			shsize = shdrp64->sh_size;
			shoffset = shdrp64->sh_offset;
			break;
		case ELFCLASS32:
			shdrp32 = &((Elf32_Shdr *)shdr)[i];
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

		shaddr = (uint64_t)(uintptr_t)(symbase + shoffset);

		switch(class) {
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
boot_ksyms_addsyms_elf(void)
{
	struct boot_symbols *bs;
	caddr_t symstart = (caddr_t) bs->bs_symstart;
	caddr_t strstart = (caddr_t) bs->bs_strstart;
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

	ksyms_addsyms_explicit((void*) &ehdr, (void*) symstart, bs->bs_symsize, (void*) strstart, bs->bs_strsize);

	return (0);
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
boot_copy_syms(struct multiboot_info *mi)
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
