/*
 * multiboot.c
 *
 *  Created on: 19 Jun 2020
 *      Author: marti
 */


#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/exec.h>
#include <sys/exec_linker.h>
#include <sys/exec_elf.h>
#include <sys/user.h>
#include <sys/stdint.h>

#include <string.h>
#include <bootstand.h>
#include <libsa/loadfile.h>
#include "bootstrap.h"
#include "multiboot.h"
#include "libi386.h"
#include <btxv86.h>

static const char mbl_name[] = "FreeBSD Loader";

static int
multiboot_loadfile(char *filename, uint64_t dest, struct preloaded_file **result)
{
	uint32_t				*magic;
	int			 			i, error;
	caddr_t			 		header_search;
	ssize_t			 		search_size;
	int			 			fd;
	struct multiboot_header	*header;
	char					*cmdline;

	/*
	 * Read MULTIBOOT_SEARCH size in order to search for the
	 * multiboot magic header.
	 */
	if (filename == NULL)
		return (EFTYPE);
	if ((fd = open(filename, O_RDONLY)) == -1)
		return (errno);
	header_search = malloc(MULTIBOOT_SEARCH);
	if (header_search == NULL) {
		close(fd);
		return (ENOMEM);
	}
	search_size = read(fd, header_search, MULTIBOOT_SEARCH);
	magic = (uint32_t*) header_search;

	header = NULL;
	for (i = 0; i < (search_size / sizeof(uint32_t)); i++) {
		if (magic[i] == MULTIBOOT_HEADER_MAGIC) {
			header = (struct multiboot_header*) &magic[i];
			break;
		}
	}

	if (header == NULL) {
		error = EFTYPE;
		goto out;
	}

	/* Valid multiboot header has been found, validate checksum */
	if (header->magic + header->flags + header->checksum != 0) {
		printf(
				"Multiboot checksum failed, magic: 0x%x flags: 0x%x checksum: 0x%x\n",
				header->magic, header->flags, header->checksum);
		error = EFTYPE;
		goto out;
	}

	if ((header->flags & ~MULTIBOOT_SUPPORTED_FLAGS) != 0) {
		printf("Unsupported multiboot flags found: 0x%x\n", header->flags);
		error = EFTYPE;
		goto out;
	}

#ifdef BOOT_AOUT
	error =	aout_loadfile(filename, dest, result);
#endif
#ifdef BOOT_ECOFF
	error =	ecoff_loadfile(filename, dest, result);
#endif
#ifdef BOOT_ELF32
	error =	elf32_loadfile(filename, dest, result);
#endif
#ifdef BOOT_ELF64
	error =	elf64_loadfile(filename, dest, result);
#endif

	if (error != 0) {
		printf("loadfile failed: %d unable to load multiboot kernel\n", error);
		goto out;
	}

	/*
	 * f_addr is already aligned to PAGE_SIZE, make sure
	 * f_size it's also aligned so when the modules are loaded
	 * they are aligned to PAGE_SIZE.
	 */
	(*result)->f_size = roundup((*result)->f_size, PAGE_SIZE);

out:
	free(header_search);
	close(fd);
	return (error);
}

