/*	$NetBSD: i82365var.h,v 1.2 1997/10/16 23:18:22 thorpej Exp $	*/

/*
 * Copyright (c) 1997 Marc Horowitz.  All rights reserved.
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
 *	This product includes software developed by Marc Horowitz.
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

#include <sys/device.h>

#include <dev/core/ic/i82365reg.h>
#include <dev/core/pcmcia/pcmciachip.h>
#include <dev/core/pcmcia/pcmciareg.h>

struct pcic_handle {
	struct device 		*ph_parent;
	bus_space_tag_t 	ph_bus_t;	/* I/O or MEM?  I don't mind */
	bus_space_handle_t 	ph_bus_h;
	uint8_t 			(*ph_read)(struct pcic_handle *, int);
	void 				(*ph_write)(struct pcic_handle *, int, uint8_t);
	struct pcic_softc 	*sc;
	int					vendor;		/* vendor of chip */
	int					chip;		/* chip index 0 or 1 */
	int					sock;
	int					flags;
	int					laststate;
	int					memalloc;
	struct {
		bus_addr_t		addr;
		bus_size_t		size;
		long			offset;
		int				kind;
	} mem[PCIC_MEM_WINS];
	int					ioalloc;
	struct {
		bus_addr_t		addr;
		bus_size_t		size;
		int				width;
	} io[PCIC_IO_WINS];
	int					ih_irq;
	struct device 		*pcmcia;
};

#define	PCIC_FLAG_SOCKETP		0x0001
#define	PCIC_FLAG_CARDP			0x0002
#define	PCIC_FLAG_ENABLED		0x0004

#define PCIC_LASTSTATE_PRESENT	0x0002
#define PCIC_LASTSTATE_HALF		0x0001
#define PCIC_LASTSTATE_EMPTY	0x0000

#define	C0SA	0
#define	C0SB	PCIC_SOCKET_OFFSET
#define	C1SA	PCIC_CHIP_OFFSET
#define	C1SB	PCIC_CHIP_OFFSET + PCIC_SOCKET_OFFSET

#define	PCIC_VENDOR_NONE			-1
#define	PCIC_VENDOR_UNKNOWN			0
#define	PCIC_VENDOR_I82365SLR0		1
#define	PCIC_VENDOR_I82365SLR1		2
#define	PCIC_VENDOR_CIRRUS_PD67XX	3
#define PCIC_VENDOR_I82365SL_DF		4
#define PCIC_VENDOR_IBM				5
#define PCIC_VENDOR_IBM_KING		6
#define PCIC_VENDOR_RICOH_5C296		7
#define PCIC_VENDOR_RICOH_5C396		8

/*
 * This is sort of arbitrary.  It merely needs to be "enough". It can be
 * overridden in the conf file, anyway.
 */

#define	PCIC_MEM_PAGES	4
#define	PCIC_MEMSIZE	PCIC_MEM_PAGES*PCIC_MEM_PAGESIZE

#define	PCIC_NSLOTS		4

struct pcic_softc {
	struct device 		dev;

	bus_space_tag_t 	memt;
	bus_space_handle_t 	memh;
	bus_space_tag_t 	iot;
	bus_space_handle_t 	ioh;

	/* XXX isa_chipset_tag_t, pci_chipset_tag_t, etc. */
	caddr_t				intr_est;

	pcmcia_chipset_tag_t pct;

	/* this needs to be large enough to hold PCIC_MEM_PAGES bits */
	int					subregionmask;

	/* used by memory window mapping functions */
	bus_addr_t 			membase;

	/*
	 * used by io window mapping functions.  These can actually overlap
	 * with another pcic, since the underlying extent mapper will deal
	 * with individual allocations.  This is here to deal with the fact
	 * that different busses have different real widths (different pc
	 * hardware seems to use 10 or 12 bits for the I/O bus).
	 */
	bus_addr_t 			iobase;
	bus_addr_t 			iosize;

	int					irq;
	void				*ih;

	struct pcic_handle 	handle[PCIC_NSLOTS];
	/* for use by underlying chip code for discovering irqs */
	int intr_detect, intr_false;
	int intr_mask[PCIC_NSLOTS / 2];	/* probed intterupts if possible */
};


int		pcic_ident_ok (int);
int		pcic_vendor (struct pcic_handle *);
char	*pcic_vendor_to_string (int);

void	pcic_attach (struct pcic_softc *);
void	pcic_attach_sockets (struct pcic_softc *);
int		pcic_intr (void *arg);

static inline int pcic_read (struct pcic_handle *, int);
static inline void pcic_write (struct pcic_handle *, int, int);
static inline void pcic_wait_ready (struct pcic_handle *);

int		pcic_chip_mem_alloc (pcmcia_chipset_handle_t, bus_size_t, struct pcmcia_mem_handle *);
void	pcic_chip_mem_free (pcmcia_chipset_handle_t, struct pcmcia_mem_handle *);
int		pcic_chip_mem_map (pcmcia_chipset_handle_t, int, bus_addr_t, bus_size_t, struct pcmcia_mem_handle *, bus_addr_t *, int *);
void	pcic_chip_mem_unmap (pcmcia_chipset_handle_t, int);

int		pcic_chip_io_alloc (pcmcia_chipset_handle_t, bus_addr_t, bus_size_t, bus_size_t, struct pcmcia_io_handle *);
void	pcic_chip_io_free (pcmcia_chipset_handle_t, struct pcmcia_io_handle *);
int		pcic_chip_io_map (pcmcia_chipset_handle_t, int, bus_addr_t, bus_size_t, struct pcmcia_io_handle *, int *);
void	pcic_chip_io_unmap (pcmcia_chipset_handle_t, int);

void	pcic_chip_socket_enable (pcmcia_chipset_handle_t);
void	pcic_chip_socket_disable (pcmcia_chipset_handle_t);

static __inline int pcic_read (struct pcic_handle *, int);
static __inline int
pcic_read(h, idx)
	struct pcic_handle *h;
	int idx;
{
	if (idx != -1)
		bus_space_write_1(h->sc->iot, h->sc->ioh, PCIC_REG_INDEX, h->sock + idx);
	return (bus_space_read_1(h->sc->iot, h->sc->ioh, PCIC_REG_DATA));
}

static __inline void pcic_write (struct pcic_handle *, int, int);
static __inline void
pcic_write(h, idx, data)
	struct pcic_handle *h;
	int idx;
	int data;
{
	if (idx != -1)
		bus_space_write_1(h->sc->iot, h->sc->ioh, PCIC_REG_INDEX, h->sock + idx);
	bus_space_write_1(h->sc->iot, h->sc->ioh, PCIC_REG_DATA, (data));
}

static __inline void pcic_wait_ready (struct pcic_handle *);
static __inline void
pcic_wait_ready(h)
	struct pcic_handle *h;
{
	int i;

	for (i = 0; i < 10000; i++) {
		if (pcic_read(h, PCIC_IF_STATUS) & PCIC_IF_STATUS_READY)
			return;
		delay(500);
	}

#ifdef DIAGNOSTIC
	printf("pcic_wait_ready ready never happened\n");
#endif
}
