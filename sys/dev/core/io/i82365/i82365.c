/*	$NetBSD: i82365.c,v 1.77 2003/12/28 01:21:37 christos Exp $	*/

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

#include <sys/cdefs.h>

#define	PCICDEBUG

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/kthread.h>
#include <sys/power.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/core/pcmcia/pcmciareg.h>
#include <dev/core/pcmcia/pcmciavar.h>

#include <dev/core/io/i82365/i82365reg.h>
#include <dev/core/io/i82365/i82365var.h>

#include "locators.h"

#ifdef PCICDEBUG
int	pcic_debug = 0;
#define	DPRINTF(arg) if (pcic_debug) printf arg;
#else
#define	DPRINTF(arg)
#endif

CFDRIVER_DECL(NULL, pcic, DV_DULL);

/*
 * Individual drivers will allocate their own memory and io regions. Memory
 * regions must be a multiple of 4k, aligned on a 4k boundary.
 */

#define	PCIC_MEM_ALIGN	PCIC_MEM_PAGESIZE

void	pcic_attach_socket(struct pcic_handle *);
void	pcic_attach_socket_finish(struct pcic_handle *);

int		pcic_submatch(struct device *, struct cfdata *, void *);
int		pcic_print(void *arg, const char *pnp);
int		pcic_intr_socket(struct pcic_handle *);
void	pcic_poll_intr(void *);

void	pcic_attach_card(struct pcic_handle *);
void	pcic_detach_card(struct pcic_handle *, int);
void	pcic_deactivate_card(struct pcic_handle *);

void	pcic_chip_do_mem_map(struct pcic_handle *, int);
void	pcic_chip_do_io_map(struct pcic_handle *, int);

void	pcic_create_event_thread(void *);
void	pcic_event_thread(void *);

void	pcic_queue_event(struct pcic_handle *, int);
void	pcic_power(int, void *);

static void	pcic_wait_ready(struct pcic_handle *);
static void	pcic_delay(struct pcic_handle *, int, char *);

static u_int8_t st_pcic_read(struct pcic_handle *, int);
static void 	st_pcic_write(struct pcic_handle *, int, u_int8_t);

int
pcic_ident_ok(ident)
	int ident;
{
	/* this is very empirical and heuristic */

	if ((ident == 0) || (ident == 0xff) || (ident & PCIC_IDENT_ZERO))
		return (0);

	if ((ident & PCIC_IDENT_IFTYPE_MASK) != PCIC_IDENT_IFTYPE_MEM_AND_IO) {
#ifdef DIAGNOSTIC
		printf("pcic: does not support memory and I/O cards, "
		    "ignored (ident=%0x)\n", ident);
#endif
		return (0);
	}
	return (1);
}

int
pcic_vendor(h)
	struct pcic_handle *h;
{
	int reg;
	int vendor;

	reg = pcic_read(h, PCIC_IDENT);

	if ((reg & PCIC_IDENT_REV_MASK) == 0)
		return (PCIC_VENDOR_NONE);

	switch (reg) {
	case 0x00:
	case 0xff:
		return (PCIC_VENDOR_NONE);
	case PCIC_IDENT_ID_INTEL0:
		vendor = PCIC_VENDOR_I82365SLR0;
		break;
	case PCIC_IDENT_ID_INTEL1:
		vendor = PCIC_VENDOR_I82365SLR1;
		break;
	case PCIC_IDENT_ID_INTEL2:
		vendor = PCIC_VENDOR_I82365SL_DF;
		break;
	case PCIC_IDENT_ID_IBM1:
	case PCIC_IDENT_ID_IBM2:
		vendor = PCIC_VENDOR_IBM;
		break;
	case PCIC_IDENT_ID_IBM3:
		vendor = PCIC_VENDOR_IBM_KING;
		break;
	default:
		vendor = PCIC_VENDOR_UNKNOWN;
		break;
	}

	if (vendor == PCIC_VENDOR_I82365SLR0 ||
	    vendor == PCIC_VENDOR_I82365SLR1) {
		/*
		 * Check for Cirrus PD67xx.
		 * the chip_id of the cirrus toggles between 11 and 00 after a
		 * write.  weird.
		 */
		pcic_write(h, PCIC_CIRRUS_CHIP_INFO, 0);
		reg = pcic_read(h, -1);
		if ((reg & PCIC_CIRRUS_CHIP_INFO_CHIP_ID) ==
		    PCIC_CIRRUS_CHIP_INFO_CHIP_ID) {
			reg = pcic_read(h, -1);
			if ((reg & PCIC_CIRRUS_CHIP_INFO_CHIP_ID) == 0)
				return (PCIC_VENDOR_CIRRUS_PD67XX);
		}

		/*
		 * check for Ricoh RF5C[23]96
		 */
		reg = pcic_read(h, PCIC_RICOH_REG_CHIP_ID);
		switch (reg) {
		case PCIC_RICOH_CHIP_ID_5C296:
			return (PCIC_VENDOR_RICOH_5C296);
		case PCIC_RICOH_CHIP_ID_5C396:
			return (PCIC_VENDOR_RICOH_5C396);
		}
	}

	return (vendor);
}

char *
pcic_vendor_to_string(vendor)
	int vendor;
{
	switch (vendor) {
	case PCIC_VENDOR_I82365SLR0:
		return ("Intel 82365SL Revision 0");
	case PCIC_VENDOR_I82365SLR1:
		return ("Intel 82365SL Revision 1");
	case PCIC_VENDOR_CIRRUS_PD67XX:
		return ("Cirrus PD6710/2X");
	case PCIC_VENDOR_I82365SL_DF:
		return ("Intel 82365SL-DF");
	case PCIC_VENDOR_RICOH_5C296:
		return ("Ricoh RF5C296");
	case PCIC_VENDOR_RICOH_5C396:
		return ("Ricoh RF5C396");
	case PCIC_VENDOR_IBM:
		return ("IBM PCIC");
	case PCIC_VENDOR_IBM_KING:
		return ("IBM KING");
	}

	return ("Unknown controller");
}

void
pcic_attach(sc)
	struct pcic_softc *sc;
{
	int i, reg, chip, socket;
	struct pcic_handle *h;

	DPRINTF(("pcic ident regs:"));

	lockinit(&sc->sc_pcic_lock, PWAIT, "pciclk", 0, 0);