static int
multiboot_exec(struct preloaded_file *fp)
{
	vm_offset_t					 module_start, last_addr, metadata_size;
	vm_offset_t					 modulep, kernend, entry;
	struct file_metadata		*md;
	Elf_Ehdr					*ehdr;
	struct multiboot_info		*mb_info = NULL;
	struct multiboot_mod_list	*mb_mod = NULL;
	char						*cmdline = NULL;
	size_t				 		len;
	int							error, mod_num;

	/*
	 * Don't pass the memory size found by the bootloader, the memory
	 * available to Dom0 will be lower than that.
	 */
	unsetenv("smbios.memory.enabled");

	/* Allocate the multiboot struct and fill the basic details. */
	mb_info = malloc(sizeof(struct multiboot_info));
	if (mb_info == NULL) {
		error = ENOMEM;
		goto error;
	}
	bzero(mb_info, sizeof(struct multiboot_info));
	mb_info->flags = MULTIBOOT_INFO_MEMORY | MULTIBOOT_INFO_BOOT_LOADER_NAME;
	mb_info->mem_lower = bios_basemem / 1024;
	mb_info->mem_upper = bios_extmem / 1024;
	mb_info->boot_loader_name = VTOP(mbl_name);

	/* Set the Xen command line. */
	if (fp->f_args == NULL) {
		/* Add the Xen command line if it is set. */
		cmdline = getenv("xen_cmdline");
		if (cmdline != NULL) {
			fp->f_args = strdup(cmdline);
			if (fp->f_args == NULL) {
				error = ENOMEM;
				goto error;
			}
		}
	}
	if (fp->f_args != NULL) {
		len = strlen(fp->f_name) + 1 + strlen(fp->f_args) + 1;
		cmdline = malloc(len);
		if (cmdline == NULL) {
			error = ENOMEM;
			goto error;
		}
		snprintf(cmdline, len, "%s %s", fp->f_name, fp->f_args);
		mb_info->cmdline = VTOP(cmdline);
		mb_info->flags |= MULTIBOOT_INFO_CMDLINE;
	}

	/* Not needed unless using meta */
	/* Find the entry point of the Xen kernel and save it for later */
	if ((md = file_findmetadata(fp, MODINFOMD_ELFHDR)) == NULL) {
		printf("Unable to find %s entry point\n", fp->f_name);
		error = EINVAL;
		goto error;
	}
	ehdr = (Elf_Ehdr*) &(md->md_data);
	entry = ehdr->e_entry & 0xffffff;

	/* Has more to be filled in here */
error:
	if (mb_mod)
		free(mb_mod);
	if (mb_info)
		free(mb_info);
	if (cmdline)
		free(cmdline);
	return (error);
}

void
ksyms_addr_set(void *ehdr, void *shdr, void *symbase)
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

static int
exec_multiboot1(struct multiboot_package *mbp)
{
	struct multiboot_info *mbi;
	int i, len;
	char *cmdline;

	mbi = alloc(sizeof(struct multiboot_info));
	mbi->mi_flags = MULTIBOOT_INFO_MEMORY | MULTIBOOT_INFO_BOOT_LOADER_NAME;

	mbi->mi_mem_upper = mbp->mbp_extmem;
	mbi->mi_mem_lower = mbp->mbp_basemem;

	if (mbp->mbp_args) {
		mbi->mi_flags |= MULTIBOOT_INFO_HAS_CMDLINE;
		len = strlen(mbp->mbp_file) + 1 + strlen(mbp->mbp_args) + 1;
		cmdline = alloc(len);
		snprintf(cmdline, len, "%s %s", mbp->mbp_file, mbp->mbp_args);
		mbi->mi_cmdline = (char *) vtophys(cmdline);
	}

	/* Loads Modules code can be added Here */

	if (mbp->mbp_marks[MARK_SYM] != 0) {
		Elf32_Ehdr ehdr;
		void *shbuf;
		size_t shlen;
		u_long shaddr;

		/* copies from absolute virtual address */
		pvbcopy((void *)mbp->mbp_marks[MARK_SYM], &ehdr, sizeof(ehdr));

		if (memcmp(&ehdr.e_ident, ELFMAG, SELFMAG) != 0)
			goto skip_ksyms;

		shaddr = mbp->mbp_marks[MARK_SYM] + ehdr.e_shoff;

		shlen = ehdr.e_shnum * ehdr.e_shentsize;
		shbuf = alloc(shlen);

		pvbcopy((void*) shaddr, shbuf, shlen);
		ksyms_addr_set(&ehdr, shbuf, (void*) (KERNBASE + mbp->mbp_marks[MARK_SYM]));
		vpbcopy(shbuf, (void*) shaddr, shlen);

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

	/* Does not return */
	multiboot(mbp->mbp_marks[MARK_ENTRY], vtophys(mbi), x86_trunc_page(mbi->mi_mem_lower * 1024), MULTIBOOT_INFO_MAGIC);
	return 0;
}


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
