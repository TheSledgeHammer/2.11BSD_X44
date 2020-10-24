/*
 * pmap3.h
 *
 *  Created on: 23 Oct 2020
 *      Author: marti
 */

#ifndef SYS_DEVEL_ARCH_I386_PMAP3_H_
#define SYS_DEVEL_ARCH_I386_PMAP3_H_

typedef uint32_t pd_entry_t;	/* page directory entry */
typedef uint32_t pt_entry_t;	/* Mach page table entry */

#define	PD_SHIFT_PAE		21
#define	PG_FRAME_PAE		(0x000ffffffffff000ull)
#define	PG_PS_FRAME_PAE		(0x000fffffffe00000ull)

#define	PD_SHIFT_NOPAE		22
#define	PG_FRAME_NOPAE		(~PGMASK)
#define	PG_PS_FRAME_NOPAE	(0xffc00000)

#ifndef NKPDE
#define NKPDE				(KVA_PAGES)	/* number of page tables/pde's */
#endif

/*
 * One page directory, shared between
 * kernel and user modes.
 */
#define I386_PAGE_SIZE		NBPG
#define I386_PDR_SIZE		NBPDR

#define I386_KPDES			8 										/* KPT page directory size */
#define I386_UPDES			(NBPDR/sizeof(struct pde) - I386_KPDES) /* UPT page directory size */

#define	UPTDI				0x3f6									/* ptd entry for u./kernel&user stack */
#define	PTDPTDI				0x3f7									/* ptd entry that points to ptd! */
#define	KPTDI_FIRST			0x3f8									/* start of kernel virtual pde's */
#define	KPTDI_LAST			0x3fA									/* last of kernel virtual pde's */

/*
 * XXX doesn't really belong here I guess...
 */
#define ISA_HOLE_START    	0xa0000
#define ISA_HOLE_LENGTH 	(0x100000-ISA_HOLE_START)

/*
 * Address of current and alternate address space page table maps
 * and directories.
 */
//#ifdef KERNEL
extern pt_entry_t	PTmap[], APTmap[], Upte;
extern pd_entry_t	PTD[], APTD[], PTDpde, APTDpde, Upde;
extern pt_entry_t	*Sysmap;

extern int			IdlePTD;	/* physical address of "Idle" state directory */
//#endif

/*
 * virtual address to page table entry and
 * to physical address. Likewise for alternate address space.
 * Note: these work recursively, thus vtopte of a pte will give
 * the corresponding pde that in turn maps it.
 */
#define	vtopte(va)			(PTmap + i386_btop(va))
#define	kvtopte(va)			vtopte(va)
#define	ptetov(pt)			(i386_ptob(pt - PTmap))
#define	vtophys(va)  		(i386_ptob(vtopte(va)->pg_pfnum) | ((int)(va) & PGOFSET))
#define ispt(va)			((va) >= UPT_MIN_ADDRESS && (va) <= KPT_MAX_ADDRESS)

#define	avtopte(va)			(APTmap + i386_btop(va))
#define	ptetoav(pt)	 		(i386_ptob(pt - APTmap))
#define	avtophys(va)  		(i386_ptob(avtopte(va)->pg_pfnum) | ((int)(va) & PGOFSET))

/*
 * Pmap stuff
 */
struct pmap {
	pd_entry_t				*pm_pdir;		/* KVA of page directory */
	pt_entry_t				*pm_ptab;		/* KVA of page table */
	boolean_t				pm_pdchanged;	/* pdir changed */
	short					pm_dref;		/* page directory ref count */
	short					pm_count;		/* pmap reference count */
	simple_lock_data_t		pm_lock;		/* lock on pmap */
	struct pmap_statistics	pm_stats;		/* pmap statistics */
	long					pm_ptpages;		/* more stats: PT pages */

	int 					pm_flags;		/* see below */
	union descriptor 		*pm_ldt;		/* user-set LDT */
	int 					pm_ldt_len;		/* number of LDT entries */
	int 					pm_ldt_sel;		/* LDT selector */
};

typedef struct pmap			*pmap_t;

//#ifdef KERNEL
extern struct pmap	kernel_pmap_store;
#define kernel_pmap (&kernel_pmap_store)
//#endif

/*
 * For each vm_page_t, there is a list of all currently valid virtual
 * mappings of that page.  An entry is a pv_entry_t, the list is pv_table.
 */
typedef struct pv_entry {
	struct pv_entry			*pv_next;	/* next pv_entry */
	pmap_t					pv_pmap;	/* pmap where mapping lies */
	vm_offset_t				pv_va;		/* virtual address for mapping */
	int						pv_flags;	/* flags */

} *pv_entry_t;

#define	PV_ENTRY_NULL	((pv_entry_t) 0)

#define	PV_CI			0x01		/* all entries must be cache inhibited */
#define PV_PTPAGE		0x02		/* entry maps a page table page */

//#ifdef	KERNEL

pv_entry_t				pv_table;	/* array of entries, one per page */

#define pa_index(pa)				atop(pa - vm_first_phys)
#define pa_to_pvh(pa)				(&pv_table[pa_index(pa)])

#define	pmap_resident_count(pmap)	((pmap)->pm_stats.resident_count)
#define	pmap_wired_count(pmap)		((pmap)->pm_stats.wired_count)

extern int pae_mode;
extern int i386_pmap_VM_NFREEORDER;
extern int i386_pmap_VM_LEVEL_0_ORDER;
extern int i386_pmap_PDRSHIFT;
#endif	KERNEL

#endif /* SYS_DEVEL_ARCH_I386_PMAP3_H_ */