	/* find and configure for the available sockets */
	for (i = 0; i < PCIC_NSLOTS; i++) {
		h = &sc->handle[i];
		chip = i / 2;
		socket = i % 2;

		h->ph_parent = (struct device*) sc;
		h->chip = chip;
		h->sock = chip * PCIC_CHIP_OFFSET + socket * PCIC_SOCKET_OFFSET;
		h->laststate = PCIC_LASTSTATE_EMPTY;
		/* initialize pcic_read and pcic_write functions */
		h->ph_read = st_pcic_read;
		h->ph_write = st_pcic_write;
		h->ph_bus_t = sc->iot;
		h->ph_bus_h = sc->ioh;
		h->flags = 0;

		/* need to read vendor -- for cirrus to report no xtra chip */
		if (socket == 0)
			h->vendor = (h + 1)->vendor = pcic_vendor(h);

		switch (h->vendor) {
		case PCIC_VENDOR_NONE:
			/* no chip */
			continue;
		case PCIC_VENDOR_CIRRUS_PD67XX:
			reg = pcic_read(h, PCIC_CIRRUS_CHIP_INFO);
			if (socket == 0 || (reg & PCIC_CIRRUS_CHIP_INFO_SLOTS))
				h->flags = PCIC_FLAG_SOCKETP;
			break;
		default:
			/*
			 * During the socket probe, read the ident register
			 * twice.  I don't understand why, but sometimes the
			 * clone chips in hpcmips boxes read all-0s the first
			 * time. -- mycroft
			 */
			reg = pcic_read(h, PCIC_IDENT);
			DPRINTF(("socket %d ident reg 0x%02x\n", i, reg))
			;
			reg = pcic_read(h, PCIC_IDENT);
			DPRINTF(("socket %d ident reg 0x%02x\n", i, reg))
			;
			if (pcic_ident_ok(reg))
				h->flags = PCIC_FLAG_SOCKETP;
			break;
		}
	}
	for (i = 0; i < PCIC_NSLOTS; i++) {
		h = &sc->handle[i];

		if (h->flags & PCIC_FLAG_SOCKETP) {
			SIMPLEQ_INIT(&h->events);

			/* disable interrupts and leave socket in reset */
			pcic_write(h, PCIC_CSC_INTR, 0);
			pcic_write(h, PCIC_INTR, 0);
			(void) pcic_read(h, PCIC_CSC);
		}
	}

	/* print detected info */
	for (i = 0; i < PCIC_NSLOTS; i += 2) {
		h = &sc->handle[i];
		chip = i / 2;

		if (h->vendor == PCIC_VENDOR_NONE)
			continue;

		printf("%s: controller %d (%s) has ", sc->dev.dv_xname, chip,
				pcic_vendor_to_string(sc->handle[i].vendor));

		if ((h->flags & PCIC_FLAG_SOCKETP)
				&& ((h + 1)->flags & PCIC_FLAG_SOCKETP))
			printf("sockets A and B\n");
		else if (h->flags & PCIC_FLAG_SOCKETP)
			printf("socket A only\n");
		else if ((h + 1)->flags & PCIC_FLAG_SOCKETP)
			printf("socket B only\n");
		else
			printf("no sockets\n");
	}
}

/*
 * attach the sockets before we know what interrupts we have
 */
void
pcic_attach_sockets(sc)
	struct pcic_softc *sc;
{
	int i;

	for (i = 0; i < PCIC_NSLOTS; i++)
		if (sc->handle[i].flags & PCIC_FLAG_SOCKETP)
			pcic_attach_socket(&sc->handle[i]);
}

void
pcic_power(why, arg)
	int why;
	void *arg;
{
	struct pcic_handle *h = (struct pcic_handle *)arg;
	struct pcic_softc *sc = (struct pcic_softc *)h->ph_parent;
	int reg;

	DPRINTF(("%s: power: why %d\n", h->ph_parent->dv_xname, why));

	if (h->flags & PCIC_FLAG_SOCKETP) {
		if ((why == PWR_RESUME) && (pcic_read(h, PCIC_CSC_INTR) == 0)) {
#ifdef PCICDEBUG
			char bitbuf[64];
#endif
			reg = PCIC_CSC_INTR_CD_ENABLE;
			if (sc->irq != -1)
				reg |= sc->irq << PCIC_CSC_INTR_IRQ_SHIFT;
			pcic_write(h, PCIC_CSC_INTR, reg);
			DPRINTF(
					("%s: CSC_INTR was zero; reset to %s\n", sc->dev.dv_xname, bitmask_snprintf(pcic_read(h, PCIC_CSC_INTR), PCIC_CSC_INTR_FORMAT, bitbuf, sizeof(bitbuf))));
		}

		/*
		 * check for card insertion or removal during suspend period.
		 * XXX: the code can't cope with card swap (remove then insert).
		 * how can we detect such situation?
		 */
		if (why == PWR_RESUME)
			(void) pcic_intr_socket(h);
	}
}

/*
 * attach a socket -- we don't know about irqs yet
 */
void
pcic_attach_socket(h)
	struct pcic_handle *h;
{
	struct pcmciabus_attach_args paa;
	struct pcic_softc *sc = (struct pcic_softc *)h->ph_parent;

	/* initialize the rest of the handle */

	h->shutdown = 0;
	h->memalloc = 0;
	h->ioalloc = 0;
	h->ih_irq = 0;

	/* now, config one pcmcia device per socket */

	paa.paa_busname = "pcmcia";
	paa.pct = (pcmcia_chipset_tag_t) sc->pct;
	paa.pch = (pcmcia_chipset_handle_t) h;
	paa.iobase = sc->iobase;
	paa.iosize = sc->iosize;

	h->pcmcia = config_found_sm(&sc->dev, &paa, pcic_print, pcic_submatch);
	if (h->pcmcia == NULL) {
		h->flags &= ~PCIC_FLAG_SOCKETP;
		return;
	}

	/*
	 * queue creation of a kernel thread to handle insert/removal events.
	 */
#ifdef DIAGNOSTIC
	if (h->event_thread != NULL)
		panic("pcic_attach_socket: event thread");
#endif
	config_pending_incr();
	kthread_create_deferred(pcic_create_event_thread, h);
}

/*
 * now finish attaching the sockets, we are ready to allocate
 * interrupts
 */
void
pcic_attach_sockets_finish(sc)
	struct pcic_softc *sc;
{
	int i;

	for (i = 0; i < PCIC_NSLOTS; i++)
		if (sc->handle[i].flags & PCIC_FLAG_SOCKETP)
			pcic_attach_socket_finish(&sc->handle[i]);
}

/*
 * finishing attaching the socket.  Interrupts may now be on
 * if so expects the pcic interrupt to be blocked
 */
void
pcic_attach_socket_finish(h)
	struct pcic_handle *h;
{
	struct pcic_softc *sc = (struct pcic_softc *)h->ph_parent;
	int reg, intr;

	DPRINTF(("%s: attach finish socket %ld\n", h->ph_parent->dv_xname,
	    (long) (h - &sc->handle[0])));

	/*
	 * Set up a powerhook to ensure it continues to interrupt on
	 * card detect even after suspend.
	 * (this works around a bug seen in suspend-to-disk on the
	 * Sony VAIO Z505; on resume, the CSC_INTR state is not preserved).
	 */
	powerhook_establish("pcic_power", pcic_power, h);

	/* enable interrupts on card detect, poll for them if no irq avail */
	reg = PCIC_CSC_INTR_CD_ENABLE;
	if (sc->irq == -1) {
		if (sc->poll_established == 0) {
			callout_init(&sc->poll_ch);
			callout_reset(&sc->poll_ch, hz / 2, pcic_poll_intr, sc);
			sc->poll_established = 1;
		}
	} else
		reg |= sc->irq << PCIC_CSC_INTR_IRQ_SHIFT;
	pcic_write(h, PCIC_CSC_INTR, reg);

