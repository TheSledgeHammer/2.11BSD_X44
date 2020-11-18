/*
 * ioapic.c
 *
 *  Created on: 18 Nov 2020
 *      Author: marti
 */

#include <devel/arch/i386/FBSD/apicreg.h>
#include <devel/arch/i386/FBSD/apicvar.h>

#define IOAPIC_ISA_INTS		16
#define	IOAPIC_MEM_REGION	32
#define	IOAPIC_REDTBL_LO(i)	(IOAPIC_REDTBL + (i) * 2)
#define	IOAPIC_REDTBL_HI(i)	(IOAPIC_REDTBL_LO(i) + 1)

struct ioapic_intsrc {
	struct intsrc 			io_intsrc;
	int 					io_irq;
	u_int 					io_intpin:8;
	u_int 					io_vector:8;
	u_int 					io_cpu;
	u_int 					io_activehi:1;
	u_int 					io_edgetrigger:1;
	u_int 					io_masked:1;
	int 					io_bus:4;
	uint32_t 				io_lowreg;
	u_int 					io_remap_cookie;
};

struct ioapic {
	struct pic 				io_pic;
	u_int 					io_id:8;			/* logical ID */
	u_int 					io_apic_id:8;		/* Id as enumerated by MADT */
	u_int 					io_hw_apic_id:8;	/* Content of APIC ID register */
	u_int 					io_intbase:8;		/* System Interrupt base */
	u_int 					io_numintr:8;
	u_int 					io_haseoi:1;
	volatile ioapic_t 		*io_addr;			/* XXX: should use bus_space */
	vm_paddr_t 				io_paddr;
	SIMPLEQ_ENTRY(ioapic) 	io_next;
	struct device			*pci_dev;			/* matched pci device, if found */
	struct resource 		*pci_wnd;			/* BAR 0, should be same or alias to io_paddr */
	struct ioapic_intsrc 	io_pins[0];
};
