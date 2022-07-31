/*	 $NetBSD: i82093reg.h,v 1.11 2017/11/13 11:45:54 nakayama Exp $ */

#ifndef _I386_I82093_H_
#define _I386_I82093_H_

#include <machine/specialreg.h>
#include <machine/apic/ioapicreg.h>
#include <machine/apic/lapicreg.h>

#define IOAPIC_ICU(irq_num)												 \
		movl	_C_LABEL(local_apic_va),%eax							;\
		movl	$0,LAPIC_EOI(%eax)

#define X2APIC_ICU(irq_num)												 \
		movl	$(MSR_APIC_000 + MSR_APIC_EOI),%ecx 				;\
		xorl	%eax,%eax												;\
		xorl	%edx,%edx												;\
		wrmsr

#define IOAPIC_MASK(irq_num)											 \
		movl	IS_PIC(%ebp),%edi										;\
		movl	IS_PIN(%ebp),%esi										;\
		leal	0x10(%esi,%esi,1),%esi									;\
		movl	SPIC_IOAPIC(%edi),%edi									;\
		movl	IOAPIC_SC_REG(%edi),%ebx								;\
		movl	%esi, (%ebx)											;\
		movl	IOAPIC_SC_DATA(%edi),%ebx								;\
		movl	(%ebx),%esi												;\
		orl		$IOAPIC_REDLO_MASK,%esi									;\
		andl	$~IOAPIC_REDLO_RIRR,%esi								;\
		movl	%esi,(%ebx)												;\
		movl	IS_PIC(%ebp),%edi										;\

#define IOAPIC_UNMASK(irq_num)											 \
		movl    (%esp),%eax												;\
		cmpl    $IREENT_MAGIC,(TF_ERR+4)(%eax)							;\
		jne     79f														;\
		movl	IS_PIC(%ebp),%edi										;\
		movl	IS_PIN(%ebp),%esi										;\
		leal	0x10(%esi,%esi,1),%esi									;\
		movl	SPIC_IOAPIC(%edi),%edi									;\
		movl	IOAPIC_SC_REG(%edi),%ebx								;\
		movl	IOAPIC_SC_DATA(%edi),%eax								;\
		movl	%esi, (%ebx)											;\
		movl	(%eax),%edx												;\
		andl	$~(IOAPIC_REDLO_MASK|IOAPIC_REDLO_RIRR),%edx			;\
		movl	%esi, (%ebx)											;\
		movl	%edx,(%eax)												;\
		movl	IS_PIC(%ebp),%edi										;\
79:

#endif /* !_I386_I82093_H_ */