	/* steer above mgmt interrupt to configured place */
	intr = pcic_read(h, PCIC_INTR);
	intr &= ~(PCIC_INTR_IRQ_MASK | PCIC_INTR_ENABLE);
	if (sc->irq == 0)
		intr |= PCIC_INTR_ENABLE;
	pcic_write(h, PCIC_INTR, intr);

	/* power down the socket */
	pcic_write(h, PCIC_PWRCTL, 0);

	/* zero out the address windows */
	pcic_write(h, PCIC_ADDRWIN_ENABLE, 0);

	/* clear possible card detect interrupt */
	pcic_read(h, PCIC_CSC);

	DPRINTF(
			("%s: attach finish vendor 0x%02x\n", h->ph_parent->dv_xname, h->vendor));

	/* unsleep the cirrus controller */
	if (h->vendor == PCIC_VENDOR_CIRRUS_PD67XX) {
		reg = pcic_read(h, PCIC_CIRRUS_MISC_CTL_2);
		if (reg & PCIC_CIRRUS_MISC_CTL_2_SUSPEND) {
			DPRINTF(
					("%s: socket %02x was suspended\n", h->ph_parent->dv_xname, h->sock));
			reg &= ~PCIC_CIRRUS_MISC_CTL_2_SUSPEND;
			pcic_write(h, PCIC_CIRRUS_MISC_CTL_2, reg);
		}
	}

	/* if there's a card there, then attach it. */
	reg = pcic_read(h, PCIC_IF_STATUS);
	if ((reg & PCIC_IF_STATUS_CARDDETECT_MASK) ==
	PCIC_IF_STATUS_CARDDETECT_PRESENT) {
		pcic_queue_event(h, PCIC_EVENT_INSERTION);
		h->laststate = PCIC_LASTSTATE_PRESENT;
	} else {
		h->laststate = PCIC_LASTSTATE_EMPTY;
	}
}

void
pcic_create_event_thread(arg)
	void *arg;
{
	struct pcic_handle *h = arg;
	const char *cs;

	switch (h->sock) {
	case C0SA:
		cs = "0,0";
		break;
	case C0SB:
		cs = "0,1";
		break;
	case C1SA:
		cs = "1,0";
		break;
	case C1SB:
		cs = "1,1";
		break;
	default:
		panic("pcic_create_event_thread: unknown pcic socket");
	}

	if (kthread_create(pcic_event_thread, h, &h->event_thread, h->ph_parent->dv_xname)) {
		printf("%s: unable to create event thread for sock 0x%02x\n", h->ph_parent->dv_xname, h->sock);
		panic("pcic_create_event_thread");
	}
}

void
pcic_event_thread(arg)
	void *arg;
{
	struct pcic_handle *h = arg;
	struct pcic_event *pe;
	int s, first = 1;
	struct pcic_softc *sc = (struct pcic_softc *)h->ph_parent;

	while (h->shutdown == 0) {
		/*
		 * Serialize event processing on the PCIC.  We may
		 * sleep while we hold this lock.
		 */
		(void) lockmgr(&sc->sc_pcic_lock, LK_EXCLUSIVE, &sc->sc_pcic_lock.lk_lnterlock, LOCKHOLDER_PID(&sc->sc_pcic_lock.lk_lockholder));

		s = splhigh();
		if ((pe = SIMPLEQ_FIRST(&h->events)) == NULL) {
			splx(s);
			if (first) {
				first = 0;
				config_pending_decr();
			}
			/*
			 * No events to process; release the PCIC lock.
			 */
			(void) lockmgr(&sc->sc_pcic_lock, LK_RELEASE, &sc->sc_pcic_lock.lk_lnterlock, LOCKHOLDER_PID(&sc->sc_pcic_lock.lk_lockholder));
			(void) kthread_tsleep(&h->events, PWAIT, "pcicev", 0);
			continue;
		} else {
			splx(s);
			/* sleep .25s to be enqueued chatterling interrupts */
			(void) kthread_tsleep((caddr_t)pcic_event_thread, PWAIT, "pcicss", hz/4);
		}
		s = splhigh();
		SIMPLEQ_REMOVE_HEAD(&h->events, pe_q);
		splx(s);

		switch (pe->pe_type) {
		case PCIC_EVENT_INSERTION:
			s = splhigh();
			while (1) {
				struct pcic_event *pe1, *pe2;

				if ((pe1 = SIMPLEQ_FIRST(&h->events)) == NULL)
					break;
				if (pe1->pe_type != PCIC_EVENT_REMOVAL)
					break;
				if ((pe2 = SIMPLEQ_NEXT(pe1, pe_q)) == NULL)
					break;
				if (pe2->pe_type == PCIC_EVENT_INSERTION) {
					SIMPLEQ_REMOVE_HEAD(&h->events, pe_q);
					free(pe1, M_TEMP);
					SIMPLEQ_REMOVE_HEAD(&h->events, pe_q);
					free(pe2, M_TEMP);
				}
			}
			splx(s);

			DPRINTF(("%s: insertion event\n",
			    h->ph_parent->dv_xname));
			pcic_attach_card(h);
			break;

		case PCIC_EVENT_REMOVAL:
			s = splhigh();
			while (1) {
				struct pcic_event *pe1, *pe2;

				if ((pe1 = SIMPLEQ_FIRST(&h->events)) == NULL)
					break;
				if (pe1->pe_type != PCIC_EVENT_INSERTION)
					break;
				if ((pe2 = SIMPLEQ_NEXT(pe1, pe_q)) == NULL)
					break;
				if (pe2->pe_type == PCIC_EVENT_REMOVAL) {
					SIMPLEQ_REMOVE_HEAD(&h->events, pe_q);
					free(pe1, M_TEMP);
					SIMPLEQ_REMOVE_HEAD(&h->events, pe_q);
					free(pe2, M_TEMP);
				}
			}
			splx(s);

			DPRINTF(("%s: removal event\n",
			    h->ph_parent->dv_xname));
			pcic_detach_card(h, DETACH_FORCE);
			break;

		default:
			panic("pcic_event_thread: unknown event %d",
			    pe->pe_type);
		}
		free(pe, M_TEMP);

		(void) lockmgr(&sc->sc_pcic_lock, LK_RELEASE, &sc->sc_pcic_lock.lk_lnterlock, LOCKHOLDER_PID(&sc->sc_pcic_lock.lk_lockholder));
	}

	h->event_thread = NULL;

	/* In case parent is waiting for us to exit. */
	kthread_wakeup(sc);

	kthread_exit(0);
}

