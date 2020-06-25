/*
 * ksyms_multiboot.c
 *
 *  Created on: 25 Jun 2020
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
#include <i386/include/bootinfo.h>
#include <new_multiboot.h>
#include "../sys/ksyms.h"


#ifdef _LOCORE64
typedef uint64_t   locore_caddr_t;
typedef Elf64_Shdr locore_Elf_Shdr;
typedef Elf64_Word locore_Elf_Word;
typedef Elf64_Addr locore_Elf_Addr;
#else
typedef caddr_t   locore_caddr_t;
typedef Elf_Shdr 	locore_Elf_Shdr;
typedef Elf_Word 	locore_Elf_Word;
typedef Elf_Addr 	locore_Elf_Addr;
#endif

extern int              boothowto;
extern struct bootinfo  bootinfo;
extern int              end;
extern int *            esym;
extern char             start;

bool has_syms = FALSE;

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

#define RELOC(type, x) ((type)((caddr_t)(x) - KERNBASE))

/*
 * Symbol and string table for the loaded kernel.
 */
struct multiboot_symbols {
	void *		s_symstart;
	size_t		s_symsize;
	void *		s_strstart;
	size_t		s_strsize;
};

void
multiboot2_copy_syms(struct multiboot_elf_symbol_table *mbt_elf, struct multiboot_symbols *ms, bool *has_symsp, int **esymp, void *endp, caddr_t kernbase)
{
	int i;
	locore_Elf_Shdr *symtabp, *strtabp;
	locore_Elf_Word symsize, strsize;
	locore_Elf_Addr symaddr, straddr;
	locore_Elf_Addr symstart, strstart;
	locore_Elf_Addr cp1src, cp1dst;
	locore_Elf_Word cp1size;
	locore_Elf_Addr cp2src, cp2dst;
	locore_Elf_Word cp2size;

	/*
	 * Locate a symbol table and its matching string table in the
	 * section headers passed in by the boot loader.  Set 'symtabp'
	 * and 'strtabp' with pointers to the matching entries.
	 */
	symtabp = strtabp = NULL;
	for (i = 0; i < mbt_elf->num && symtabp == NULL && strtabp == NULL; i++) {
		locore_Elf_Shdr *shdrp;

		shdrp = &((locore_Elf_Shdr*) mbt_elf->sections)[i];

		if ((shdrp->sh_type == SHT_SYMTAB) && shdrp->sh_link != SHN_UNDEF) {
			locore_Elf_Shdr *shdrp2;

			shdrp2 = &((locore_Elf_Shdr*) mbt_elf->sections)[shdrp->sh_link];

			if (shdrp2->sh_type == SHT_STRTAB) {
				symtabp = (locore_Elf_Shdr*) shdrp;
				strtabp = (locore_Elf_Shdr*) shdrp2;
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
	if ((void*) (uintptr_t) symaddr < endp
			&& (void*) (uintptr_t) straddr < endp) {
		cp1src = symaddr;
		cp1size = symsize;
		cp2src = straddr;
		cp2size = strsize;
	} else if ((void*) (uintptr_t) symaddr > endp
			&& (void*) (uintptr_t) straddr < endp) {
		cp1src = symaddr;
		cp1size = symsize;
		cp2src = straddr;
		cp2size = strsize;
	} else if ((void*) (uintptr_t) symaddr < endp
			&& (void*) (uintptr_t) straddr > endp) {
		cp1src = straddr;
		cp1size = strsize;
		cp2src = symaddr;
		cp2size = symsize;
	} else {
		/* symaddr and straddr are both over end */
		if (symaddr < straddr) {
			cp1src = symaddr;
			cp1size = symsize;
			cp2src = straddr;
			cp2size = strsize;
		} else {
			cp1src = straddr;
			cp1size = strsize;
			cp2src = symaddr;
			cp2size = symsize;
		}
	}

	cp1dst = (locore_Elf_Addr) (uintptr_t) endp;
	cp2dst = (locore_Elf_Addr) (uintptr_t) endp + cp1size;

	(void) memcpy((void*) (uintptr_t) cp1dst, (void*) (uintptr_t) cp1src,
			cp1size);
	(void) memcpy((void*) (uintptr_t) cp2dst, (void*) (uintptr_t) cp2src,
			cp2size);

	symstart = (cp1src == symaddr) ? cp1dst : cp2dst;
	strstart = (cp1src == straddr) ? cp1dst : cp2dst;

	ms->s_symstart = symstart + kernbase;
	ms->s_symsize = symsize;
	ms->s_strstart = strstart + kernbase;
	ms->s_strsize = strsize;

	*has_symsp = TRUE;
	*esymp = (int*) ((uintptr_t) endp + symsize + strsize + kernbase);
}


void
multiboot2_pre_reloc(char *mbi)
{
	uint32_t mbi_size;
	char *cp;
	struct multiboot_elf_symbol_table *mbt_elf = NULL;
	struct efi_systbl *efi_systbl = NULL;


	if (mbt_elf) {
		multiboot2_copy_syms(mbt_elf, RELOC(struct multiboot_symbols *, &Multiboot_Symbols), RELOC(bool *, &has_syms), RELOC(int **, &esym), RELOC(void *, &end), KERNBASE);
	}
	return;
}

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
	struct multiboot_info *midest = RELOC(struct multiboot_info*, &Multiboot_Info);

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
		return TRUE;

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

		ksyms_addsyms_explicit((void *)&ehdr, ms->s_symstart, ms->s_symsize, ms->s_strstart, ms->s_strsize);
	}

	return mi->mi_flags & MULTIBOOT_INFO_ELF_SYMS;
}

/*
 * Sets up the initial kernel symbol table.  Returns true if this was
 * passed in by Multiboot; false otherwise.
 */
bool
multiboot2_ksyms_addsyms_elf(void)
{
	struct multiboot_symbols *ms = &Multiboot_Symbols;
	caddr_t symstart = (caddr_t) ms->s_symstart;
	caddr_t strstart = (caddr_t) ms->s_strstart;
	Elf_Ehdr ehdr;

	if (!multiboot2_enabled || !has_syms)
		return false;

	KASSERT(esym != 0);

#ifdef __LP64__
	/* Adjust pointer as 64 bits */
	symstart &= 0xffffffff;
	symstart |= ((caddr_t)KERNBASE_HI << 32);
	strstart &= 0xffffffff;
	strstart |= ((caddr_t)KERNBASE_HI << 32);
#endif

	memset(&ehdr, 0, sizeof(ehdr));
	memcpy(ehdr.e_ident, ELFMAG, SELFMAG);
	ehdr.e_ident[EI_CLASS] = ELFCLASS;
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
	ehdr.e_entry = (Elf_Addr) & start;
	ehdr.e_ehsize = sizeof(ehdr);

	ksyms_addsyms_explicit((void*) &ehdr, (void*) symstart, ms->s_symsize, (void*) strstart, ms->s_strsize);

	return TRUE;
}

static int
multiboot_ksyms(struct preloaded_file *fp)
{
	struct multiboot_package 	*mbp;
	struct multiboot_info		*mbi;
	int				 			error;

	if (mbp->mbp_marks[MARK_SYM] != 0) {
		Elf32_Ehdr ehdr;
		void *shbuf;
		size_t shlen;
		u_long shaddr;

		bcopy((void *)mbp->mbp_marks[MARK_SYM], &ehdr, sizeof(ehdr));

		if (memcmp(&ehdr.e_ident, ELFMAG, SELFMAG) != 0)
			goto skip_ksyms;

		shaddr = mbp->mbp_marks[MARK_SYM] + ehdr.e_shoff;

		shlen = ehdr.e_shnum * ehdr.e_shentsize;
		shbuf = alloc(shlen);

		bcopy((void*) shaddr, shbuf, shlen);
		ksyms_addr_set(&ehdr, shbuf, (void*) (KERNBASE + mbp->mbp_marks[MARK_SYM]));
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
ksyms_addr_set(void *ehdr, void *shdr, void *symbase)
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

/* sections */
static size_t
mbi_elf_sections(struct multiboot_package *mbp, void *buf)
{
	size_t len = 0;
	struct multiboot_tag_elf_sections *mbt = buf;
	Elf_Ehdr ehdr;
	int class;
	Elf32_Ehdr *ehdr32 = NULL;
	Elf64_Ehdr *ehdr64 = NULL;
	uint64_t shnum, shentsize, shstrndx, shoff;
	size_t shdr_len;

	if (mbp->mbp_marks[MARK_SYM] == 0)
		goto out;

	pvbcopy((void *)mbp->mbp_marks[MARK_SYM], &ehdr, sizeof(ehdr));

	/*
	 * Check this is a ELF header
	 */
	if (memcmp(&ehdr.e_ident, ELFMAG, SELFMAG) != 0)
		goto out;

	class = ehdr.e_ident[EI_CLASS];

	switch (class) {
	case ELFCLASS32:
		ehdr32 = (Elf32_Ehdr *)&ehdr;
		shnum = ehdr32->e_shnum;
		shentsize = ehdr32->e_shentsize;
		shstrndx = ehdr32->e_shstrndx;
		shoff = ehdr32->e_shoff;
		break;
	case ELFCLASS64:
		ehdr64 = (Elf64_Ehdr *)&ehdr;
		shnum = ehdr64->e_shnum;
		shentsize = ehdr64->e_shentsize;
		shstrndx = ehdr64->e_shstrndx;
		shoff = ehdr64->e_shoff;
		break;
	default:
		goto out;
	}

	shdr_len = shnum * shentsize;
	if (shdr_len == 0)
		goto out;

	len = sizeof(*mbt) + shdr_len;
	if (mbt) {
		char *shdr = (char *)mbp->mbp_marks[MARK_SYM] + shoff;

		mbt->type = MULTIBOOT_TAG_TYPE_ELF_SECTIONS;
		mbt->size = len;
		mbt->num = shnum;
		mbt->entsize = shentsize;
		mbt->shndx = shstrndx;

		pvbcopy((void *)shdr, mbt + 1, shdr_len);

		/*
		 * Adjust sh_addr for symtab and strtab
		 * section that have been loaded.
		 */
		ksyms_addr_set(&ehdr, mbt + 1, (void *)mbp->mbp_marks[MARK_SYM]);
	}

out:
	return roundup(len, MULTIBOOT_TAG_ALIGN);
}
