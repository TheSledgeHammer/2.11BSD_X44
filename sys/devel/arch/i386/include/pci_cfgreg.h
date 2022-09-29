/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 1997, Stefan Esser <se@freebsd.org>
 * All rights reserved.
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

#ifndef __I386_PCI_CFGREG_H__
#define	__I386_PCI_CFGREG_H__
/*
#define CONF1_ADDR_PORT    0x0cf8
#define CONF1_DATA_PORT    0x0cfc

#define CONF1_ENABLE       0x80000000ul
#define CONF1_ENABLE_CHK   0x80000000ul
#define CONF1_ENABLE_MSK   0x7f000000ul
#define CONF1_ENABLE_CHK1  0xff000001ul
#define CONF1_ENABLE_MSK1  0x80000001ul
#define CONF1_ENABLE_RES1  0x80000000ul

#define CONF2_ENABLE_PORT  0x0cf8
#define CONF2_FORWARD_PORT 0x0cfa

#define CONF2_ENABLE_CHK   0x0e
#define CONF2_ENABLE_RES   0x0e
*/

/* Convert to NetBSD equivalent */
/* some PCI bus constants */
#define	PCI_SLOTMAX				31		/* highest supported slot number */
#define	PCI_FUNCMAX				7		/* highest supported function number */
#define	PCIE_REGMAX				4095	/* highest supported config register addr. */

#define	PCI_INVALID_IRQ			255
#define	PCI_INTERRUPT_VALID(x)	((x) != PCI_INVALID_IRQ /*|| I386_PCI_INTERRUPT_LINE_NO_CONNECTION*/)

#define	PCIR_INTLINE			0x3c
#define	PCIR_INTPIN				0x3d

enum {
	CFGMECH_NONE = 0,
	CFGMECH_1,
	CFGMECH_2,
	CFGMECH_PCIE,
};

extern int cfgmech;

/* NetBSD / FreeBSD */
#define	PCI_MODE1_ADDRESS_REG	0x0cf8
#define	PCI_MODE1_DATA_REG		0x0cfc

#define	PCI_MODE1_ENABLE		0x80000000UL
#define PCI_MODE1_ENABLE_CHK   	0x80000000UL
#define PCI_MODE1_ENABLE_MSK   	0x7f000000UL
#define PCI_MODE1_ENABLE_CHK1  	0xff000001UL
#define PCI_MODE1_ENABLE_MSK1  	0x80000001UL
#define PCI_MODE1_ENABLE_RES1  	0x80000000UL

#define	PCI_MODE2_ENABLE_REG	0x0cf8
#define	PCI_MODE2_FORWARD_REG	0x0cfa

#define PCI_MODE2_ENABLE_CHK   	0x0e
#define PCI_MODE2_ENABLE_RES   	0x0e

#endif /* !__I386_PCI_CFGREG_H__ */
