/*	$NetBSD: pci_subr.c,v 1.21 1997/09/13 08:49:51 enami Exp $	*/

/*
 * Copyright (c) 1995, 1996 Christopher G. Demetriou.  All rights reserved.
 * Copyright (c) 1994 Charles Hannum.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Charles Hannum.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

/*
 * This is a Temporary file!! Related to libpci and pcictl
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/user.h>

#ifdef _KERNEL
typedef void (*pci_conf_func_t)(pci_chipset_tag_t , pcitag_t, const pcireg_t *);
#else
typedef void (*pci_conf_func_t)(const pcireg_t *);
#endif

/*
 * pci_conf kern/user interface
 */
struct pci_cfg {
    int                 pc_type;
    pci_chipset_tag_t   pc_pc;
    pcitag_t            pc_tag;
    const pcireg_t 		*pc_regs;
};

struct pci_cfg pci_cfg_array[3];


struct pci_cfg *pci_cfg_get(int, const pcireg_t *);
void pci_cfg_set(int, pci_chipset_tag_t, pcitag_t, const pcireg_t *);

static void pci_uconf_print_common(struct pci_cfg *, const pcireg_t *);
static void pci_uconf_print_type0(const pcireg_t *);
static void pci_uconf_print_type1(const pcireg_t *);
static void pci_uconf_print_type2(const pcireg_t *);

static void pci_conf_print_typeX(struct pci_cfg *, int, const pcireg_t *, pci_conf_func_t);

struct pci_cfg *
pci_cfg_get(int type, const pcireg_t *regs)
{
	struct pci_cfg *cfg;

	cfg = &pci_cfg_array[type];
	if (cfg != NULL) {
		if ((cfg->pc_type == type) && (cfg->pc_regs == regs)) {
			return (cfg);
		}
	}
	return (NULL);
}

void
pci_cfg_set(int type, pci_chipset_tag_t pc, pcitag_t tag, const pcireg_t *regs)
{
	struct pci_cfg *cfg;

	cfg = &pci_cfg_array[type];
	cfg->pc_type = type;
	cfg->pc_pc = pc;
	cfg->pc_tag = tag;
	cfg->pc_regs = regs;
}

static void
pci_uconf_print_common(struct pci_cfg *cfg, const pcireg_t *regs)
{
	pci_conf_print_common(cfg->pc_pc, cfg->pc_tag, regs);
}

static void
pci_uconf_print_type0(const pcireg_t *regs)
{
	struct pci_cfg *cfg = pci_cfg_get(0, regs);
	if (cfg != NULL) {
		pci_conf_print_type0(cfg->pc_pc, cfg->pc_tag, regs);
	}
}

static void
pci_uconf_print_type1(const pcireg_t *regs)
{
	struct pci_cfg *cfg = pci_cfg_get(1, regs);
	if (cfg != NULL) {
		pci_conf_print_type1(cfg->pc_pc, cfg->pc_tag, regs);
	}
}

static void
pci_uconf_print_type2(const pcireg_t *regs)
{
	struct pci_cfg *cfg = pci_cfg_get(2, regs);
	if (cfg != NULL) {
		pci_conf_print_type2(cfg->pc_pc, cfg->pc_tag, regs);
	}
}

static void
pci_conf_print_typeX(struct pci_cfg *cfg, int hdrtype, const pcireg_t *regs, pci_conf_func_t printfn)
{
	pci_conf_func_t typeprintfn;
	const char *typename;
	int endoff;

	/* common header */
	printf("  Common header:\n");
	pci_conf_print_regs(regs, 0, 16);

	printf("\n");
	pci_uconf_print_common(cfg, regs);
	printf("\n");

	/* type-dependent header */
	switch (cfg->pc_type) {
	case 0:
		/* Standard device header */
		typename = "\"normal\" device";
#ifdef _KERNEL
		typeprintfn = &pci_conf_print_type0;
#else
		typeprintfn = &pci_uconf_print_type0;
#endif
		endoff = 64;
		break;
	case 1:
		/* PCI-PCI bridge header */
		typename = "PCI-PCI bridge";
#ifdef _KERNEL
		typeprintfn = &pci_conf_print_type1;
#else
		typeprintfn = &pci_uconf_print_type1;
#endif
		endoff = 64;
		break;
	case 2:
		/* PCI-CardBus bridge header */
		typename = "PCI-CardBus bridge";
#ifdef _KERNEL
		typeprintfn = &pci_conf_print_type2;
#else
		typeprintfn = &pci_uconf_print_type2;
#endif
		endoff = 72;
		break;
	default:
		typename = NULL;
		typeprintfn = 0;
		endoff = 64;
		break;
	}
    printf("  Type %d ", cfg->pc_type);
    if (typename != NULL) {
        printf("(%s) ", typename);
    }
    printf("header:\n");
    pci_conf_print_regs(regs, 16, endoff);
    printf("\n");
	if (typeprintfn) {
		(*typeprintfn)(cfg->pc_pc, cfg->pc_tag, regs);
	} else {
		printf("    Don't know how to pretty-print type %d header.\n",
		    hdrtype);
	}
	printf("\n");

	/* device-dependent header */
	printf("  Device-dependent header:\n");
	pci_conf_print_regs(regs, endoff, 256);
	printf("\n");
	if (printfn) {
		(*printfn)(cfg->pc_pc, cfg->pc_tag, regs);
	} else {
		printf("    Don't know how to pretty-print device-dependent header.\n");
	}
	printf("\n");
}

#ifdef _KERNEL

void
pci_conf_print(pci_chipset_tag_t pc, pcitag_t tag, pci_conf_func_t printfn)
{
	pcireg_t regs[o2i(256)];
	struct pci_cfg *cfg;
	int off, hdrtype;

	printf("PCI configuration registers:\n");

	for (off = 0; off < 256; off += 4) {
		regs[o2i(off)] = pci_conf_read(pc, tag, off);
	}

	hdrtype = PCI_HDRTYPE(regs[o2i(PCI_BHLC_REG)]);
	cfg = pci_cfg_get(hdrtype, regs);
	if (cfg != NULL) {
		pci_conf_print_typeX(cfg, hdrtype, regs, printfn);
	}
}

#else

void
pci_conf_print(int pcifd, u_int bus, u_int dev, u_int func)
{
	pcireg_t regs[o2i(256)];
	struct pci_cfg *cfg;
	int off, hdrtype;

	printf("PCI configuration registers:\n");

	for (off = 0; off < 256; off += 4) {
		if (pcibus_conf_read(pcifd, bus, dev, func, off, &regs[o2i(off)]) == -1) {
			regs[o2i(off)] = 0;
		}
	}

	hdrtype = PCI_HDRTYPE(regs[o2i(PCI_BHLC_REG)]);
	cfg = pci_cfg_get(hdrtype, regs);
	if (cfg != NULL) {
		pci_conf_print_typeX(cfg, hdrtype, regs, NULL);
	}
}

#endif
