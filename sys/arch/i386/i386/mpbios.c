/*	$NetBSD: mpbios.c,v 1.19.4.1 2005/04/15 22:16:07 tron Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by RedBack Networks Inc.
 *
 * Author: Bill Sommerfeld
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1999 Stefan Grefen
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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR AND CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*
 * Derived from FreeBSD's mp_machdep.c
 */
/*
 * Copyright (c) 1996, by Steve Passe
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the developer may NOT be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * The Intel MP-stuff is just one way of x86 SMP systems
 * so only Intel MP specific stuff is here.
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <vm/include/vm.h>
#include <vm/include/vm_kern.h>
#include <vm/include/vm_param.h>
#include <vm/include/vm_extern.h>

#include <machine/param.h>
#include <machine/vmparam.h>
#include <machine/specialreg.h>
#include <machine/cpuvar.h>
#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/pic.h>
#include <machine/mpbiosvar.h>

#include <machine/apic/apic.h>
#include <machine/apic/ioapicreg.h>
#include <machine/apic/ioapicvar.h>
#include <machine/apic/lapicreg.h>
#include <machine/apic/lapicvar.h>

#include <dev/core/isa/isareg.h>

#ifdef I386_MPBIOS_SUPPORT_EISA
#include <dev/core/eisa/eisavar.h>	/* for ELCR* def'ns */
#endif

#include "pci.h"

#if NPCI > 0
#include <dev/core/pci/pcivar.h>
#include <dev/core/pci/pcireg.h>
#endif

static struct mpbios_ioapic default_ioapic = {
		.type = 2,
		.apic_id = 0,
		.apic_version = 1,
		.apic_flags = IOAPICENTRY_FLAG_EN,
		.apic_address = (u_int32_t)IOAPIC_BASE_DEFAULT
};

/* descriptions of MP basetable entries */
struct mpbios_baseentry {
	u_int8_t 	type;
	u_int8_t 	length;
	u_int16_t 	count;
	const char 	*name;
};

static const char *loc_where[] = {
		"extended bios data area",
		"last page of base memory",
		"bios"
};

struct mp_map {
	vm_offset_t		sva;	/* start address */
	vm_offset_t		eva;	/* end address */
	vm_offset_t 	baseva;	/* address */
	u_long 			pa;
	int	 			vsize;
	int				psize;
};

int mp_print(void *, const char *);
int mp_match(struct device *, struct cfdata *, void *);
static const void *mpbios_search(struct device *, u_long, int, struct mp_map *);
static inline int mpbios_cksum(const void *,int);

static void mp_cfg_special_intr(const struct mpbios_int *, u_int32_t *);
static void mp_print_special_intr(int);

static void mp_cfg_pci_intr(const struct mpbios_int *, u_int32_t *);
static void mp_print_pci_intr(int);

#ifdef I386_MPBIOS_SUPPORT_EISA
static void mp_print_eisa_intr(int);
static void mp_cfg_eisa_intr(const struct mpbios_int *, u_int32_t *);
#endif

static void mp_cfg_isa_intr(const struct mpbios_int *, u_int32_t *);
static void mp_print_isa_intr(int);

static void mpbios_cpus(struct device *);
static void mpbios_cpu(const u_int8_t *, struct device *);
static void mpbios_bus(const u_int8_t *, struct device *);
static void mpbios_ioapic(const u_int8_t *, struct device *);
static void mpbios_int(const u_int8_t *, int, struct mp_intr_map *);

static const void *mpbios_map(u_long, int, struct mp_map *);
static void mpbios_unmap(struct mp_map *);

/*
 * globals to help us bounce our way through parsing the config table.
 */

static struct mp_map 	mp_cfg_table_map;
static struct mp_map 	mp_fp_map;
const struct mpbios_cth *mp_cth;
const struct mpbios_fps *mp_fps;

int mpbios_scanned;

int
mp_print(aux, pnp)
	void *aux;
	const char *pnp;
{
	struct cpu_attach_args *caa = (struct cpu_attach_args*) aux;
	if (pnp)
		printf("%s at %s:", caa->caa_name, pnp);
	return (UNCONF);
}

int
mp_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct cpu_attach_args *caa = (struct cpu_attach_args*) aux;
	if (strcmp(caa->caa_name, cf->cf_driver->cd_name)) {
		return 0;
	}

	return (config_match(parent, cf, aux));
}

