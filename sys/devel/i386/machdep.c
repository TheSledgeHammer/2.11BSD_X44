/*
 * machdep.c
 *
 *  Created on: 26 Apr 2020
 *      Author: marti
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>
#include <sys/map.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/exec.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/callout.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/msgbuf.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/syscallargs.h>

#include <dev/cons.h>

#include <vm/include/vm.h>
#include <vm/include/vm_kern.h>
#include <vm/include/vm_page.h>

#include <sys/sysctl.h>

#define _I386_BUS_DMA_PRIVATE
#include <machine/bus.h>

#include <machine/cpu.h>
#include <machine/cpufunc.h>
#include <machine/gdt.h>
#include <machine/pio.h>
#include <machine/psl.h>
#include <machine/reg.h>
#include <machine/specialreg.h>
#include <machine/bootinfo.h>

#include <dev/isa/isa.h>
#include <dev/isa/isa_device.h>
#include <dev/ic/i8042reg.h>
#include <dev/ic/mc146818reg.h>
#include <i386/isa/isa_machdep.h>
#include <i386/isa/nvram.h>

#ifdef VM86
#include <machine/vm86.h>
#endif


void
setgate(gd, func, args, type, dpl)
	struct gate_descriptor *gd;
	void *func;
	int args, type, dpl;
{

	gd->gd_looffset = (int)func;
	gd->gd_selector = GSEL(GCODE_SEL, SEL_KPL);
	gd->gd_stkcpy = args;
	gd->gd_xx = 0;
	gd->gd_type = type;
	gd->gd_dpl = dpl;
	gd->gd_p = 1;
	gd->gd_hioffset = (int)func >> 16;
}

void
setsegment(sd, base, limit, type, dpl, def32, gran)
	struct segment_descriptor *sd;
	void *base;
	size_t limit;
	int type, dpl, def32, gran;
{
	sd->sd_lolimit = (int)limit;
	sd->sd_lobase = (int)base;
	sd->sd_type = type;
	sd->sd_dpl = dpl;
	sd->sd_p = 1;
	sd->sd_hilimit = (int)limit >> 16;
	sd->sd_xx = 0;
	sd->sd_def32 = def32;
	sd->sd_gran = gran;
	sd->sd_hibase = (int)base >> 24;
}

void
cpu_reboot(howto, bootstr)
	int howto;
	char *bootstr;
{
	extern int cold;

	if (cold) {
		howto |= RB_HALT;
		goto haltsys;
	}

	boothowto = howto;
	if ((howto & RB_NOSYNC) == 0 && waittime < 0) {
		waittime = 0;
		vfs_shutdown();
		/*
		 * If we've been adjusting the clock, the todr
		 * will be out of synch; adjust it now.
		 */
		resettodr();
	}

	/* Disable interrupts. */
	splhigh();

	/* Do a dump if requested. */
	if ((howto & (RB_DUMP | RB_HALT)) == RB_DUMP)
		dumpsys();

haltsys:
	doshutdownhooks();

	if (howto & RB_HALT) {
#if NAPM > 0 && !defined(APM_NO_POWEROFF)
		/* turn off, if we can.  But try to turn disk off and
		 * wait a bit first--some disk drives are slow to clean up
		 * and users have reported disk corruption.
		 */
		delay(500000);
		apm_set_powstate(APM_DEV_DISK(0xff), APM_SYS_OFF);
		delay(500000);
		apm_set_powstate(APM_DEV_ALLDEVS, APM_SYS_OFF);
#endif
		printf("\n");
		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");
		cngetc();
	}

	printf("rebooting...\n");
	cpu_reset();
	for(;;) ;
	/*NOTREACHED*/
}

/*
 * Initialize segments and descriptor tables
 */

union descriptor static_gdt[NGDT], *gdt = static_gdt;
union descriptor static_ldt[NLDT];
struct gate_descriptor static_idt[NIDT], *idt = static_idt;


void
setregion(rd, base, limit)
	struct region_descriptor *rd;
	void *base;
	size_t limit;
{
	rd->rd_limit = (int)limit;
	rd->rd_base = (int)base;
}

