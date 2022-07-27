#	$NetBSD: Makefile,v 1.50 2019/09/13 02:19:45 manu Exp $

INCSDIR=	/usr/include/i386

INCS=		ansi.h aout_machdep.h asm.h atomic.h bios.h bootinfo.h bus.h 			\
			coff_machdep.h cpu.h cpufunc.h cpuinfo.h cputypes.h cpuvar.h 			\
			db_machdep.h ecoff_machdep.h elf_machdep.h endian.h exec.h 				\
			float.h frame.h gdt.h intr.h limits.h loadfile_machdep.h 				\
			macho_machdep.h npx.h param.h pcb.h pic.h pio.h pmap.h pmap_reg.h 		\
			pmap_tlb.h proc.h profile.h psl.h pte.h reg.h segments.h setjmp.h 		\
			signal.h specialreg.h stdarg.h sysarch.h trap.h tss.h types.h vm86.h 	\
			vmparam.h xcoff_machdep.h
			
# MP/SMP
INCS+=		mcontext.h mpbiosreg.h mpbiosvar.h mpconfig.h percpu.h smp.h
# APIC
INCS+= 		apic/apic.h apic/ioapicreg.h apic/ioapicvar.h apic/lapicreg.h 			\
			apic/lapicvar.h
			
# machdeps needed outside of machine arch
# EISA
INCS+=		eisa/eisa_machdep.h
# ISA
INCS+=		isa/isa_machdep.h isa/isapnp_machdep.h
# MCA
INCS+=		mca/mca_machdep.h
# PCI
INCS+=		pci/pci_machdep.h

.include <bsd.kinc.mk>
