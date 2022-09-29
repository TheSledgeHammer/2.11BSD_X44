/*
 * pci_pir1.c
 *
 *  Created on: 29 Sept 2022
 *      Author: marti
 */

#include <sys/null.h>
#include <sys/queue.h>

#include <devel/arch/i386/include/pcibios.h>

SIMPLEQ_HEAD(pciintr_link_map_head, pciintr_link_map);
struct pciintr_link_map {
	int 							link;
	int 							clink;
	uint8_t							irq;
	uint16_t						bitmap;
	int 							fixup_stage;/* routed */
	int 							routed;
	int								references;
	SIMPLEQ_ENTRY(pciintr_link_map) list;
};

struct pci_link_lookup {
	struct pciintr_link_map **pciintr_link_ptr;
	int 					bus;
	int 					device;
	int 					pin;
};

typedef void pir_entry_handler(struct PIR_entry *, struct PIR_intpin *, void *);

struct pciintr_link_map *pci_pir_find_link(uint8_t);
struct pciintr_link_map *pci_pir_link_alloc(struct PIR_entry *, struct PIR_intpin *, int);
void pci_pir_walk_table(pir_entry_handler *handler, void *arg);
void pci_pir_create_links(struct PIR_entry *, struct PIR_intpin *, void *);
void pci_pir_inital_irqs(struct PIR_entry *, struct PIR_intpin *, void *);
u_int16_t pci_pir_compat_router(struct PIR_header *, pcireg_t);

/*
 * Find the pci_link structure for a given link ID.
 */
struct pciintr_link_map *
pci_pir_find_link(link_id)
	uint8_t link_id;
{
	struct pciintr_link_map *pci_link;

	SIMPLEQ_FOREACH(pci_link, &pciintr_link_map_list, list) {
		if (pci_link->link == link_id) {
			return (pci_link);
		}
	}
	return (NULL);
}

static void
pci_pir_find_link_handler(struct PIR_entry *entry, struct PIR_intpin *intpin, void *arg)
{
	struct pci_link_lookup *lookup;

	lookup = (struct pci_link_lookup *)arg;
	if (entry->pe_bus == lookup->bus && entry->pe_device == lookup->device && intpin - entry->pe_intpin == lookup->pin) {
		*lookup->pciintr_link_ptr = pci_pir_find_link(intpin->link);
	}
}

void
pci_pir_walk_table(pir_entry_handler *handler, void *arg)
{
	struct PIR_entry 	*entry;
	struct PIR_intpin 	*intpin;
	int i, pin;

	for (i = 0; i < pci_route_count; i++) {
		entry = &pci_route_table->pt_entry[i];
		for (pin = 0; pin < PCI_INTERRUPT_PIN_MAX; pin++) {
			intpin = &entry->pe_intpin[pin];
			if (intpin->link != 0) {
				handler(entry, intpin, arg);
			}
		}
	}
}

struct pciintr_link_map *
pci_pir_link_alloc(struct PIR_entry *entry, struct PIR_intpin *intpin, int pin)
{
	struct pciintr_link_map *l, *lstart;
	int link, clink, irq;

	intpin = entry->pe_intpin[pin];
	link = intpin->link;
	if (pciintr_icu_tag != NULL) {
		if (pciintr_icu_getclink(pciintr_icu_tag, pciintr_icu_handle, link, &clink) != 0) {
#ifdef DIAGNOSTIC
			printf("pciintr_link_alloc: bus %d device %d: "
					"link 0x%02x invalid\n", entry->pe_bus,
					PIR_DEVFUNC_DEVICE(entry->pe_device), link);
#endif
			return (NULL);
		}

		if (pciintr_icu_get_intr(pciintr_icu_tag, pciintr_icu_handle, clink, &irq) != 0) {
#ifdef DIAGNOSTIC
			printf("pciintr_link_alloc: "
				"bus %d device %d link 0x%02x: "
				"PIRQ 0x%02x invalid\n", entry->pe_bus,
				PIR_DEVFUNC_DEVICE(entry->pe_device), link, clink);
#endif
			return (NULL);
		}
	}
	l = malloc(sizeof(*l), M_DEVBUF, M_NOWAIT);
	if (l == NULL) {
		panic("pciintr_link_alloc");
	}
	memset(l, 0, sizeof(*l));

	l->link = link;
	l->bitmap = intpin->irqs;
	if (pciintr_icu_tag != NULL) { 	 /* compatible PCI ICU found */
		l->clink = clink;
		l->irq = irq; 				/* maybe I386_PCI_INTERRUPT_LINE_NO_CONNECTION */
	} else {
		l->clink = link; 			/* only for PCIBIOSVERBOSE diagnostic */
		l->irq = I386_PCI_INTERRUPT_LINE_NO_CONNECTION;
	}
	l->references = 1;
	l->routed = 0;
	lstart = SIMPLEQ_FIRST(&pciintr_link_map_list);
	if (lstart == NULL || lstart->link < l->link) {
		SIMPLEQ_INSERT_TAIL(&pciintr_link_map_list, l, list);
	} else {
		SIMPLEQ_INSERT_HEAD(&pciintr_link_map_list, l, list);
	}
	return (l);
}