int
pcic_submatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{

	struct pcmciabus_attach_args *paa = aux;
	struct pcic_handle *h = (struct pcic_handle *) paa->pch;

	switch (h->sock) {
	case C0SA:
		if (cf->cf_loc[PCMCIABUSCF_CONTROLLER] != PCMCIABUSCF_CONTROLLER_DEFAULT
				&& cf->cf_loc[PCMCIABUSCF_CONTROLLER] != 0)
			return 0;
		if (cf->cf_loc[PCMCIABUSCF_SOCKET] != PCMCIABUSCF_SOCKET_DEFAULT
				&& cf->cf_loc[PCMCIABUSCF_SOCKET] != 0)
			return 0;

		break;
	case C0SB:
		if (cf->cf_loc[PCMCIABUSCF_CONTROLLER] != PCMCIABUSCF_CONTROLLER_DEFAULT
				&& cf->cf_loc[PCMCIABUSCF_CONTROLLER] != 0)
			return 0;
		if (cf->cf_loc[PCMCIABUSCF_SOCKET] != PCMCIABUSCF_SOCKET_DEFAULT
				&& cf->cf_loc[PCMCIABUSCF_SOCKET] != 1)
			return 0;

		break;
	case C1SA:
		if (cf->cf_loc[PCMCIABUSCF_CONTROLLER] != PCMCIABUSCF_CONTROLLER_DEFAULT
				&& cf->cf_loc[PCMCIABUSCF_CONTROLLER] != 1)
			return 0;
		if (cf->cf_loc[PCMCIABUSCF_SOCKET] != PCMCIABUSCF_SOCKET_DEFAULT
				&& cf->cf_loc[PCMCIABUSCF_SOCKET] != 0)
			return 0;

		break;
	case C1SB:
		if (cf->cf_loc[PCMCIABUSCF_CONTROLLER] != PCMCIABUSCF_CONTROLLER_DEFAULT
				&& cf->cf_loc[PCMCIABUSCF_CONTROLLER] != 1)
			return 0;
		if (cf->cf_loc[PCMCIABUSCF_SOCKET] != PCMCIABUSCF_SOCKET_DEFAULT
				&& cf->cf_loc[PCMCIABUSCF_SOCKET] != 1)
			return 0;

		break;
	default:
		panic("unknown pcic socket");
	}

	return (config_match(parent, cf, aux));
}

int
pcic_print(arg, pnp)
	void *arg;
	const char *pnp;
{
	struct pcmciabus_attach_args *paa = arg;
	struct pcic_handle *h = (struct pcic_handle *) paa->pch;

	/* Only "pcmcia"s can attach to "pcic"s... easy. */
	if (pnp)
		printf("pcmcia at %s", pnp);

	switch (h->sock) {
	case C0SA:
		printf(" controller 0 socket 0");
		break;
	case C0SB:
		printf(" controller 0 socket 1");
		break;
	case C1SA:
		printf(" controller 1 socket 0");
		break;
	case C1SB:
		printf(" controller 1 socket 1");
		break;
	default:
		panic("unknown pcic socket");
	}

	return (UNCONF);
}

void
pcic_poll_intr(arg)
	void *arg;
{
	struct pcic_softc *sc;
	int i, s;

	s = spltty();
	sc = arg;
	for (i = 0; i < PCIC_NSLOTS; i++)
		if (sc->handle[i].flags & PCIC_FLAG_SOCKETP)
			(void)pcic_intr_socket(&sc->handle[i]);
	callout_reset(&sc->poll_ch, hz / 2, pcic_poll_intr, sc);
	splx(s);
}

int
pcic_intr(arg)
	void *arg;
{
	struct pcic_softc *sc = arg;
	int i, ret = 0;

	DPRINTF(("%s: intr\n", sc->dev.dv_xname));

	for (i = 0; i < PCIC_NSLOTS; i++)
		if (sc->handle[i].flags & PCIC_FLAG_SOCKETP)
			ret += pcic_intr_socket(&sc->handle[i]);

	return (ret ? 1 : 0);
}


int
pcic_intr_socket(h)
	struct pcic_handle *h;
{
	int cscreg;

	cscreg = pcic_read(h, PCIC_CSC);

	cscreg &= (PCIC_CSC_GPI |
		   PCIC_CSC_CD |
		   PCIC_CSC_READY |
		   PCIC_CSC_BATTWARN |
		   PCIC_CSC_BATTDEAD);

	if (cscreg & PCIC_CSC_GPI) {
		DPRINTF(("%s: %02x GPI\n", h->ph_parent->dv_xname, h->sock));
	}
	if (cscreg & PCIC_CSC_CD) {
		int statreg;

		statreg = pcic_read(h, PCIC_IF_STATUS);

		DPRINTF(("%s: %02x CD %x\n", h->ph_parent->dv_xname, h->sock,
		    statreg));

		if ((statreg & PCIC_IF_STATUS_CARDDETECT_MASK) ==
		    PCIC_IF_STATUS_CARDDETECT_PRESENT) {
			if (h->laststate != PCIC_LASTSTATE_PRESENT) {
				DPRINTF(("%s: enqueing INSERTION event\n",
					 h->ph_parent->dv_xname));
				pcic_queue_event(h, PCIC_EVENT_INSERTION);
			}
			h->laststate = PCIC_LASTSTATE_PRESENT;
		} else {
			if (h->laststate == PCIC_LASTSTATE_PRESENT) {
				/* Deactivate the card now. */
				DPRINTF(("%s: deactivating card\n",
					 h->ph_parent->dv_xname));
				pcic_deactivate_card(h);

				DPRINTF(("%s: enqueing REMOVAL event\n",
					 h->ph_parent->dv_xname));
				pcic_queue_event(h, PCIC_EVENT_REMOVAL);
			}
			h->laststate =
			    ((statreg & PCIC_IF_STATUS_CARDDETECT_MASK) == 0) ?
			    PCIC_LASTSTATE_EMPTY : PCIC_LASTSTATE_HALF;
		}
	}
	if (cscreg & PCIC_CSC_READY) {
		DPRINTF(("%s: %02x READY\n", h->ph_parent->dv_xname, h->sock));
		/* shouldn't happen */
	}
	if (cscreg & PCIC_CSC_BATTWARN) {
		DPRINTF(("%s: %02x BATTWARN\n", h->ph_parent->dv_xname,
		    h->sock));
	}
	if (cscreg & PCIC_CSC_BATTDEAD) {
		DPRINTF(("%s: %02x BATTDEAD\n", h->ph_parent->dv_xname,
		    h->sock));
	}
	return (cscreg ? 1 : 0);
}

void
pcic_queue_event(h, event)
	struct pcic_handle *h;
	int event;
{
	struct pcic_event *pe;
	int s;

	pe = malloc(sizeof(*pe), M_TEMP, M_NOWAIT);
	if (pe == NULL)
		panic("pcic_queue_event: can't allocate event");

	pe->pe_type = event;
	s = splhigh();
	SIMPLEQ_INSERT_TAIL(&h->events, pe, pe_q);
	splx(s);
	wakeup(&h->events);
}

void
pcic_attach_card(h)
	struct pcic_handle *h;
{
	if (!(h->flags & PCIC_FLAG_CARDP)) {
		/* call the MI attach function */
		pcmcia_card_attach(h->pcmcia);

		h->flags |= PCIC_FLAG_CARDP;
	} else {
		DPRINTF(("pcic_attach_card: already attached"));
	}
}

