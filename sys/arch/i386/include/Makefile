#	$NetBSD: Makefile,v 1.50 2019/09/13 02:19:45 manu Exp $

INCSDIR=	/usr/include/i386

INCS=		ansi.h aout_machdep.h asm.h atomic.h bios.h bootinfo.h			\
			bus_dma.h bus_space.h coff_machdep.h cpu.h cpufunc.h cpuinfo.h	\
			cputypes.h cpuvar.h db_machdep.h dbg.h ecoff_machdep.h 			\
			elf_machdep.h endian_machdep.h exec.h float.h frame.h gdt.h 	\
			intr.h limits.h	loadfile_machdep.h macho_machdep.h math_emu.h 	\
			npx.h param.h pcb.h pic.h pio.h pmap_base.h pmap_nopae.h 		\
			pmap_pae.h pmap.h proc.h profile.h psl.h pte.h reg.h reloc.h 	\
			segments.h setjmp.h signal.h specialreg.h stdarg.h sysarch.h 	\
			trap.h tss.h types.h vm86.h vmparam.h xcoff_machdep.h
# MP/SMP
INCS+=		mpbiosreg.h mpbiosvar.h mpconfig.h
# APIC
INCS+= 		apic/apic.h apic/ioapicreg.h apic/ioapicvar.h apic/lapicreg.h 	\
			apic/lapicvar.h
	
.include <bsd.kinc.mk>