/*
 * Map a chunk of memory read-only and return an appropraitely
 * const'ed pointer.
 */

static const void *
mpbios_map(pa, len, handle)
	u_long pa;
	int len;
	struct mp_map *handle;
{
	u_long startpa = i386_trunc_page(pa);
	u_long endpa = i386_round_page(pa + len);
	vm_offset_t va = kmem_alloc(kernel_map, endpa - startpa);
	vm_offset_t retva = va + (pa & PGOFSET);

	handle->pa = pa;
	handle->sva = startpa;
	handle->eva = endpa;
	handle->psize = len;
	handle->baseva = va;
	handle->vsize = endpa - startpa;

	do {
		pmap_enter(kernel_pmap, va, startpa, VM_PROT_READ, FALSE);
		va += PAGE_SIZE;
		startpa += PAGE_SIZE;
	} while (startpa < endpa);

	return (const void*) retva;
}

inline static void
mpbios_unmap(handle)
	struct mp_map *handle;
{
	pmap_remove(kernel_pmap, handle->sva, handle->eva);
	kmem_free(kernel_map, handle->baseva, handle->vsize);
}

/*
 * Look for an Intel MP spec table, indicating SMP capable hardware.
 */
int
mpbios_probe(self)
	struct device *self;
{
	u_long ebda, memtop;

	u_long cthpa;
	int cthlen;
	const u_int8_t *mpbios_page;
	int scan_loc;

	struct mp_map t;

	/* see if EBDA exists */

	mpbios_page = mpbios_map(0, PAGE_SIZE, &t);

	ebda = *(const u_int16_t*) (&mpbios_page[0x40e]);
	ebda <<= 4;

	memtop = *(const u_int16_t*) (&mpbios_page[0x413]);
	memtop <<= 10;

	mpbios_page = NULL;
	mpbios_unmap(&t);

	scan_loc = 0;

	if (ebda && ebda < IOM_BEGIN) {
		mp_fps = mpbios_search(self, ebda, 1024, &mp_fp_map);
		if (mp_fps != NULL)
			goto found;
	}

	scan_loc = 1;

	if (memtop && memtop <= IOM_BEGIN) {
		mp_fps = mpbios_search(self, memtop - 1024, 1024, &mp_fp_map);
		if (mp_fps != NULL)
			goto found;
	}

	scan_loc = 2;

	mp_fps = mpbios_search(self, BIOS_BASE, BIOS_COUNT, &mp_fp_map);
	if (mp_fps != NULL)
		goto found;

	/* nothing found */
	return 0;

 found:
	if (mp_verbose)
		printf("%s: MP floating pointer found in %s at 0x%lx\n", self->dv_xname,
				loc_where[scan_loc], mp_fp_map.pa);

	if (mp_fps->pap == 0) {
		if (mp_fps->mpfb1 == 0) {
			printf("%s: MP fps invalid: "
					"no default config and no configuration table\n",
					self->dv_xname);

			goto err;
		}
		printf("%s: MP default configuration %d\n", self->dv_xname,
				mp_fps->mpfb1);
		return 10;
	}

	cthpa = mp_fps->pap;

	mp_cth = mpbios_map(cthpa, sizeof(*mp_cth), &mp_cfg_table_map);
	cthlen = mp_cth->base_len;
	mpbios_unmap(&mp_cfg_table_map);

	mp_cth = mpbios_map(cthpa, cthlen, &mp_cfg_table_map);

	if (mp_verbose)
		printf("%s: MP config table at 0x%lx, %d bytes long\n", self->dv_xname,
				cthpa, cthlen);

	if (mp_cth->signature != MP_CT_SIG) {
		printf("%s: MP signature mismatch (%x vs %x)\n", self->dv_xname,
		MP_CT_SIG, mp_cth->signature);
		goto err;
	}

	if (mpbios_cksum(mp_cth, cthlen)) {
		printf("%s: MP Configuration Table checksum mismatch\n",
				self->dv_xname);
		goto err;
	}
	return 10;
 err:
	if (mp_fps) {
		mp_fps = NULL;
		mpbios_unmap(&mp_fp_map);
	}
	if (mp_cth) {
		mp_cth = NULL;
		mpbios_unmap(&mp_cfg_table_map);
	}
	return 0;
}

