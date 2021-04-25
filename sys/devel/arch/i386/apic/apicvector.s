
#include <devel/i386/isa/icu.h>
#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#define	IRQ_BIT(irq_num)	    	(1 << ((irq_num) % 8))
#define	IRQ_BYTE(irq_num)	    	((irq_num) / 8)

#define INTR_ADDR(intr, irq_num) 	((intr)+(irq_num) * 4)

#define	MASK(irq_num, icu)                                          	\
		movb	_imen + IRQ_BYTE(irq_num),%al							;\
		orb		$IRQ_BIT(irq_num),%al									;\
		movb	%al,_imen + IRQ_BYTE(irq_num)							;\
		FASTER_NOP														;\
		outb	%al,$(icu+1)

#define	UNMASK(irq_num, icu)                                        	\
		cli																;\
		movb	_imen + IRQ_BYTE(irq_num),%al							;\
		andb	$~IRQ_BIT(irq_num),%al									;\
		movb	%al,_imen + IRQ_BYTE(irq_num)							;\
		FASTER_NOP														;\
		outb	%al,$(icu+1)											;\
		sti

#define APIC_STRAY(irq_num)                                         	\
IDTVEC(stray/**/irq_num)                                            	;\
		call	_isa_strayintr					                        ;\
		addl	$4,%esp						    	                    ;\
		jmp		5b		

#define APIC_HOLD(irq_num)                                          	\
IDTVEC(hold/**/irq_num)													;\
		orb	$IRQ_BIT(irq_num),_ipending + IRQ_BYTE(irq_num)				;\
		INTRFASTEXIT

#define APIC_RECURSE(num)                                     			\
IDTVEC(recurse_/**/num)													;\
		pushfl															;\
		pushl	%cs														;\
		pushl	%esi													;\
		subl	$4,%esp													;\
		pushl	$T_ASTFLT				/* trap # for doing ASTs */		;\
		INTRENTRY														;\

#define APIC_RESUME(irq_num)                                         	\
IDTVEC(resume_/**/num)													;\
		movl	$IREENT_MAGIC,TF_ERR(%esp)								;\
		movl	%ebx,%esi												;\
		movl	INTR_ADDR(intrsource, irq_num), %ebp					;\
		movl	IS_MAXLEVEL(%ebp),%ebx									;\
		jmp		1f														;\

#define	APICINTR(irq_num, icu, enable_icus)                         	\
IDTVEC(apic_intr/**/irq_num)                                        	;\
    	pushl	$0					    /* dummy error code */			;\
		pushl	$T_ASTFLT			    /* trap # for doing ASTs */		;\
		INTRENTRY													    ;\
    	MASK(irq_num, icu)				/* mask it in hardware */		;\
		enable_icus(irq_num)			/* and allow other intrs */		;\
    	testb	$IRQ_BIT(irq_num),_cpl + IRQ_BYTE(irq_num)				;\
		APIC_HOLD(irq_num)		        /* currently masked; hold it */	;\
    	APIC_RESUME(irq_num)                                            ;\
    	movl	_cpl,%eax				/* cpl to restore on exit */	;\
		pushl	%eax													;\
		orl		INTR_ADDR(intrmask, irq_num),%eax						;\
		movl	%eax,_cpl				/* add in this intr's mask */	;\
		sti								/* safe to take intrs now */	;\
		movl	INTR_ADDR(intrhand, irq_num),%ebx	/* head of chain */	;\
		testl	%ebx,%ebx												;\
    	jz      APIC_STRAY(irq_num)                                     ;\
    	APIC_STRAY_INITIALIZE											;\
7:		movl	IH_ARG(%ebx),%eax		/* get handler arg */			;\
		testl	%eax,%eax												;\
		jnz		4f														;\
		movl	%esp,%eax				/* 0 means frame pointer */		;\
4:		pushl	%eax													;\
		call	IH_FUN(%ebx)			/* call it */					;\
		addl	$4,%esp					/* toss the arg */				;\
		STRAY_INTEGRATE													;\
		incl	IH_COUNT(%ebx)			/* count the intrs */			;\
		movl	IH_NEXT(%ebx),%ebx		/* next handler in chain */		;\
		testl	%ebx,%ebx												;\
		jnz		7b														;\
		STRAY_TEST														;\
5:		UNMASK(irq_num, icu)			/* unmask it in hardware */		;\
		INTREXIT						/* lower spl and do ASTs */		;\

#define	APIC_STRAY_INITIALIZE 	                                        \
		xorl	%esi,%esi				/* nobody claimed it yet */
#define	APIC_STRAY_INTEGRATE 	                                        \
		orl		%eax,%esi				/* maybe he claimed it */	
#define	APIC_STRAY_TEST 			                                    \
		testl	%esi,%esi				/* no more handlers */			;\
		jz		APIC_STRAY(irq_num)		/* nobody claimed it */


APICINTR(0, IO_ICU1, ENABLE_ICU1)
APICINTR(1, IO_ICU1, ENABLE_ICU1)
APICINTR(2, IO_ICU1, ENABLE_ICU1)
APICINTR(3, IO_ICU1, ENABLE_ICU1)
APICINTR(4, IO_ICU1, ENABLE_ICU1)
APICINTR(5, IO_ICU1, ENABLE_ICU1)
APICINTR(6, IO_ICU1, ENABLE_ICU1)
APICINTR(7, IO_ICU1, ENABLE_ICU1)
APICINTR(8, IO_ICU2, ENABLE_ICU1_AND_2)
APICINTR(9, IO_ICU2, ENABLE_ICU1_AND_2)
APICINTR(10, IO_ICU2, ENABLE_ICU1_AND_2)
APICINTR(11, IO_ICU2, ENABLE_ICU1_AND_2)
APICINTR(12, IO_ICU2, ENABLE_ICU1_AND_2)
APICINTR(13, IO_ICU2, ENABLE_ICU1_AND_2)
APICINTR(14, IO_ICU2, ENABLE_ICU1_AND_2)
APICINTR(15, IO_ICU2, ENABLE_ICU1_AND_2)