void
pcic_detach_card(h, flags)
	struct pcic_handle *h;
	int flags;		/* DETACH_* */
{
	if (h->flags & PCIC_FLAG_CARDP) {
		h->flags &= ~PCIC_FLAG_CARDP;

		/* call the MI detach function */
		pcmcia_card_detach(h->pcmcia, flags);
	} else {
		DPRINTF(("pcic_detach_card: already detached"));
	}
}


void
pcic_deactivate_card(h)
	struct pcic_handle *h;
{
	int intr;

	/* call the MI deactivate function */
	pcmcia_card_deactivate(h->pcmcia);

	/* power down the socket */
	pcic_write(h, PCIC_PWRCTL, 0);

	/* reset the socket */
	intr = pcic_read(h, PCIC_INTR);
	intr &= PCIC_INTR_ENABLE;
	pcic_write(h, PCIC_INTR, intr);
}

int 
pcic_chip_mem_alloc(pch, size, pcmhp)
	pcmcia_chipset_handle_t pch;
	bus_size_t size;
	struct pcmcia_mem_handle *pcmhp;
{
	struct pcic_handle *h = (struct pcic_handle *) pch;
	bus_space_handle_t memh;
	bus_addr_t addr;
	bus_size_t sizepg;
	int i, mask, mhandle;
	struct pcic_softc *sc = (struct pcic_softc *)h->ph_parent;

	/* out of sc->memh, allocate as many pages as necessary */

	/* convert size to PCIC pages */
	sizepg = (size + (PCIC_MEM_ALIGN - 1)) / PCIC_MEM_ALIGN;
	if (sizepg > PCIC_MAX_MEM_PAGES)
		return (1);

	mask = (1 << sizepg) - 1;

	addr = 0;		/* XXX gcc -Wuninitialized */
	mhandle = 0;		/* XXX gcc -Wuninitialized */

	for (i = 0; i <= PCIC_MAX_MEM_PAGES - sizepg; i++) {
		if ((sc->subregionmask & (mask << i)) == (mask << i)) {
			if (bus_space_subregion(sc->memt, sc->memh, i * PCIC_MEM_PAGESIZE,
					sizepg * PCIC_MEM_PAGESIZE, &memh))
				return (1);
			mhandle = mask << i;
			addr = sc->membase + (i * PCIC_MEM_PAGESIZE);
			sc->subregionmask &= ~(mhandle);
			pcmhp->memt = sc->memt;
			pcmhp->memh = memh;
			pcmhp->addr = addr;
			pcmhp->size = size;
			pcmhp->mhandle = mhandle;
			pcmhp->realsize = sizepg * PCIC_MEM_PAGESIZE;
			return (0);
		}
	}

	return (1);
}

void 
pcic_chip_mem_free(pch, pcmhp)
	pcmcia_chipset_handle_t pch;
	struct pcmcia_mem_handle *pcmhp;
{
	struct pcic_handle *h = (struct pcic_handle *) pch;
	struct pcic_softc *sc = (struct pcic_softc *)h->ph_parent;

	sc->subregionmask |= pcmhp->mhandle;
}

static struct mem_map_index_st {
	int	sysmem_start_lsb;
	int	sysmem_start_msb;
	int	sysmem_stop_lsb;
	int	sysmem_stop_msb;
	int	cardmem_lsb;
	int	cardmem_msb;
	int	memenable;
} mem_map_index[] = {
	{
		PCIC_SYSMEM_ADDR0_START_LSB,
		PCIC_SYSMEM_ADDR0_START_MSB,
		PCIC_SYSMEM_ADDR0_STOP_LSB,
		PCIC_SYSMEM_ADDR0_STOP_MSB,
		PCIC_CARDMEM_ADDR0_LSB,
		PCIC_CARDMEM_ADDR0_MSB,
		PCIC_ADDRWIN_ENABLE_MEM0,
	},
	{
		PCIC_SYSMEM_ADDR1_START_LSB,
		PCIC_SYSMEM_ADDR1_START_MSB,
		PCIC_SYSMEM_ADDR1_STOP_LSB,
		PCIC_SYSMEM_ADDR1_STOP_MSB,
		PCIC_CARDMEM_ADDR1_LSB,
		PCIC_CARDMEM_ADDR1_MSB,
		PCIC_ADDRWIN_ENABLE_MEM1,
	},
	{
		PCIC_SYSMEM_ADDR2_START_LSB,
		PCIC_SYSMEM_ADDR2_START_MSB,
		PCIC_SYSMEM_ADDR2_STOP_LSB,
		PCIC_SYSMEM_ADDR2_STOP_MSB,
		PCIC_CARDMEM_ADDR2_LSB,
		PCIC_CARDMEM_ADDR2_MSB,
		PCIC_ADDRWIN_ENABLE_MEM2,
	},
	{
		PCIC_SYSMEM_ADDR3_START_LSB,
		PCIC_SYSMEM_ADDR3_START_MSB,
		PCIC_SYSMEM_ADDR3_STOP_LSB,
		PCIC_SYSMEM_ADDR3_STOP_MSB,
		PCIC_CARDMEM_ADDR3_LSB,
		PCIC_CARDMEM_ADDR3_MSB,
		PCIC_ADDRWIN_ENABLE_MEM3,
	},
	{
		PCIC_SYSMEM_ADDR4_START_LSB,
		PCIC_SYSMEM_ADDR4_START_MSB,
		PCIC_SYSMEM_ADDR4_STOP_LSB,
		PCIC_SYSMEM_ADDR4_STOP_MSB,
		PCIC_CARDMEM_ADDR4_LSB,
		PCIC_CARDMEM_ADDR4_MSB,
		PCIC_ADDRWIN_ENABLE_MEM4,
	},
};