/*
 * Simple byte checksum used on config tables.
 */

inline static int
mpbios_cksum(start, len)
	const void *start;
	int len;
{
	unsigned char res = 0;
	const char *p = start;
	const char *end = p + len;

	while (p < end)
		res += *p++;

	return res;
}

/*
 * Look for the MP floating pointer signature in the given physical
 * address range.
 *
 * We map the memory, scan through it, and unmap it.
 * If we find it, remap the floating pointer structure and return it.
 */

const void *
mpbios_search(self, start, count, map)
	struct device *self;
	u_long start;
	int count;
	struct mp_map *map;
{
	struct mp_map t;

	int i, len;
	const struct mpbios_fps *m;
	int end = count - sizeof(*m);
	const u_int8_t *base = mpbios_map(start, count, &t);

	if (mp_verbose)
		printf("%s: scanning 0x%lx to 0x%lx for MP signature\n", self->dv_xname,
				start, start + count - sizeof(*m));

	for (i = 0; i <= end; i += 4) {
		m = (struct mpbios_fps*) &base[i];

		if ((m->signature == MP_FP_SIG) && ((len = m->length << 4) != 0)
				&& mpbios_cksum(m, (m->length << 4)) == 0) {

			mpbios_unmap(&t);

			return mpbios_map(start + i, len, map);
		}
	}
	mpbios_unmap(&t);

	return 0;
}

/*
 * MP configuration table parsing.
 */

static struct mpbios_baseentry mp_conf[] = {
		{0, 20, 0, "cpu"},
		{1, 8, 0, "bus"},
		{2, 8, 0, "ioapic"},
		{3, 8, 0, "ioint"},
		{4, 8, 0, "lint"},
};

static struct mp_bus extint_bus = {
		"ExtINT",
		-1,
		mp_print_special_intr,
		mp_cfg_special_intr,
		0
};
static struct mp_bus smi_bus = {
		"SMI",
		-1,
		mp_print_special_intr,
		mp_cfg_special_intr,
		0
};
static struct mp_bus nmi_bus = {
		"NMI",
		-1,
		mp_print_special_intr,
		mp_cfg_special_intr,
		0
};

/*
 * 1st pass on BIOS's Intel MP specification table.
 *
 * initializes:
 *	mp_ncpus = 1
 *
 * determines:
 *	cpu_apic_address (common to all CPUs)
 *	ioapic_address[N]
 *	mp_naps
 *	mp_nbusses
 *	mp_napics
 *	nintrs
 */
void
mpbios_scan(self)
	struct device *self;
{
	const u_int8_t *position, *end;
	int count;
	int type;
	int intr_cnt, cur_intr;
	u_long lapic_base;
	const struct mpbios_int *iep;
	struct mpbios_int ie;
	struct ioapic_softc *sc;

	printf("%s: Intel MP Specification ", self->dv_xname);

	switch (mp_fps->spec_rev) {
	case 1:
		printf("(Version 1.1)");
		break;
	case 4:
		printf("(Version 1.4)");
		break;
	default:
		printf("(unrecognized rev %d)", mp_fps->spec_rev);
	}

	/*
	 * looks like we've got a MP system.  start setting up
	 * infrastructure..
	 * XXX is this the right place??
	 */

#ifdef MPACPI
	if (mpacpi_ncpu == 0) {
#endif
		lapic_base = LAPIC_BASE;
		if (mp_cth != NULL)
			lapic_base = (u_long) mp_cth->apic_address;

		lapic_boot_init(lapic_base);
#ifdef MPACPI
	}
#endif