void
init386(first_avail)
	vm_offset_t first_avail;
{
	int x;
	struct region_descriptor region;

	proc0.p_addr = proc0paddr;

	/* make gdt gates and memory segments */
	setsegment(&static_gdt[GCODE_SEL].sd, 0, 0xfffff, SDT_MEMERA, SEL_KPL, 1,
			1);
	setsegment(&static_gdt[GDATA_SEL].sd, 0, 0xfffff, SDT_MEMRWA, SEL_KPL, 1,
			1);
	setsegment(&static_gdt[GLDT_SEL].sd, static_ldt, sizeof(static_ldt) - 1,
			SDT_SYSLDT, SEL_KPL, 0, 0);
	setsegment(&static_gdt[GUCODE_SEL].sd, 0, i386_btop(VM_MAXUSER_ADDRESS) - 1,
			SDT_MEMERA, SEL_UPL, 1, 1);
	setsegment(&static_gdt[GUDATA_SEL].sd, 0, i386_btop(VM_MAXUSER_ADDRESS) - 1,
			SDT_MEMRWA, SEL_UPL, 1, 1);

	/* make ldt gates and memory segments */
	setgate(&static_ldt[LSYS5CALLS_SEL].gd, &IDTVEC(osyscall), 1, SDT_SYS386CGT, SEL_UPL);
	static_ldt[LUCODE_SEL] = static_gdt[GUCODE_SEL];
	static_ldt[LUDATA_SEL] = static_gdt[GUDATA_SEL];
	static_ldt[LSOL26CALLS_SEL] = static_ldt[LSYS5CALLS_SEL];
	static_ldt[LBSDICALLS_SEL] = static_ldt[LSYS5CALLS_SEL];

	/* exceptions */
	for (x = 0; x < 32; x++)
		setgate(&static_idt[x], IDTVEC(exceptions)[x], 0, SDT_SYS386TGT,
				(x == 3 || x == 4) ? SEL_UPL : SEL_KPL);

	/* new-style interrupt gate for syscalls */
	setgate(&static_idt[128], &IDTVEC(syscall), 0, SDT_SYS386TGT, SEL_UPL);

	setregion(&region, static_gdt, sizeof(static_gdt) - 1);
	lgdt(&region);
	setregion(&region, static_idt, sizeof(static_idt) - 1);
	lidt(&region);

#if NISA > 0
	isa_defaultirq();
#endif

	splraise(-1);
	enable_intr();

	init_memio_ports(biosbasemem, biosextmem);
}


void *
lookup_bootinfo(type)
	int type;
{
	struct btinfo_common *help;
	int n = *(int*)bootinfo;
	help = (struct btinfo_common *)(bootinfo + sizeof(int));
	while(n--) {
		if(help->type == type)
			return(help);
		help = (struct btinfo_common *)((char*)help + help->len);
	}
	return(0);
}

/*
 * consinit:
 * initialize the system console.
 * XXX - shouldn't deal with this initted thing, but then,
 * it shouldn't be called from init386 either.
 */
void
consinit()
{
	struct btinfo_console *consinfo;
	static int initted;

	if (initted)
		return;
	initted = 1;

#ifndef CONS_OVERRIDE
	consinfo = lookup_bootinfo(BTINFO_CONSOLE);
	if (!consinfo)
#endif
		consinfo = &default_consinfo;

#if (NPC > 0) || (NVT > 0)
	if(!strcmp(consinfo->devname, "pc")) {
		pccnattach();
		return;
	}
#endif
#if (NCOM > 0)
	if(!strcmp(consinfo->devname, "com")) {
		bus_space_tag_t tag = I386_BUS_SPACE_IO;

		if(comcnattach(tag, consinfo->addr, consinfo->speed,
			       COM_FREQ, comcnmode))
			panic("can't init serial console @%x", consinfo->addr);

		return;
	}
#endif
	panic("invalid console device %s", consinfo->devname);
}

#ifdef KGDB
void
kgdb_port_init()
{
#if (NCOM > 0)
	if(!strcmp(kgdb_devname, "com")) {
		bus_space_tag_t tag = I386_BUS_SPACE_IO;

		com_kgdb_attach(tag, comkgdbaddr, comkgdbrate, COM_FREQ,
		    comkgdbmode);
	}
#endif
}
#endif

void
cpu_reset()
{
	struct region_descriptor region;

	disable_intr();

	/*
	 * The keyboard controller has 4 random output pins, one of which is
	 * connected to the RESET pin on the CPU in many PCs.  We tell the
	 * keyboard controller to pulse this line a couple of times.
	 */
	outb(KBCMDP, KBC_PULSE0);
	delay(100000);
	outb(KBCMDP, KBC_PULSE0);
	delay(100000);

	/*
	 * Try to cause a triple fault and watchdog reset by making the IDT
	 * invalid and causing a fault.
	 */
	bzero((caddr_t) idt, NIDT * sizeof(idt[0]));
	setregion(&region, idt, NIDT * sizeof(idt[0]) - 1);
	lidt(&region);
	__asm __volatile("divl %0,%1" : : "q" (0), "a" (0));

#if 0
	/*
	 * Try to cause a triple fault and watchdog reset by unmapping the
	 * entire address space and doing a TLB flush.
	 */
	bzero((caddr_t)PTD, NBPG);
	pmap_update();
#endif

	for (;;)
		;
}