void 
pcic_chip_do_mem_map(h, win)
	struct pcic_handle *h;
	int win;
{
	int reg;
	int kind = h->mem[win].kind & ~PCMCIA_WIDTH_MEM_MASK;
	int mem8 =
	    (h->mem[win].kind & PCMCIA_WIDTH_MEM_MASK) == PCMCIA_WIDTH_MEM8
	    || (kind == PCMCIA_MEM_ATTR);

	DPRINTF(("mem8 %d\n", mem8));
	/* mem8 = 1; */

	pcic_write(h, mem_map_index[win].sysmem_start_lsb,
	    (h->mem[win].addr >> PCIC_SYSMEM_ADDRX_SHIFT) & 0xff);
	pcic_write(h, mem_map_index[win].sysmem_start_msb,
	    ((h->mem[win].addr >> (PCIC_SYSMEM_ADDRX_SHIFT + 8)) &
	    PCIC_SYSMEM_ADDRX_START_MSB_ADDR_MASK) |
	    (mem8 ? 0 : PCIC_SYSMEM_ADDRX_START_MSB_DATASIZE_16BIT));

	pcic_write(h, mem_map_index[win].sysmem_stop_lsb,
	    ((h->mem[win].addr + h->mem[win].size) >>
	    PCIC_SYSMEM_ADDRX_SHIFT) & 0xff);
	pcic_write(h, mem_map_index[win].sysmem_stop_msb,
	    (((h->mem[win].addr + h->mem[win].size) >>
	    (PCIC_SYSMEM_ADDRX_SHIFT + 8)) &
	    PCIC_SYSMEM_ADDRX_STOP_MSB_ADDR_MASK) |
	    PCIC_SYSMEM_ADDRX_STOP_MSB_WAIT2);

	pcic_write(h, mem_map_index[win].cardmem_lsb,
	    (h->mem[win].offset >> PCIC_CARDMEM_ADDRX_SHIFT) & 0xff);
	pcic_write(h, mem_map_index[win].cardmem_msb,
	    ((h->mem[win].offset >> (PCIC_CARDMEM_ADDRX_SHIFT + 8)) &
	    PCIC_CARDMEM_ADDRX_MSB_ADDR_MASK) |
	    ((kind == PCMCIA_MEM_ATTR) ?
	    PCIC_CARDMEM_ADDRX_MSB_REGACTIVE_ATTR : 0));

	reg = pcic_read(h, PCIC_ADDRWIN_ENABLE);
	reg |= (mem_map_index[win].memenable | PCIC_ADDRWIN_ENABLE_MEMCS16);
	pcic_write(h, PCIC_ADDRWIN_ENABLE, reg);

	delay(100);

#ifdef PCICDEBUG
	{
		int r1, r2, r3, r4, r5, r6;

		r1 = pcic_read(h, mem_map_index[win].sysmem_start_msb);
		r2 = pcic_read(h, mem_map_index[win].sysmem_start_lsb);
		r3 = pcic_read(h, mem_map_index[win].sysmem_stop_msb);
		r4 = pcic_read(h, mem_map_index[win].sysmem_stop_lsb);
		r5 = pcic_read(h, mem_map_index[win].cardmem_msb);
		r6 = pcic_read(h, mem_map_index[win].cardmem_lsb);

		DPRINTF(("pcic_chip_do_mem_map window %d: %02x%02x %02x%02x "
		    "%02x%02x\n", win, r1, r2, r3, r4, r5, r6));
	}
#endif
}

int 
pcic_chip_mem_map(pch, kind, card_addr, size, pcmhp, offsetp, windowp)
	pcmcia_chipset_handle_t pch;
	int kind;
	bus_addr_t card_addr;
	bus_size_t size;
	struct pcmcia_mem_handle *pcmhp;
	bus_addr_t *offsetp;
	int *windowp;
{
	struct pcic_handle *h = (struct pcic_handle *) pch;
	bus_addr_t busaddr;
	long card_offset;
	int i, win;
	struct pcic_softc *sc = (struct pcic_softc *)h->ph_parent;

	win = -1;
	for (i = 0; i < (sizeof(mem_map_index) / sizeof(mem_map_index[0]));
	    i++) {
		if ((h->memalloc & (1 << i)) == 0) {
			win = i;
			h->memalloc |= (1 << i);
			break;
		}
	}

	if (win == -1)
		return (1);

	*windowp = win;

	/* XXX this is pretty gross */

	if (sc->memt != pcmhp->memt)
		panic("pcic_chip_mem_map memt is bogus");

	busaddr = pcmhp->addr;

	/*
	 * compute the address offset to the pcmcia address space for the
	 * pcic.  this is intentionally signed.  The masks and shifts below
	 * will cause TRT to happen in the pcic registers.  Deal with making
	 * sure the address is aligned, and return the alignment offset.
	 */

	*offsetp = card_addr % PCIC_MEM_ALIGN;
	card_addr -= *offsetp;

	DPRINTF(("pcic_chip_mem_map window %d bus %lx+%lx+%lx at card addr "
	    "%lx\n", win, (u_long) busaddr, (u_long) * offsetp, (u_long) size,
	    (u_long) card_addr));

	/*
	 * include the offset in the size, and decrement size by one, since
	 * the hw wants start/stop
	 */
	size += *offsetp - 1;

	card_offset = (((long) card_addr) - ((long) busaddr));

	h->mem[win].addr = busaddr;
	h->mem[win].size = size;
	h->mem[win].offset = card_offset;
	h->mem[win].kind = kind;

	pcic_chip_do_mem_map(h, win);

	return (0);
}

void 
pcic_chip_mem_unmap(pch, window)
	pcmcia_chipset_handle_t pch;
	int window;
{
	struct pcic_handle *h = (struct pcic_handle *) pch;
	int reg;

	if (window >= (sizeof(mem_map_index) / sizeof(mem_map_index[0])))
		panic("pcic_chip_mem_unmap: window out of range");

	reg = pcic_read(h, PCIC_ADDRWIN_ENABLE);
	reg &= ~mem_map_index[window].memenable;
	pcic_write(h, PCIC_ADDRWIN_ENABLE, reg);

	h->memalloc &= ~(1 << window);
}

int 
pcic_chip_io_alloc(pch, start, size, align, pcihp)
	pcmcia_chipset_handle_t pch;
	bus_addr_t start;
	bus_size_t size;
	bus_size_t align;
	struct pcmcia_io_handle *pcihp;
{
	struct pcic_handle *h = (struct pcic_handle *) pch;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	bus_addr_t ioaddr;
	int flags = 0;
	struct pcic_softc *sc = (struct pcic_softc *)h->ph_parent;

	/*
	 * Allocate some arbitrary I/O space.
	 */

	iot = sc->iot;

	if (start) {
		ioaddr = start;
		if (bus_space_map(iot, start, size, 0, &ioh))
			return (1);
		DPRINTF(
				("pcic_chip_io_alloc map port %lx+%lx\n", (u_long) ioaddr, (u_long) size));
	} else {
		flags |= PCMCIA_IO_ALLOCATED;
		if (bus_space_alloc(iot, sc->iobase, sc->iobase + sc->iosize, size,
				align, 0, 0, &ioaddr, &ioh))
			return (1);
		DPRINTF(
				("pcic_chip_io_alloc alloc port %lx+%lx\n", (u_long) ioaddr, (u_long) size));
	}

	pcihp->iot = iot;
	pcihp->ioh = ioh;
	pcihp->addr = ioaddr;
	pcihp->size = size;
	pcihp->flags = flags;

	return (0);
}

void 
pcic_chip_io_free(pch, pcihp)
	pcmcia_chipset_handle_t pch;
	struct pcmcia_io_handle *pcihp;
{
	bus_space_tag_t iot = pcihp->iot;
	bus_space_handle_t ioh = pcihp->ioh;
	bus_size_t size = pcihp->size;

	if (pcihp->flags & PCMCIA_IO_ALLOCATED)
		bus_space_free(iot, ioh, size);
	else
		bus_space_unmap(iot, ioh, size);
}