	/* check for use of 'default' configuration */
	if (mp_fps->mpfb1 != 0) {

		printf("\n%s: MP default configuration %d\n", self->dv_xname,
				mp_fps->mpfb1);

#ifdef MPACPI
		if (mpacpi_ncpu == 0)
#endif
			mpbios_cpus(self);

#ifdef MPACPI
		if (mpacpi_nioapic == 0)
#endif
		mpbios_ioapic((u_int8_t*) &default_ioapic, self);

		/* XXX */
		printf("%s: WARNING: interrupts not configured\n", self->dv_xname);
		panic("lazy bum");
		return;
	} else {
		/*
		 * should not happen; mp_probe returns 0 in this case,
		 * but..
		 */
		if (mp_cth == NULL)
			panic("mpbios_scan: no config (can't happen?)");

		printf(" (%8.8s %12.12s)\n", mp_cth->oem_id, mp_cth->product_id);

		/*
		 * Walk the table once, counting items
		 */
		position = (const u_int8_t*) (mp_cth);
		end = position + mp_cth->base_len;
		position += sizeof(*mp_cth);

		count = mp_cth->entry_count;
		intr_cnt = 0;

		while ((count--) && (position < end)) {
			type = *position;
			if (type >= MPS_MCT_NTYPES) {
				printf("%s: unknown entry type %x"
						" in MP config table\n", self->dv_xname, type);
				break;
			}
			mp_conf[type].count++;
			if (type == MPS_MCT_BUS) {
				const struct mpbios_bus *bp = (const struct mpbios_bus*) position;
				if (bp->bus_id >= mp_nbus)
					mp_nbus = bp->bus_id + 1;
			}
			/*
			 * Count actual interrupt instances.
			 * dst_apic_id of MPS_ALL_APICS means "wired to all
			 * apics of this type".
			 */
			if (type == MPS_MCT_IOINT) {
				iep = (const struct mpbios_int*) position;
				if (iep->dst_apic_id == MPS_ALL_APICS)
					intr_cnt += mp_conf[MPS_MCT_IOAPIC].count;
				else
					intr_cnt++;
			} else if (type == MPS_MCT_LINT)
				intr_cnt++;
			position += mp_conf[type].length;
		}

		mp_busses = malloc(sizeof(struct mp_bus) * mp_nbus, M_DEVBUF, M_NOWAIT);
		memset(mp_busses, 0, sizeof(struct mp_bus) * mp_nbus);
		mp_intrs = malloc(sizeof(struct mp_intr_map) * intr_cnt, M_DEVBUF, M_NOWAIT | M_ZERO);
		mp_nintr = intr_cnt;

		/* re-walk the table, recording info of interest */
		position = (const u_int8_t*) mp_cth + sizeof(*mp_cth);
		count = mp_cth->entry_count;
		cur_intr = 0;

		while ((count--) && (position < end)) {
			switch (type = *(u_char*) position) {
			case MPS_MCT_CPU:
#ifdef MPACPI
				/* ACPI has done this for us */
				if (mpacpi_ncpu)
					break;
#endif
				mpbios_cpu(position, self);
				break;
			case MPS_MCT_BUS:
				mpbios_bus(position, self);
				break;
			case MPS_MCT_IOAPIC:
#ifdef MPACPI
				/* ACPI has done this for us */
				if (mpacpi_nioapic)
					break;
#endif
				mpbios_ioapic(position, self);
				break;
			case MPS_MCT_IOINT:
				iep = (const struct mpbios_int*) position;
				ie = *iep;
				if (iep->dst_apic_id == MPS_ALL_APICS) {
					SIMPLEQ_FOREACH(sc, &ioapics, sc_next) {
						ie.dst_apic_id = sc->sc_apicid;
						mpbios_int((char*) &ie, type, &mp_intrs[cur_intr++]);
					}
				} else {
					mpbios_int(position, type, &mp_intrs[cur_intr++]);
				}
				break;
			case MPS_MCT_LINT:
				mpbios_int(position, type, &mp_intrs[cur_intr]);
				cur_intr++;
				break;
			default:
				printf("%s: unknown entry type %x in MP config table\n",
						self->dv_xname, type);
				/* NOTREACHED */
				return;
			}

			position += mp_conf[type].length;
		}
		if (mp_verbose && mp_cth->ext_len)
			printf(
					"%s: MP WARNING: %d bytes of extended entries not examined\n",
					self->dv_xname, mp_cth->ext_len);
	}
	/* Clean up. */
	mp_fps = NULL;
	mpbios_unmap (&mp_fp_map);
	if (mp_cth != NULL) {
		mp_cth = NULL;
		mpbios_unmap (&mp_cfg_table_map);
	}
	mpbios_scanned = 1;
}

static void
mpbios_cpu(ent, self)
	const u_int8_t *ent;
	struct device *self;
{
	const struct mpbios_proc *entry = (const struct mpbios_proc *)ent;
	struct cpu_attach_args caa;

	/* XXX move this into the CPU attachment goo. */
	/* check for usability */
	if (!(entry->cpu_flags & PROCENTRY_FLAG_EN))
		return;