void
pci_pir_create_links(struct PIR_entry *entry, struct PIR_intpin *intpin, void *arg)
{
	struct pciintr_link_map *pci_link;

	pci_link = pci_pir_find_link(intpin->link);
	if (pci_link != NULL) {
		pci_link->references++;
		if (intpin->irqs != pci_link->bitmap) {
			if (bootverbose) {
				printf("$PIR: Entry %d.%d.INT%c has different mask for link %#x, merging\n",
						entry->pe_bus, entry->pe_device,
						(intpin - entry->pe_intpin) + 'A', pci_link->link);
			}
			pci_link->bitmap &= intpin->irqs;
		}
	} else {
		pci_link = pci_pir_link_alloc(entry, intpin, PCI_INVALID_IRQ);
	}
}

void
pci_pir_inital_irqs(struct PIR_entry *entry, struct PIR_intpin *intpin, void *arg)
{
	struct pciintr_link_map *pci_link;
	uint8_t pin;

	pin = intpin - entry->pe_intpin;
	if(intpin->link == 0) {
		/* No connection for this pin. */
		continue;
	}
	/*
	 * Multiple devices may be wired to the same
	 * interrupt; check to see if we've seen this
	 * one already.  If not, allocate a new link
	 * map entry and stuff it in the map.
	 */
	pci_link = pci_pir_find_link(intpin->link);
	if (pci_link == NULL) {
		(void) pci_pir_link_alloc(entry, intpin, pin);
	} else if (entry->pe_intpin[pin].irqs != pci_link->bitmap) {
		/*
		 * violates PCI IRQ Routing Table Specification
		 */
#ifdef DIAGNOSTIC
		printf("pciintr_link_init: "
				"bus %d device %d link 0x%02x: "
				"bad irq bitmap 0x%04x, "
				"should be 0x%04x\n", entry->pe_bus,
				PIR_DEVFUNC_DEVICE(entry->pe_device), intpin->link,
				entry->pe_intpin[pin].irqs, pci_link->bitmap);
#endif
		/* safer value. */
		pci_link->bitmap &= entry->pe_intpin[pin].irqs;
		/* XXX - or, should ignore this entry? */
	}
}

struct pciintr_icu_table {
	pci_vendor_id_t		piit_vendor;
	pci_product_id_t 	piit_product;
	struct piitops		*piit_op;
	SIMPLEQ_ENTRY(pciintr_icu_table) piit_list;
};
SIMPLEQ_HEAD(piit_head, pciintr_icu_table);
struct piit_head piit_header;

struct piitops {
	int (*piit_init)(pci_chipset_tag_t, bus_space_tag_t, pcitag_t, pciintr_icu_tag_t *, pciintr_icu_handle_t *);
};

struct pciintr_icu_table *
piit_icu_lookup(vendor, product)
	pci_vendor_id_t 		vendor;
	pci_product_id_t 		product;
{
	struct pciintr_icu_table *piit;
	SIMPLEQ_FOREACH(piit, &piit_header, piit_list) {
		if (piit->piit_vendor == vendor && piit->piit_product == product) {
			return (piit);
		}
	}
	return (NULL);
}

void
piit_icu_insert(piit, vendor, product)
	struct pciintr_icu_table *piit;
	pci_vendor_id_t 		vendor;
	pci_product_id_t 		product;
{
	piit->piit_vendor = vendor;
	piit->piit_product = product;
	SIMPLEQ_INSERT_HEAD(&piit_header, piit, piit_list);
}

void
piit_icu_remove(vendor, product)
	pci_vendor_id_t 		vendor;
	pci_product_id_t 		product;
{
	struct pciintr_icu_table *piit;

	piit = piit_icu_lookup(vendor, product);
	if(piit != NULL) {
		SIMPLEQ_REMOVE(&piit_header, piit, pciintr_icu_table, piit_list);
	}
}

int
piit_init()
{
	struct pciintr_icu_table *piit;

	return (0);
}