static const struct io_map_index_st {
	int	start_lsb;
	int	start_msb;
	int	stop_lsb;
	int	stop_msb;
	int	ioenable;
	int	ioctlmask;
	int	ioctlbits[3];		/* indexed by PCMCIA_WIDTH_* */
}       io_map_index[] = {
	{
		PCIC_IOADDR0_START_LSB,
		PCIC_IOADDR0_START_MSB,
		PCIC_IOADDR0_STOP_LSB,
		PCIC_IOADDR0_STOP_MSB,
		PCIC_ADDRWIN_ENABLE_IO0,
		PCIC_IOCTL_IO0_WAITSTATE | PCIC_IOCTL_IO0_ZEROWAIT |
		PCIC_IOCTL_IO0_IOCS16SRC_MASK | PCIC_IOCTL_IO0_DATASIZE_MASK,
		{
			PCIC_IOCTL_IO0_IOCS16SRC_CARD,
			PCIC_IOCTL_IO0_IOCS16SRC_DATASIZE |
			    PCIC_IOCTL_IO0_DATASIZE_8BIT,
			PCIC_IOCTL_IO0_IOCS16SRC_DATASIZE |
			    PCIC_IOCTL_IO0_DATASIZE_16BIT,
		},
	},
	{
		PCIC_IOADDR1_START_LSB,
		PCIC_IOADDR1_START_MSB,
		PCIC_IOADDR1_STOP_LSB,
		PCIC_IOADDR1_STOP_MSB,
		PCIC_ADDRWIN_ENABLE_IO1,
		PCIC_IOCTL_IO1_WAITSTATE | PCIC_IOCTL_IO1_ZEROWAIT |
		PCIC_IOCTL_IO1_IOCS16SRC_MASK | PCIC_IOCTL_IO1_DATASIZE_MASK,
		{
			PCIC_IOCTL_IO1_IOCS16SRC_CARD,
			PCIC_IOCTL_IO1_IOCS16SRC_DATASIZE |
			    PCIC_IOCTL_IO1_DATASIZE_8BIT,
			PCIC_IOCTL_IO1_IOCS16SRC_DATASIZE |
			    PCIC_IOCTL_IO1_DATASIZE_16BIT,
		},
	},
};


void 
pcic_chip_do_io_map(h, win)
	struct pcic_handle *h;
	int win;
{
	int reg;

	DPRINTF(("pcic_chip_do_io_map win %d addr %lx size %lx width %d\n",
	    win, (long) h->io[win].addr, (long) h->io[win].size,
	    h->io[win].width * 8));

	pcic_write(h, io_map_index[win].start_lsb, h->io[win].addr & 0xff);
	pcic_write(h, io_map_index[win].start_msb,
	    (h->io[win].addr >> 8) & 0xff);

	pcic_write(h, io_map_index[win].stop_lsb,
	    (h->io[win].addr + h->io[win].size - 1) & 0xff);
	pcic_write(h, io_map_index[win].stop_msb,
	    ((h->io[win].addr + h->io[win].size - 1) >> 8) & 0xff);

	reg = pcic_read(h, PCIC_IOCTL);
	reg &= ~io_map_index[win].ioctlmask;
	reg |= io_map_index[win].ioctlbits[h->io[win].width];
	pcic_write(h, PCIC_IOCTL, reg);

	reg = pcic_read(h, PCIC_ADDRWIN_ENABLE);
	reg |= io_map_index[win].ioenable;
	pcic_write(h, PCIC_ADDRWIN_ENABLE, reg);
}

int 
pcic_chip_io_map(pch, width, offset, size, pcihp, windowp)
	pcmcia_chipset_handle_t pch;
	int width;
	bus_addr_t offset;
	bus_size_t size;
	struct pcmcia_io_handle *pcihp;
	int *windowp;
{
	struct pcic_handle *h = (struct pcic_handle *) pch;
	bus_addr_t ioaddr = pcihp->addr + offset;
	int i, win;
#ifdef PCICDEBUG
	static char *width_names[] = { "auto", "io8", "io16" };
#endif
	struct pcic_softc *sc = (struct pcic_softc *)h->ph_parent;
	
	/* XXX Sanity check offset/size. */

	win = -1;
	for (i = 0; i < (sizeof(io_map_index) / sizeof(io_map_index[0])); i++) {
		if ((h->ioalloc & (1 << i)) == 0) {
			win = i;
			h->ioalloc |= (1 << i);
			break;
		}
	}

	if (win == -1)
		return (1);

	*windowp = win;

	/* XXX this is pretty gross */

	if (sc->iot != pcihp->iot)
		panic("pcic_chip_io_map iot is bogus");

	DPRINTF(("pcic_chip_io_map window %d %s port %lx+%lx\n",
		 win, width_names[width], (u_long) ioaddr, (u_long) size));

	/* XXX wtf is this doing here? */

	printf(" port 0x%lx", (u_long) ioaddr);
	if (size > 1)
		printf("-0x%lx", (u_long) ioaddr + (u_long) size - 1);

	h->io[win].addr = ioaddr;
	h->io[win].size = size;
	h->io[win].width = width;

	pcic_chip_do_io_map(h, win);

	return (0);
}

void 
pcic_chip_io_unmap(pch, window)
	pcmcia_chipset_handle_t pch;
	int window;
{
	struct pcic_handle *h = (struct pcic_handle *) pch;
	int reg;

	if (window >= (sizeof(io_map_index) / sizeof(io_map_index[0])))
		panic("pcic_chip_io_unmap: window out of range");

	reg = pcic_read(h, PCIC_ADDRWIN_ENABLE);
	reg &= ~io_map_index[window].ioenable;
	pcic_write(h, PCIC_ADDRWIN_ENABLE, reg);

	h->ioalloc &= ~(1 << window);
}

static void
pcic_wait_ready(h)
	struct pcic_handle *h;
{
	int i;

	/* wait an initial 10ms for quick cards */
	if (pcic_read(h, PCIC_IF_STATUS) & PCIC_IF_STATUS_READY)
		return;
	pcic_delay(h, 10, "pccwr0");
	for (i = 0; i < 50; i++) {
		if (pcic_read(h, PCIC_IF_STATUS) & PCIC_IF_STATUS_READY)
			return;
		/* wait .1s (100ms) each iteration now */
		pcic_delay(h, 100, "pccwr1");
#ifdef PCICDEBUG
		if (pcic_debug) {
			if ((i > 20) && (i % 100 == 99))
				printf(".");
		}
#endif
	}

#ifdef DIAGNOSTIC
	printf("pcic_wait_ready: ready never happened, status = %02x\n",
	    pcic_read(h, PCIC_IF_STATUS));
#endif
}

/*
 * Perform long (msec order) delay.
 */
static void
pcic_delay(h, timo, wmesg)
	struct pcic_handle *h;
	int timo;			/* in ms.  must not be zero */
	char *wmesg;
{
#ifdef DIAGNOSTIC
	if (timo <= 0) {
		printf("called with timeout %d\n", timo);
		panic("pcic_delay");
	}
	if (curproc == NULL) {
		printf("called in interrupt context\n");
		panic("pcic_delay");
	}
	if (h->event_thread == NULL) {
		printf("no event thread\n");
		panic("pcic_delay");
	}
#endif
	DPRINTF(
			("pcic_delay: \"%s\" %p, sleep %d ms\n", wmesg, h->event_thread, timo));
	tsleep(pcic_delay, PWAIT, wmesg, roundup(timo * hz, 1000) / 1000);
}