	/* check for BSP flag */
	if (entry->cpu_flags & PROCENTRY_FLAG_BP)
		caa.cpu_role = CPU_ROLE_BP;
	else
		caa.cpu_role = CPU_ROLE_AP;

	caa.caa_name = "cpu";
	caa.cpu_apic_id = entry->apic_id;
	caa.cpu_ops = &cpu_ops;

	config_found_sm(self, &caa, mp_print, mp_match);
}

static void
mpbios_cpus(self)
	struct device *self;
{
	struct mpbios_proc pe;
	/* use default addresses */
	pe.apic_id = lapic_cpu_number();
	pe.cpu_flags = PROCENTRY_FLAG_EN | PROCENTRY_FLAG_BP;
//	pe.cpu_signature = CPUID_TO_MODEL();
	pe.feature_flags = (cpu_feature | cpu_feature2 | amd_feature | amd_feature2);

	mpbios_cpu((u_int8_t*) &pe, self);

	pe.apic_id = 1 - lapic_cpu_number();
	pe.cpu_flags = PROCENTRY_FLAG_EN;

	mpbios_cpu((u_int8_t*) &pe, self);
}

/*
 * The following functions conspire to compute base ioapic redirection
 * table entry for a given interrupt line.
 *
 * Fill in: trigger mode, polarity, and possibly delivery mode.
 */
static void
mp_cfg_special_intr(entry, redir)
	const struct mpbios_int *entry;
	u_int32_t *redir;
{

	/*
	 * All of these require edge triggered, zero vector,
	 * appropriate delivery mode.
	 * see page 13 of the 82093AA datasheet.
	 */
	*redir &= ~IOAPIC_REDLO_DEL_MASK;
	*redir &= ~IOAPIC_REDLO_VECTOR_MASK;
	*redir &= ~IOAPIC_REDLO_LEVEL;

	switch (entry->int_type) {
	case MPS_INTTYPE_NMI:
		*redir |= (IOAPIC_REDLO_DEL_NMI << IOAPIC_REDLO_DEL_SHIFT);
		break;

	case MPS_INTTYPE_SMI:
		*redir |= (IOAPIC_REDLO_DEL_SMI << IOAPIC_REDLO_DEL_SHIFT);
		break;

	case MPS_INTTYPE_ExtINT:
		/*
		 * We are using the ioapic in "native" mode.
		 * This indicates where the 8259 is wired to the ioapic
		 * and/or local apic..
		 */
		*redir |= (IOAPIC_REDLO_DEL_EXTINT << IOAPIC_REDLO_DEL_SHIFT);
		*redir |= (IOAPIC_REDLO_MASK);
		break;
	}
}

/* XXX too much duplicated code here. */

static void
mp_cfg_pci_intr(entry, redir)
	const struct mpbios_int *entry;
	u_int32_t *redir;
{
	int mpspo = entry->int_flags & 0x03; /* XXX magic */
	int mpstrig = (entry->int_flags >> 2) & 0x03; /* XXX magic */

	*redir &= ~IOAPIC_REDLO_DEL_MASK;
	switch (mpspo) {
	case MPS_INTPO_ACTHI:
		*redir &= ~IOAPIC_REDLO_ACTLO;
		break;
	case MPS_INTPO_DEF:
	case MPS_INTPO_ACTLO:
		*redir |= IOAPIC_REDLO_ACTLO;
		break;
	default:
		panic("unknown MPS interrupt polarity %d", mpspo);
	}

	if (entry->int_type != MPS_INTTYPE_INT) {
		mp_cfg_special_intr(entry, redir);
		return;
	}
	*redir |= (IOAPIC_REDLO_DEL_FIXED << IOAPIC_REDLO_DEL_SHIFT);

	switch (mpstrig) {
	case MPS_INTTR_DEF:
	case MPS_INTTR_LEVEL:
		*redir |= IOAPIC_REDLO_LEVEL;
		break;
	case MPS_INTTR_EDGE:
		*redir &= ~IOAPIC_REDLO_LEVEL;
		break;
	default:
		panic("unknown MPS interrupt trigger %d", mpstrig);
	}
}

