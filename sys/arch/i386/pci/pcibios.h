/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright 1997, Stefan Esser <se@freebsd.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 *
 * $FreeBSD$
 *
 */

#ifndef _I386_PCIBIOS_H_
#define	_I386_PCIBIOS_H_

#define	PCI_IRQ_TABLE_START		0xf0000
#define	PCI_IRQ_TABLE_END		0xfffff

#define	PIR_DEVFUNC_DEVICE(devfunc)		(((devfunc) >> 3) & 0x1f)
#define	PIR_DEVFUNC_FUNCTION(devfunc)	((devfunc) & 7)

void pcibios_init(void);

extern struct PIR_table *pci_route_table;
extern int pci_route_count;
extern int pcibios_max_bus;

#ifdef PCIBIOSVERBOSE
extern int pcibiosverbose;

#define	PCIBIOS_PRINTV(arg) 		\
	do { 							\
		if (pcibiosverbose) 		\
			printf arg; 			\
	} while (0)
#define	PCIBIOS_PRINTVN(n, arg) 	\
	do { 							\
		 if (pcibiosverbose > (n)) 	\
			printf arg; 			\
	} while (0)
#else
#define	PCIBIOS_PRINTV(arg)
#define	PCIBIOS_PRINTVN(n, arg)
#endif
#endif /* !_I386_PCIBIOS_H_ */
