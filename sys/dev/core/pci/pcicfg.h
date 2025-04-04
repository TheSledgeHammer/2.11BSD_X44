/*
 * The 3-Clause BSD License:
 * Copyright (c) 2024 Martin Kelly
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

#ifndef _DEV_PCI_PCICFG_H_
#define	_DEV_PCI_PCICFG_H_

#ifdef _KERNEL
void    pci_conf_print_common(pci_chipset_tag_t, pcitag_t, const pcireg_t *);
void    pci_conf_print_regs(const pcireg_t *, int, int);
void    pci_conf_print_type0(pci_chipset_tag_t, pcitag_t, const pcireg_t *);
void    pci_conf_print_type1(pci_chipset_tag_t, pcitag_t, const pcireg_t *);
void    pci_conf_print_type2(pci_chipset_tag_t, pcitag_t, const pcireg_t *);
#else
void    pci_conf_print_regs(const pcireg_t *, int, int);
void    pci_conf_print_type0(const pcireg_t *);
void    pci_conf_print_type1(const pcireg_t *);
void    pci_conf_print_type2(const pcireg_t *);
void    pci_conf_printX(int, const pcireg_t *);
#endif
#endif /* _DEV_PCI_PCICFG_H_ */