#ifdef I386_MPBIOS_SUPPORT_EISA
static void
mp_cfg_eisa_intr (entry, redir)
	const struct mpbios_int *entry;
	u_int32_t *redir;
{
	int mpspo = entry->int_flags & 0x03; /* XXX magic */
	int mpstrig = (entry->int_flags >> 2) & 0x03; /* XXX magic */

	*redir &= ~IOAPIC_REDLO_DEL_MASK;
	switch (mpspo) {
	case MPS_INTPO_DEF:
	case MPS_INTPO_ACTHI:
		*redir &= ~IOAPIC_REDLO_ACTLO;
		break;
	case MPS_INTPO_ACTLO:
		*redir |= IOAPIC_REDLO_ACTLO;
		break;
	default:
		panic("unknown MPS interrupt polarity %d", mpspo);
	}

	if (entry->int_type != MPS_INTTYPE_INT) {
		mp_cfg_special_intr(entry, redir);
		return;
	}
	*redir |= (IOAPIC_REDLO_DEL_FIXED << IOAPIC_REDLO_DEL_SHIFT);

	switch (mpstrig) {
	case MPS_INTTR_LEVEL:
		*redir |= IOAPIC_REDLO_LEVEL;
		break;
	case MPS_INTTR_EDGE:
		*redir &= ~IOAPIC_REDLO_LEVEL;
		break;
	case MPS_INTTR_DEF:
		/*
		 * Set "default" setting based on ELCR value snagged
		 * earlier.
		 */
		if (mp_busses[entry->src_bus_id].mb_data & (1 << entry->src_bus_irq)) {
			*redir |= IOAPIC_REDLO_LEVEL;
		} else {
			*redir &= ~IOAPIC_REDLO_LEVEL;
		}
		break;
	default:
		panic("unknown MPS interrupt trigger %d", mpstrig);
	}
}
#endif


static void
mp_cfg_isa_intr (entry, redir)
	const struct mpbios_int *entry;
	u_int32_t *redir;
{
	int mpspo = entry->int_flags & 0x03; /* XXX magic */
	int mpstrig = (entry->int_flags >> 2) & 0x03; /* XXX magic */

	*redir &= ~IOAPIC_REDLO_DEL_MASK;
	switch (mpspo) {
	case MPS_INTPO_DEF:
	case MPS_INTPO_ACTHI:
		*redir &= ~IOAPIC_REDLO_ACTLO;
		break;
	case MPS_INTPO_ACTLO:
		*redir |= IOAPIC_REDLO_ACTLO;
		break;
	default:
		panic("unknown MPS interrupt polarity %d", mpspo);
	}

	if (entry->int_type != MPS_INTTYPE_INT) {
		mp_cfg_special_intr(entry, redir);
		return;
	}
	*redir |= (IOAPIC_REDLO_DEL_FIXED << IOAPIC_REDLO_DEL_SHIFT);

	switch (mpstrig) {
	case MPS_INTTR_LEVEL:
		*redir |= IOAPIC_REDLO_LEVEL;
		break;
	case MPS_INTTR_DEF:
	case MPS_INTTR_EDGE:
		*redir &= ~IOAPIC_REDLO_LEVEL;
		break;
	default:
		panic("unknown MPS interrupt trigger %d", mpstrig);
	}
}


static void
mp_print_special_intr (intr)
	int intr;
{
}

static void
mp_print_pci_intr (intr)
	int intr;
{
	printf(" device %d INT_%c", (intr>>2)&0x1f, 'A' + (intr & 0x3));
}

static void
mp_print_isa_intr (intr)
	int intr;
{
	printf(" irq %d", intr);
}

#ifdef I386_MPBIOS_SUPPORT_EISA
static void
mp_print_eisa_intr (intr)
	int intr;
{
	printf(" EISA irq %d", intr);
}
#endif

#define TAB_UNIT		4
#define TAB_ROUND(a)	_TAB_ROUND(a, TAB_UNIT)

#define _TAB_ROUND(a,u)	(((a) + (u - 1)) & ~(u-1))
#define EXTEND_TAB(a,u)	(!(_TAB_ROUND(a,u) == _TAB_ROUND((a+1),u)))

static void
mpbios_bus(ent, self)
	const u_int8_t *ent;
	struct device *self;
{
	const struct mpbios_bus *entry = (const struct mpbios_bus *)ent;
	int bus_id = entry->bus_id;

