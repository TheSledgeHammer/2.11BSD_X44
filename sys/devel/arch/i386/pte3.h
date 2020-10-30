/*
 * pte3.h
 *
 *  Created on: 22 Oct 2020
 *      Author: marti
 */

#ifndef SYS_DEVEL_ARCH_I386_PTE3_H_
#define SYS_DEVEL_ARCH_I386_PTE3_H_

#define	PG_RO		0x000		/* R/O	Read-Only		*/
#define	PG_V		0x001		/* P	Valid			*/
#define	PG_RW		0x002		/* R/W	Read/Write		*/
#define	PG_u		0x004
#define	PG_PROT		0x006 		/* all protection bits .*/
#define	PG_NC_PWT	0x008		/* PWT	Write through	*/
#define	PG_NC_PCD	0x010		/* PCD	Cache disable	*/
#define PG_U		0x020		/* U/S  User/Supervisor	*/
#define	PG_M		0x040		/* D	Dirty			*/
#define PG_A		0x060		/* A	Accessed		*/
#define	PG_PS		0x080		/* PS	Page size (0=4k,1=4M)	*/
#define	PG_PTE_PAT	0x080		/* PAT	PAT index		*/
#define	PG_G		0x100		/* G	Global			*/
#define	PG_W		0x200		/* "Wired" pseudoflag 	*/
#define	PG_SWAPM	0x400
#define	PG_FOD		0x600
#define PG_N		0x800 		/* Non-cacheable 		*/
#define	PG_PDE_PAT	0x1000		/* PAT	PAT index		*/
#define	PG_NX		(1ull<<63) 	/* No-execute 			*/

#define	PG_FRAME	0xfffff000

#define	PG_NOACC	0
#define	PG_KR		0x2000
#define	PG_KW		0x4000
#define	PG_URKR		0x6000
#define	PG_URKW		0x6000
#define	PG_UW		0x8000

#define	PG_FZERO	0
#define	PG_FTEXT	1
#define	PG_FMAX		(PG_FTEXT)

/*
 * Page Protection Exception bits
 */
#define PGEX_P		0x01		/* Protection violation vs. not present */
#define PGEX_W		0x02		/* during a Write cycle */
#define PGEX_U		0x04		/* access from User mode (UPL) */
#define PGEX_RSV	0x08		/* reserved PTE field is non-zero */
#define PGEX_I		0x10		/* during an instruction fetch */

extern pt_entry_t	PTmap[], APTmap[], Upte;
extern pd_entry_t	PTD[], APTD[], PTDpde, APTDpde, Upde;
extern pt_entry_t	*Sysmap;

extern pt_entry_t	IdlePTD;	/* physical address of "Idle" state directory */
extern pt_entry_t	KPTmap;

#endif /* SYS_DEVEL_ARCH_I386_PTE3_H_ */
