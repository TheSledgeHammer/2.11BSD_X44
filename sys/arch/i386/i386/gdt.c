/*
 * Copyright (c) 2020 Martin Kelly
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>

#include <machine/segments.h>
#include <machine/gdt.h>

/* Defines all the parameters for the soft segment descriptors */
void
setup_descriptor_table(ssd, ssd_base, ssd_limit, ssd_type, ssd_dpl, ssd_p, ssd_xx, ssd_xx1, ssd_def32, ssd_gran)
    struct soft_segment_descriptor *ssd;
    unsigned int ssd_base, ssd_limit, ssd_type, ssd_dpl, ssd_p, ssd_xx, ssd_xx1, ssd_def32, ssd_gran;
{
    ssd->ssd_base = ssd_base;
    ssd->ssd_limit = ssd_limit;
    ssd->ssd_type = ssd_type;
    ssd->ssd_dpl = ssd_dpl;
    ssd->ssd_p = ssd_p;
    ssd->ssd_xx = ssd_xx;
    ssd->ssd_xx1 = ssd_xx1;
    ssd->ssd_def32 = ssd_def32;
    ssd->ssd_gran = ssd_gran;
}

/* Allocates & Predefines all the parameters for the Global Descriptor Table (GDT) segments */
void
allocate_gdt(gdt)
    struct soft_segment_descriptor *gdt[];
{
	/* GNULL_SEL	0 Null Descriptor */
	setup_descriptor_table(&gdt[GNULL_SEL], 0x0, 0x0, 0, SEL_KPL, 0, 0, 0, 0, 0);
	/* GUFS_SEL		1 %fs Descriptor for user */
	setup_descriptor_table(&gdt[GUFS_SEL], 0x0, 0xfffff, SDT_MEMRWA, SEL_UPL, 1, 0, 0, 1, 1);
	/* GUGS_SEL		2 %gs Descriptor for user */
	setup_descriptor_table(&gdt[GUGS_SEL], 0x0, 0xfffff, SDT_MEMRWA, SEL_UPL, 1, 0, 0, 1, 1);
	/* GCODE_SEL	3 Code Descriptor for kernel */
	setup_descriptor_table(&gdt[GCODE_SEL], 0x0, 0xfffff, SDT_MEMERA, SEL_KPL, 1, 0, 0, 1, 1);
	/* GDATA_SEL	4 Data Descriptor for kernel */
	setup_descriptor_table(&gdt[GDATA_SEL], 0x0, 0xfffff, SDT_MEMERA, SEL_KPL, 1, 0, 0, 1, 1);
	/* GUCODE_SEL	5 Code Descriptor for user */
	setup_descriptor_table(&gdt[GUCODE_SEL], 0x0, 0xfffff, SDT_MEMERA, SEL_UPL, 1, 0, 0, 1, 1);
	/* GUDATA_SEL	6 Data Descriptor for user */
	setup_descriptor_table(&gdt[GUDATA_SEL], 0x0, 0xfffff, SDT_MEMERA, SEL_UPL, 1, 0, 0, 1, 1);
	/* GPROC0_SEL	7 Proc 0 Tss Descriptor */
	setup_descriptor_table(&gdt[GPROC0_SEL], 0x0, sizeof(struct i386tss)-1, SDT_SYS386TSS, 0, 1, 0, 0, 0, 0);
	 /* GLDT_SEL	8 LDT Descriptor */
	setup_descriptor_table(&gdt[GLDT_SEL], 0x0, (sizeof(union descriptor) * NLDT - 1), SDT_SYSLDT, SEL_UPL, 1, 0, 0, 0, 0);
	/* GUSERLDT_SEL	9 User LDT Descriptor per process */
	setup_descriptor_table(&gdt[GUSERLDT_SEL], 0x0, (512 * sizeof(union descriptor)-1), SDT_SYSLDT, 0, 1, 0, 0, 0, 0);
	/* GPANIC_SEL	10 Panic Tss Descriptor */
	setup_descriptor_table(&gdt[GPANIC_SEL], 0x0, sizeof(struct i386tss)-1, SDT_SYS386TSS, 0, 1, 0, 0, 0, 0);
	/* GTGATE_SEL	11 Process task switch gate */
	setup_descriptor_table(&gdt[GTGATE_SEL], 0x0, sizeof(struct i386tss)-1, SDT_SYS386TSS, 0, 1, 0, 0, 0, 0);
	/* GINVTSS_SEL	12 Invalid Tss fault Tss Descriptor */
	setup_descriptor_table(&gdt[GINVTSS_SEL], 0x0, sizeof(struct i386tss)-1, SDT_SYS386TSS, 0, 1, 0, 0, 0, 0);
	/* GDBLFLT_SEL	13 Double fault Tss Descriptor */
	setup_descriptor_table(&gdt[GDBLFLT_SEL], 0x0, sizeof(struct i386tss)-1, SDT_SYS386TSS, 0, 1, 0, 0, 0, 0);
	/* GEXIT_SEL	14 Exit Tss Descriptor */
	setup_descriptor_table(&gdt[GEXIT_SEL], 0x0, sizeof(struct i386tss)-1, SDT_SYS386TSS, 0, 1, 0, 0, 0, 0);

	/* GBIOSCODE32_SEL 15 BIOS 32-bit interface (32bit Code) */
	setup_descriptor_table(&gdt[GBIOSCODE32_SEL], 0x0, 0xfffff, SDT_MEMERA, 0, 1, 0, 0, 0, 1);
	/* GBIOSCODE16_SEL 16 BIOS 32-bit interface (16bit Code) */
	setup_descriptor_table(&gdt[GBIOSCODE16_SEL], 0x0, 0xfffff, SDT_MEMERA, 0, 1, 0, 0, 0, 1);
	/* GBIOSDATA_SEL 17 BIOS 32-bit interface (Data) */
	setup_descriptor_table(&gdt[GBIOSDATA_SEL], 0x0, 0xfffff, SDT_MEMERA, 0, 1, 0, 0, 0, 1);
	/* GBIOSUTIL_SEL 18 BIOS 16-bit interface (Utility) */
	setup_descriptor_table(&gdt[GBIOSUTIL_SEL], 0x0, 0xfffff, SDT_MEMERA, 0, 1, 0, 0, 0, 1);
	/* GBIOSARGS_SEL 19 BIOS 16-bit interface (Arguments) */
	setup_descriptor_table(&gdt[GBIOSARGS_SEL], 0x0, 0xfffff, SDT_MEMERA, 0, 1, 0, 0, 0, 1);
}

/* Allocates & Predefines all the parameters for the Local Descriptor Table (LDT) segments */
void
allocate_ldt(ldt)
	struct soft_segment_descriptor *ldt[];
{
	setup_descriptor_table(&ldt[LUNULL_SEL], 0x0, 0x0, 0, 0, 0, 0, 0, 0, 0);
    setup_descriptor_table(&ldt[LUNULL_SEL], 0x0, 0x0, 0, 0, 0, 0, 0, 0, 0);
    setup_descriptor_table(&ldt[LUNULL_SEL], 0x0, 0x0, 0, 0, 0, 0, 0, 0, 0);
    setup_descriptor_table(&ldt[LUCODE_SEL], 0x0, 0xfffff, SDT_MEMERA, SEL_UPL, 1, 0, 0, 1, 1);
    setup_descriptor_table(&ldt[LUNULL_SEL], 0x0, 0x0, 0, 0, 0, 0, 0, 0, 0);
    setup_descriptor_table(&ldt[LUDATA_SEL], 0x0, 0xfffff, SDT_MEMERA, SEL_UPL, 1, 0, 0, 1, 1);
}