	printf("mpbios: bus %d is type %6.6s\n", bus_id, entry->bus_type);

#ifdef DIAGNOSTIC
	/*
	 * This "should not happen" unless the table changes out
	 * from underneath us
	 */
	if (bus_id >= mp_nbus) {
		panic("mpbios: bus number %d out of range?? (type %6.6s)\n",
		    bus_id, entry->bus_type);
	}
#endif

	mp_busses[bus_id].mb_intrs = NULL;

	if (memcmp(entry->bus_type, "PCI   ", 6) == 0) {
		mp_busses[bus_id].mb_name = "pci";
		mp_busses[bus_id].mb_idx = bus_id;
		mp_busses[bus_id].mb_intr_print = mp_print_pci_intr;
		mp_busses[bus_id].mb_intr_cfg = mp_cfg_pci_intr;
#ifdef I386_MPBIOS_SUPPORT_EISA
	} else if (memcmp(entry->bus_type, "EISA  ", 6) == 0) {
		mp_busses[bus_id].mb_name = "eisa";
		mp_busses[bus_id].mb_idx = bus_id;
		mp_busses[bus_id].mb_intr_print = mp_print_eisa_intr;
		mp_busses[bus_id].mb_intr_cfg = mp_cfg_eisa_intr;

		mp_busses[bus_id].mb_data = inb(ELCR0) | (inb(ELCR1) << 8);

		if (mp_eisa_bus != -1)
			printf("oops: multiple isa busses?\n");
		else
			mp_eisa_bus = bus_id;
#endif

	} else if (memcmp(entry->bus_type, "ISA   ", 6) == 0) {
		mp_busses[bus_id].mb_name = "isa";
		mp_busses[bus_id].mb_idx = 0; /* XXX */
		mp_busses[bus_id].mb_intr_print = mp_print_isa_intr;
		mp_busses[bus_id].mb_intr_cfg = mp_cfg_isa_intr;
		if (mp_isa_bus != -1)
			printf("oops: multiple isa busses?\n");
		else
			mp_isa_bus = bus_id;
	} else {
		printf("%s: unsupported bus type %6.6s\n", self->dv_xname,
				entry->bus_type);
	}
}

static void
mpbios_ioapic(ent, self)
	const u_int8_t *ent;
	struct device *self;
{
	const struct mpbios_ioapic *entry = (const struct mpbios_ioapic *)ent;
	struct apic_attach_args aaa;

	/* XXX let flags checking happen in ioapic driver.. */
	if (!(entry->apic_flags & IOAPICENTRY_FLAG_EN))
		return;

	aaa.aaa_name = "ioapic";
	aaa.apic_id = entry->apic_id;
	aaa.apic_version = entry->apic_version;
	aaa.apic_address = (u_long) entry->apic_address;
	aaa.apic_vecbase = -1;
	aaa.flags = (mp_fps->mpfb2 & 0x80) ? IOAPIC_PICMODE : IOAPIC_VWIRE;

	config_found_sm(self, &aaa, mp_print, mp_match);
}

static const char inttype_fmt[] = "\177\020"
		"f\0\2type\0" "=\1NMI\0" "=\2SMI\0" "=\3ExtINT\0";

static const char flagtype_fmt[] = "\177\020"
		"f\0\2pol\0" "=\1Act Hi\0" "=\3Act Lo\0"
		"f\2\2trig\0" "=\1Edge\0" "=\3Level\0";

static void
mpbios_int(ent, enttype, mpi)
	const u_int8_t *ent;
	int enttype;
	struct mp_intr_map *mpi;
{
	const struct mpbios_int *entry = (const struct mpbios_int *)ent;
	struct ioapic_softc *sc = NULL, *sc2;
	struct softpic *spic;

	struct mp_intr_map *altmpi;
	struct mp_bus *mpb;

	u_int32_t id = entry->dst_apic_id;
	u_int32_t pin = entry->dst_apic_int;
	u_int32_t bus = entry->src_bus_id;
	u_int32_t dev = entry->src_bus_irq;
	u_int32_t type = entry->int_type;
	u_int32_t flags = entry->int_flags;

	switch (type) {
	case MPS_INTTYPE_INT:
		mpb = &(mp_busses[bus]);
		break;
	case MPS_INTTYPE_ExtINT:
		mpb = &extint_bus;
		break;
	case MPS_INTTYPE_SMI:
		mpb = &smi_bus;
		break;
	case MPS_INTTYPE_NMI:
		mpb = &nmi_bus;
		break;
	default:
		panic("unknown MPS interrupt type %d", entry->int_type);
	}