void
pcic_chip_socket_enable(pch)
	pcmcia_chipset_handle_t pch;
{
	struct pcic_handle *h = (struct pcic_handle *) pch;
	int cardtype, win, intr, pwr;
#if defined(DIAGNOSTIC) || defined(PCICDEBUG)
	int reg;
#endif

#ifdef DIAGNOSTIC
	if (h->flags & PCIC_FLAG_ENABLED)
		printf("pcic_chip_socket_enable: enabling twice\n");
#endif

	/* disable interrupts */
	intr = pcic_read(h, PCIC_INTR);
	intr &= ~PCIC_INTR_IRQ_MASK;
	pcic_write(h, PCIC_INTR, intr);

	/* power down the socket to reset it, clear the card reset pin */
	pwr = 0;
	pcic_write(h, PCIC_PWRCTL, pwr);

	/*
	 * wait 300ms until power fails (Tpf).  Then, wait 100ms since
	 * we are changing Vcc (Toff).
	 */
	pcic_delay(h, 300 + 100, "pccen0");

	/*
	 * power hack for RICOH RF5C[23]96
	 */
	switch( h->vendor ) {
	case PCIC_VENDOR_RICOH_5C296:
	case PCIC_VENDOR_RICOH_5C396:
	{
		int regtmp;
		regtmp = pcic_read(h, PCIC_RICOH_REG_MCR2);
#ifdef RICOH_POWER_HACK
		regtmp |= PCIC_RICOH_MCR2_VCC_DIRECT;
#else
		regtmp &= ~(PCIC_RICOH_MCR2_VCC_DIRECT|PCIC_RICOH_MCR2_VCC_SEL_3V);
#endif
		pcic_write(h, PCIC_RICOH_REG_MCR2, regtmp);
	}
		break;
	default:
		break;
	}

#ifdef VADEM_POWER_HACK
	bus_space_write_1(sc->iot, sc->ioh, PCIC_REG_INDEX, 0x0e);
	bus_space_write_1(sc->iot, sc->ioh, PCIC_REG_INDEX, 0x37);
	printf("prcr = %02x\n", pcic_read(h, 0x02));
	printf("cvsr = %02x\n", pcic_read(h, 0x2f));
	printf("DANGER WILL ROBINSON!  Changing voltage select!\n");
	pcic_write(h, 0x2f, pcic_read(h, 0x2f) & ~0x03);
	printf("cvsr = %02x\n", pcic_read(h, 0x2f));
#endif
	/* power up the socket */
	pwr |= PCIC_PWRCTL_DISABLE_RESETDRV | PCIC_PWRCTL_PWR_ENABLE | PCIC_PWRCTL_VPP1_VCC;
	pcic_write(h, PCIC_PWRCTL, pwr);

	/*
	 * wait 100ms until power raise (Tpr) and 20ms to become
	 * stable (Tsu(Vcc)).
	 *
	 * some machines require some more time to be settled
	 * (300ms is added here).
	 */
	pcic_delay(h, 100 + 20 + 300, "pccen1");
	pwr |= PCIC_PWRCTL_OE;
	pcic_write(h, PCIC_PWRCTL, pwr);

	/* now make sure we have reset# active */
	intr &= ~PCIC_INTR_RESET;
	pcic_write(h, PCIC_INTR, intr);

	pcic_write(h, PCIC_PWRCTL, PCIC_PWRCTL_DISABLE_RESETDRV |
	    PCIC_PWRCTL_OE | PCIC_PWRCTL_PWR_ENABLE | PCIC_PWRCTL_VPP1_VCC);
	/*
	 * hold RESET at least 10us, this is a min allow for slop in
	 * delay routine.
	 */
	delay(20);

	/* clear the reset flag */
	intr |= PCIC_INTR_RESET;
	pcic_write(h, PCIC_INTR, intr);

	/* wait 20ms as per pc card standard (r2.01) section 4.3.6 */
	pcic_delay(h, 20, "pccen2");

#if defined(DIAGNOSTIC) || defined(PCICDEBUG)
	reg = pcic_read(h, PCIC_IF_STATUS);
#endif
#ifdef DIAGNOSTIC
	if (!(reg & PCIC_IF_STATUS_POWERACTIVE)) {
		printf("pcic_chip_socket_enable: status %x\n", reg);
	}
#endif
	/* wait for the chip to finish initializing */
	pcic_wait_ready(h);

	/* zero out the address windows */
	pcic_write(h, PCIC_ADDRWIN_ENABLE, 0);

	/* set the card type and enable the interrupt */
	cardtype = pcmcia_card_gettype(h->pcmcia);
	intr |= ((cardtype == PCMCIA_IFTYPE_IO) ? PCIC_INTR_CARDTYPE_IO : PCIC_INTR_CARDTYPE_MEM);
	pcic_write(h, PCIC_INTR, intr);

	DPRINTF(
			("%s: pcic_chip_socket_enable %02x cardtype %s %02x\n", h->ph_parent->dv_xname, h->sock, ((cardtype == PCMCIA_IFTYPE_IO) ? "io" : "mem"), reg));

	/* reinstall all the memory and io mappings */
	for (win = 0; win < PCIC_MEM_WINS; win++)
		if (h->memalloc & (1 << win))
			pcic_chip_do_mem_map(h, win);
	for (win = 0; win < PCIC_IO_WINS; win++)
		if (h->ioalloc & (1 << win))
			pcic_chip_do_io_map(h, win);

	h->flags |= PCIC_FLAG_ENABLED;

	/* finally enable the interrupt */
	intr |= h->ih_irq;
	pcic_write(h, PCIC_INTR, intr);
}

void
pcic_chip_socket_disable(pch)
	pcmcia_chipset_handle_t pch;
{
	struct pcic_handle *h = (struct pcic_handle *) pch;
	int intr;

	DPRINTF(("pcic_chip_socket_disable\n"));

	/* disable interrupts */
	intr = pcic_read(h, PCIC_INTR);
	intr &= ~PCIC_INTR_IRQ_MASK;
	pcic_write(h, PCIC_INTR, intr);

	/* power down the socket */
	pcic_write(h, PCIC_PWRCTL, 0);

	/* zero out the address windows */
	pcic_write(h, PCIC_ADDRWIN_ENABLE, 0);

	h->flags &= ~PCIC_FLAG_ENABLED;
}

static u_int8_t
st_pcic_read(h, idx)
	struct pcic_handle *h;
	int idx;
{

	if (idx != -1)
		bus_space_write_1(h->ph_bus_t, h->ph_bus_h, PCIC_REG_INDEX,
				h->sock + idx);
	return (bus_space_read_1(h->ph_bus_t, h->ph_bus_h, PCIC_REG_DATA));
}

static void
st_pcic_write(h, idx, data)
	struct pcic_handle *h;
	int idx;
	u_int8_t data;
{

	if (idx != -1)
		bus_space_write_1(h->ph_bus_t, h->ph_bus_h, PCIC_REG_INDEX,
				h->sock + idx);
	bus_space_write_1(h->ph_bus_t, h->ph_bus_h, PCIC_REG_DATA, data);
}
