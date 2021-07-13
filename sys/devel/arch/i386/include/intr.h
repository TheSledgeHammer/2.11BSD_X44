/*	$NetBSD: intrdefs.h,v 1.25 2021/03/18 01:50:12 nonaka Exp $	*/

#ifndef _X86_INTRDEFS_H_
#define _X86_INTRDEFS_H_

/*
 * Local APIC masks and software interrupt masks, in order
 * of priority.  Must not conflict with SIR_* below.
 */
#define LIR_IPI		31
#define LIR_TIMER	30

#define I386_NIPI	9

#endif /* _X86_INTRDEFS_H_ */