	mpi->next = mpb->mb_intrs;
	mpb->mb_intrs = mpi;
	mpi->bus = mpb;
	mpi->bus_pin = dev;
	mpi->global_int = -1;

	mpi->type = type;
	mpi->flags = flags;
	mpi->redir = 0;
	if (mpb->mb_intr_cfg == NULL) {
		printf("mpbios: can't find bus %d for apic %d pin %d\n", bus, id, pin);
		return;
	}

	(*mpb->mb_intr_cfg)(entry, &mpi->redir);

	if (enttype == MPS_MCT_IOINT) {
		sc = ioapic_find(id);
		if (sc == NULL) {
			printf("mpbios: can't find ioapic %d\n", id);
			return;
		}

		/*
		 * XXX workaround for broken BIOSs that put the ACPI
		 * global interrupt number in the entry, not the pin
		 * number.
		 */
		if (pin >= sc->sc_apic_sz) {
			sc2 = ioapic_find_bybase(pin);
			if (sc2 != sc) {
				printf("mpbios: bad pin %d for apic %d\n", pin, id);
				return;
			}
			printf("mpbios: WARNING: pin %d for apic %d too high; "
					"assuming ACPI global int value\n", pin, id);
			pin -= sc->sc_apic_vecbase;
		}

		mpi->ioapic = sc;
		mpi->ioapic_pin = pin;
		spic = sc->sc_softpic;
		altmpi = spic->sp_pins[pin].sp_map;

		if (altmpi != NULL) {
			if ((altmpi->type != type) || (altmpi->flags != flags)) {
				printf("%s: conflicting map entries for pin %d\n",
						sc->sc_dev->dv_xname, pin);
			}
		} else {
			spic->sp_pins[pin].sp_map = mpi;
		}
	} else {
		if (pin >= 2)
			printf("pin %d of local apic doesn't exist!\n", pin);
		else {
			mpi->ioapic = NULL;
			mpi->ioapic_pin = pin;
			mpi->cpu_id = id;
		}
	}

	mpi->ioapic_ih = APIC_INT_VIA_APIC
			| ((id << APIC_INT_APIC_SHIFT) | ((pin << APIC_INT_PIN_SHIFT)));

	if (mp_verbose) {
		char buf[256];

		printf("%s: int%d attached to %s",
				sc ? sc->sc_dev->dv_xname : "local apic", pin, mpb->mb_name);

		if (mpb->mb_idx != -1)
			printf("%d", mpb->mb_idx);

		(*(mpb->mb_intr_print))(dev);

		printf(" (type %s",
				bitmask_snprintf(type, inttype_fmt, buf, sizeof(buf)));

		printf(" flags %s)\n",
				bitmask_snprintf(flags, flagtype_fmt, buf, sizeof(buf)));
	}
}

#if NPCI > 0
int
mpbios_pci_attach_hook(parent, self, pba)
	struct device *parent, *self;
	struct pcibus_attach_args *pba;
{
	struct mp_bus *mpb;

	if (mpbios_scanned == 0)
		return ENOENT;

	if (pba->pba_bus >= mp_nbus) {
		intr_add_pcibus(pba);
		return 0;
	}

	mpb = &mp_busses[pba->pba_bus];
	if (mpb->mb_name != NULL) {
		if (strcmp(mpb->mb_name, "pci"))
			return EINVAL;
	} else
		mpb->mb_name = "pci";

	mpb->mb_configured = 1;
	mpb->mb_pci_bridge_tag = pba->pba_bridgetag;
	mpb->mb_pci_chipset_tag = pba->pba_pc;
	return 0;
}

int
mpbios_scan_pci(self, pba, print)
	struct device *self;
	struct pcibus_attach_args *pba;
	cfprint_t print;
{
	int i;
	struct mp_bus *mpb;
	struct pci_attach_args;

	for (i = 0; i < mp_nbus; i++) {
		mpb = &mp_busses[i];
		if (mpb->mb_name == NULL)
			continue;
		if (!strcmp(mpb->mb_name, "pci") && mpb->mb_configured == 0) {
			pba->pba_bus = i;
			config_found(self, pba, print);
		}
	}
	return 0;
}
#endif
