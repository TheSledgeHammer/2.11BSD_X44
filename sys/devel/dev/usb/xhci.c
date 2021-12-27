/*	$NetBSD: xhci.c,v 1.138 2021/01/05 18:00:21 skrll Exp $	*/

/*
 * Copyright (c) 2013 Jonathan A. Kollasch
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * USB rev 2.0 and rev 3.1 specification
 *  http://www.usb.org/developers/docs/
 * xHCI rev 1.1 specification
 *  http://www.intel.com/technology/usb/spec.htm
 */

#include <sys/cdefs.h>
/* __KERNEL_RCSID(0, "$NetBSD: xhci.c,v 1.138 2021/01/05 18:00:21 skrll Exp $"); */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/select.h>
#include <sys/proc.h>
#include <sys/queue.h>
#include <sys/sysctl.h>

#include <machine/bus.h>
#include <machine/endian.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbhist.h>
#include <dev/usb/usb_mem.h>
#include <dev/usb/usb_quirks.h>

#include <dev/usb/xhcireg.h>
#include <dev/usb/xhcivar.h>
#include <dev/usb/usbroothub.h>

#ifndef HEXDUMP
#define HEXDUMP(a, b, c)
#endif

#define DPRINTF(FMT,A,B,C,D)	USBHIST_LOG(xhcidebug,FMT,A,B,C,D)
#define DPRINTFN(N,FMT,A,B,C,D)	USBHIST_LOGN(xhcidebug,N,FMT,A,B,C,D)
#define XHCIHIST_FUNC()			USBHIST_FUNC()
#define XHCIHIST_CALLED(name)	USBHIST_CALLED(xhcidebug)
#define XHCIHIST_CALLARGS(FMT,A,B,C,D) \
				USBHIST_CALLARGS(xhcidebug,FMT,A,B,C,D)

#define XHCI_DCI_SLOT 			0
#define XHCI_DCI_EP_CONTROL 	1

#define XHCI_ICI_INPUT_CONTROL 	0

struct xhci_pipe {
	struct usbd_pipe 	xp_pipe;
	struct usb_task 	xp_async_task;
	int16_t 			xp_isoc_next; 	/* next frame */
	uint8_t 			xp_maxb; 		/* max burst */
	uint8_t 			xp_mult;
};

#define XHCI_COMMAND_RING_TRBS 		256
#define XHCI_EVENT_RING_TRBS 		256
#define XHCI_EVENT_RING_SEGMENTS 	1
#define XHCI_TRB_3_ED_BIT 			XHCI_TRB_3_ISP_BIT

static usbd_status 	xhci_open(struct usbd_pipe *);
static void 		xhci_close_pipe(struct usbd_pipe *);
static int 			xhci_intr1(struct xhci_softc * const);
static void 		xhci_softintr(void *);
static void 		xhci_poll(struct usbd_bus *);
static struct usbd_xfer *xhci_allocx(struct usbd_bus *, unsigned int);
static void 		xhci_freex(struct usbd_bus *, struct usbd_xfer *);
static void 		xhci_abortx(struct usbd_xfer *);
static bool_t 		xhci_dying(struct usbd_bus *);
static void 		xhci_get_lock(struct usbd_bus *, struct lock_object **);
static usbd_status 	xhci_new_device(struct device *, struct usbd_bus *, int, int, int, struct usbd_port *);
static int 			xhci_roothub_ctrl(struct usbd_bus *, usb_device_request_t *, void *, int);

static usbd_status xhci_configure_endpoint(struct usbd_pipe *);
//static usbd_status xhci_unconfigure_endpoint(struct usbd_pipe *);
static usbd_status xhci_reset_endpoint(struct usbd_pipe *);
static usbd_status xhci_stop_endpoint(struct usbd_pipe *);

static void 		xhci_host_dequeue(struct xhci_ring * const);
static usbd_status 	xhci_set_dequeue(struct usbd_pipe *);

static usbd_status 	xhci_do_command(struct xhci_softc * const, struct xhci_soft_trb * const, int);
static usbd_status 	xhci_do_command_locked(struct xhci_softc * const, struct xhci_soft_trb * const, int);
static usbd_status 	xhci_init_slot(struct usbd_device *, uint32_t);
static void 		xhci_free_slot(struct xhci_softc *, struct xhci_slot *);
static usbd_status 	xhci_set_address(struct usbd_device *, uint32_t, bool_t);
static usbd_status 	xhci_enable_slot(struct xhci_softc * const, uint8_t * const);
static usbd_status 	xhci_disable_slot(struct xhci_softc * const, uint8_t);
static usbd_status 	xhci_address_device(struct xhci_softc * const, uint64_t, uint8_t, bool_t);
static void 		xhci_set_dcba(struct xhci_softc * const, uint64_t, int);
static usbd_status 	xhci_update_ep0_mps(struct xhci_softc * const, struct xhci_slot * const, u_int);
static usbd_status 	xhci_ring_init(struct xhci_softc * const, struct xhci_ring **, size_t, size_t);
static void 		xhci_ring_free(struct xhci_softc * const, struct xhci_ring ** const);

static void 		xhci_setup_ctx(struct usbd_pipe *);
static void 		xhci_setup_route(struct usbd_pipe *, uint32_t *);
static void 		xhci_setup_tthub(struct usbd_pipe *, uint32_t *);
static void 		xhci_setup_maxburst(struct usbd_pipe *, uint32_t *);
static uint32_t 	xhci_bival2ival(uint32_t, uint32_t);

static void 		xhci_noop(struct usbd_pipe *);

static usbd_status 	xhci_root_intr_transfer(struct usbd_xfer *);
static usbd_status 	xhci_root_intr_start(struct usbd_xfer *);
static void 		xhci_root_intr_abort(struct usbd_xfer *);
static void 		xhci_root_intr_close(struct usbd_pipe *);
static void 		xhci_root_intr_done(struct usbd_xfer *);

static usbd_status 	xhci_device_ctrl_transfer(struct usbd_xfer *);
static usbd_status 	xhci_device_ctrl_start(struct usbd_xfer *);
static void 		xhci_device_ctrl_abort(struct usbd_xfer *);
static void 		xhci_device_ctrl_close(struct usbd_pipe *);
static void 		xhci_device_ctrl_done(struct usbd_xfer *);

static usbd_status 	xhci_device_isoc_transfer(struct usbd_xfer *);
static usbd_status 	xhci_device_isoc_enter(struct usbd_xfer *);
static void 		xhci_device_isoc_abort(struct usbd_xfer *);
static void 		xhci_device_isoc_close(struct usbd_pipe *);
static void 		xhci_device_isoc_done(struct usbd_xfer *);

static usbd_status 	xhci_device_intr_transfer(struct usbd_xfer *);
static usbd_status 	xhci_device_intr_start(struct usbd_xfer *);
static void 		xhci_device_intr_abort(struct usbd_xfer *);
static void 		xhci_device_intr_close(struct usbd_pipe *);
static void 		xhci_device_intr_done(struct usbd_xfer *);

static usbd_status 	xhci_device_bulk_transfer(struct usbd_xfer *);
static usbd_status 	xhci_device_bulk_start(struct usbd_xfer *);
static void 		xhci_device_bulk_abort(struct usbd_xfer *);
static void 		xhci_device_bulk_close(struct usbd_pipe *);
static void 		xhci_device_bulk_done(struct usbd_xfer *);

static const struct usbd_bus_methods xhci_bus_methods = {
	.ubm_open = xhci_open,
	.ubm_softint = xhci_softintr,
	.ubm_dopoll = xhci_poll,
	.ubm_allocx = xhci_allocx,
	.ubm_freex = xhci_freex,
	.ubm_abortx = xhci_abortx,
	.ubm_dying = xhci_dying,
	.ubm_getlock = xhci_get_lock,
	.ubm_newdev = xhci_new_device,
	.ubm_rhctrl = xhci_roothub_ctrl,
};

static const struct usbd_pipe_methods xhci_root_intr_methods = {
	.upm_transfer = xhci_root_intr_transfer,
	.upm_start = xhci_root_intr_start,
	.upm_abort = xhci_root_intr_abort,
	.upm_close = xhci_root_intr_close,
	.upm_cleartoggle = xhci_noop,
	.upm_done = xhci_root_intr_done,
};


static const struct usbd_pipe_methods xhci_device_ctrl_methods = {
	.upm_transfer = xhci_device_ctrl_transfer,
	.upm_start = xhci_device_ctrl_start,
	.upm_abort = xhci_device_ctrl_abort,
	.upm_close = xhci_device_ctrl_close,
	.upm_cleartoggle = xhci_noop,
	.upm_done = xhci_device_ctrl_done,
};

static const struct usbd_pipe_methods xhci_device_isoc_methods = {
	.upm_transfer = xhci_device_isoc_transfer,
	.upm_abort = xhci_device_isoc_abort,
	.upm_close = xhci_device_isoc_close,
	.upm_cleartoggle = xhci_noop,
	.upm_done = xhci_device_isoc_done,
};

static const struct usbd_pipe_methods xhci_device_bulk_methods = {
	.upm_transfer = xhci_device_bulk_transfer,
	.upm_start = xhci_device_bulk_start,
	.upm_abort = xhci_device_bulk_abort,
	.upm_close = xhci_device_bulk_close,
	.upm_cleartoggle = xhci_noop,
	.upm_done = xhci_device_bulk_done,
};

static const struct usbd_pipe_methods xhci_device_intr_methods = {
	.upm_transfer = xhci_device_intr_transfer,
	.upm_start = xhci_device_intr_start,
	.upm_abort = xhci_device_intr_abort,
	.upm_close = xhci_device_intr_close,
	.upm_cleartoggle = xhci_noop,
	.upm_done = xhci_device_intr_done,
};

static inline uint32_t
xhci_read_1(const struct xhci_softc * const sc, bus_size_t offset)
{
	return bus_space_read_1(sc->sc_iot, sc->sc_ioh, offset);
}

static inline uint32_t
xhci_read_2(const struct xhci_softc * const sc, bus_size_t offset)
{
	return bus_space_read_2(sc->sc_iot, sc->sc_ioh, offset);
}

static inline uint32_t
xhci_read_4(const struct xhci_softc * const sc, bus_size_t offset)
{
	return bus_space_read_4(sc->sc_iot, sc->sc_ioh, offset);
}

static inline void
xhci_write_1(const struct xhci_softc * const sc, bus_size_t offset,
    uint32_t value)
{
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, offset, value);
}

#if 0 /* unused */
static inline void
xhci_write_4(const struct xhci_softc * const sc, bus_size_t offset,
    uint32_t value)
{
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, offset, value);
}
#endif /* unused */

static inline void
xhci_barrier(const struct xhci_softc * const sc, int flags)
{
	bus_space_barrier(sc->sc_iot, sc->sc_ioh, 0, sc->sc_ios, flags);
}

static inline uint32_t
xhci_cap_read_4(const struct xhci_softc * const sc, bus_size_t offset)
{
	return bus_space_read_4(sc->sc_iot, sc->sc_cbh, offset);
}

static inline uint32_t
xhci_op_read_4(const struct xhci_softc * const sc, bus_size_t offset)
{
	return bus_space_read_4(sc->sc_iot, sc->sc_obh, offset);
}

static inline void
xhci_op_write_4(const struct xhci_softc * const sc, bus_size_t offset,
    uint32_t value)
{
	bus_space_write_4(sc->sc_iot, sc->sc_obh, offset, value);
}

static inline uint64_t
xhci_op_read_8(const struct xhci_softc * const sc, bus_size_t offset)
{
	uint64_t value;

	if (XHCI_HCC_AC64(sc->sc_hcc)) {
#ifdef XHCI_USE_BUS_SPACE_8
		value = bus_space_read_8(sc->sc_iot, sc->sc_obh, offset);
#else
		value = bus_space_read_4(sc->sc_iot, sc->sc_obh, offset);
		value |= (uint64_t)bus_space_read_4(sc->sc_iot, sc->sc_obh,
		    offset + 4) << 32;
#endif
	} else {
		value = bus_space_read_4(sc->sc_iot, sc->sc_obh, offset);
	}

	return value;
}

static inline void
xhci_op_write_8(const struct xhci_softc * const sc, bus_size_t offset,
    uint64_t value)
{
	if (XHCI_HCC_AC64(sc->sc_hcc)) {
#ifdef XHCI_USE_BUS_SPACE_8
		bus_space_write_8(sc->sc_iot, sc->sc_obh, offset, value);
#else
		bus_space_write_4(sc->sc_iot, sc->sc_obh, offset + 0,
		    (value >> 0) & 0xffffffff);
		bus_space_write_4(sc->sc_iot, sc->sc_obh, offset + 4,
		    (value >> 32) & 0xffffffff);
#endif
	} else {
		bus_space_write_4(sc->sc_iot, sc->sc_obh, offset, value);
	}
}

static inline uint32_t
xhci_rt_read_4(const struct xhci_softc * const sc, bus_size_t offset)
{
	return bus_space_read_4(sc->sc_iot, sc->sc_rbh, offset);
}

static inline void
xhci_rt_write_4(const struct xhci_softc * const sc, bus_size_t offset, uint32_t value)
{
	bus_space_write_4(sc->sc_iot, sc->sc_rbh, offset, value);
}

#if 0 /* unused */
static inline uint64_t
xhci_rt_read_8(const struct xhci_softc * const sc, bus_size_t offset)
{
	uint64_t value;

	if (XHCI_HCC_AC64(sc->sc_hcc)) {
#ifdef XHCI_USE_BUS_SPACE_8
		value = bus_space_read_8(sc->sc_iot, sc->sc_rbh, offset);
#else
		value = bus_space_read_4(sc->sc_iot, sc->sc_rbh, offset);
		value |= (uint64_t)bus_space_read_4(sc->sc_iot, sc->sc_rbh,
		    offset + 4) << 32;
#endif
	} else {
		value = bus_space_read_4(sc->sc_iot, sc->sc_rbh, offset);
	}

	return value;
}
#endif /* unused */

static inline void
xhci_rt_write_8(const struct xhci_softc * const sc, bus_size_t offset, uint64_t value)
{
	if (XHCI_HCC_AC64(sc->sc_hcc)) {
#ifdef XHCI_USE_BUS_SPACE_8
		bus_space_write_8(sc->sc_iot, sc->sc_rbh, offset, value);
#else
		bus_space_write_4(sc->sc_iot, sc->sc_rbh, offset + 0,
		    (value >> 0) & 0xffffffff);
		bus_space_write_4(sc->sc_iot, sc->sc_rbh, offset + 4,
		    (value >> 32) & 0xffffffff);
#endif
	} else {
		bus_space_write_4(sc->sc_iot, sc->sc_rbh, offset, value);
	}
}

#if 0 /* unused */
static inline uint32_t
xhci_db_read_4(const struct xhci_softc * const sc, bus_size_t offset)
{
	return bus_space_read_4(sc->sc_iot, sc->sc_dbh, offset);
}
#endif /* unused */

static inline void
xhci_db_write_4(const struct xhci_softc * const sc, bus_size_t offset,
    uint32_t value)
{
	bus_space_write_4(sc->sc_iot, sc->sc_dbh, offset, value);
}

/* --- */


static inline uint8_t
xhci_ep_get_type(usb_endpoint_descriptor_t * const ed)
{
	u_int eptype;

	switch (UE_GET_XFERTYPE(ed->bmAttributes)) {
	case UE_CONTROL:
		eptype = 0x0;
		break;
	case UE_ISOCHRONOUS:
		eptype = 0x1;
		break;
	case UE_BULK:
		eptype = 0x2;
		break;
	case UE_INTERRUPT:
		eptype = 0x3;
		break;
	}

	if ((UE_GET_XFERTYPE(ed->bmAttributes) == UE_CONTROL) ||
	    (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN))
		return eptype | 0x4;
	else
		return eptype;
}

static u_int
xhci_ep_get_dci(usb_endpoint_descriptor_t * const ed)
{
	/* xHCI 1.0 section 4.5.1 */
	u_int epaddr = UE_GET_ADDR(ed->bEndpointAddress);
	u_int in = 0;

	if ((UE_GET_XFERTYPE(ed->bmAttributes) == UE_CONTROL) ||
	    (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN))
		in = 1;

	return epaddr * 2 + in;
}

static inline u_int
xhci_dci_to_ici(const u_int i)
{
	return i + 1;
}

static inline void *
xhci_slot_get_dcv(struct xhci_softc * const sc, struct xhci_slot * const xs,
    const u_int dci)
{
	return KERNADDR(&xs->xs_dc_dma, sc->sc_ctxsz * dci);
}

#if 0 /* unused */
static inline bus_addr_t
xhci_slot_get_dcp(struct xhci_softc * const sc, struct xhci_slot * const xs,
    const u_int dci)
{
	return DMAADDR(&xs->xs_dc_dma, sc->sc_ctxsz * dci);
}
#endif /* unused */

static inline void *
xhci_slot_get_icv(struct xhci_softc * const sc, struct xhci_slot * const xs,
    const u_int ici)
{
	return KERNADDR(&xs->xs_ic_dma, sc->sc_ctxsz * ici);
}

static inline bus_addr_t
xhci_slot_get_icp(struct xhci_softc * const sc, struct xhci_slot * const xs,
    const u_int ici)
{
	return DMAADDR(&xs->xs_ic_dma, sc->sc_ctxsz * ici);
}

static inline struct xhci_trb *
xhci_ring_trbv(struct xhci_ring * const xr, u_int idx)
{
	return KERNADDR(&xr->xr_dma, XHCI_TRB_SIZE * idx);
}

static inline bus_addr_t
xhci_ring_trbp(struct xhci_ring * const xr, u_int idx)
{
	return DMAADDR(&xr->xr_dma, XHCI_TRB_SIZE * idx);
}

static inline void
xhci_xfer_put_trb(struct xhci_xfer * const xx, u_int idx,
    uint64_t parameter, uint32_t status, uint32_t control)
{
	KASSERTMSG(idx < xx->xx_ntrb, "idx=%u xx_ntrb=%u", idx, xx->xx_ntrb);
	xx->xx_trb[idx].trb_0 = parameter;
	xx->xx_trb[idx].trb_2 = status;
	xx->xx_trb[idx].trb_3 = control;
}

static inline void
xhci_trb_put(struct xhci_trb * const trb, uint64_t parameter, uint32_t status,
    uint32_t control)
{
	trb->trb_0 = htole64(parameter);
	trb->trb_2 = htole32(status);
	trb->trb_3 = htole32(control);
}

static int
xhci_trb_get_idx(struct xhci_ring *xr, uint64_t trb_0, int *idx)
{
	/* base address of TRBs */
	bus_addr_t trbp = xhci_ring_trbp(xr, 0);

	/* trb_0 range sanity check */
	if (trb_0 == 0 || trb_0 < trbp ||
	    (trb_0 - trbp) % sizeof(struct xhci_trb) != 0 ||
	    (trb_0 - trbp) / sizeof(struct xhci_trb) >= xr->xr_ntrb) {
		return 1;
	}
	*idx = (trb_0 - trbp) / sizeof(struct xhci_trb);
	return 0;
}

static unsigned int
xhci_get_epstate(struct xhci_softc * const sc, struct xhci_slot * const xs,
    u_int dci)
{
	uint32_t *cp;

	usb_syncmem(&xs->xs_dc_dma, 0, sc->sc_pgsz, BUS_DMASYNC_POSTREAD);
	cp = xhci_slot_get_dcv(sc, xs, dci);
	return XHCI_EPCTX_0_EPSTATE_GET(le32toh(cp[0]));
}

static inline unsigned int
xhci_ctlrport2bus(struct xhci_softc * const sc, unsigned int ctlrport)
{
	const unsigned int port = ctlrport - 1;
	const uint8_t bit = __BIT(port % NBBY);

	return __SHIFTOUT(sc->sc_ctlrportbus[port / NBBY], bit);
}

/*
 * Return the roothub port for a controller port.  Both are 1..n.
 */
static inline unsigned int
xhci_ctlrport2rhport(struct xhci_softc * const sc, unsigned int ctrlport)
{

	return sc->sc_ctlrportmap[ctrlport - 1];
}

/*
 * Return the controller port for a bus roothub port.  Both are 1..n.
 */
static inline unsigned int
xhci_rhport2ctlrport(struct xhci_softc * const sc, unsigned int bn, unsigned int rhport)
{

	return sc->sc_rhportmap[bn][rhport - 1];
}

/* --- */

void
xhci_childdet(struct device *self, struct device *child)
{
	struct xhci_softc * const sc = device_private(self);

	KASSERT((sc->sc_child == child) || (sc->sc_child2 == child));
	if (child == sc->sc_child2)
		sc->sc_child2 = NULL;
	else if (child == sc->sc_child)
		sc->sc_child = NULL;
}

int
xhci_init(struct xhci_softc *sc)
{
	bus_size_t bsz;
	uint32_t hcs1, hcs2, hcs3, dboff, rtsoff;
	uint32_t pagesize, config;
	int i = 0;
	uint16_t hciversion;
	uint8_t caplength;
	XHCIHIST_FUNC(); XHCIHIST_CALLED();

	/* Set up the bus struct for the usb 3 and usb 2 buses */
	sc->sc_bus.ub_methods = &xhci_bus_methods;
	sc->sc_bus.ub_pipesize = sizeof(struct xhci_pipe);
	sc->sc_bus.ub_usedma = true;
	sc->sc_bus.ub_hcpriv = sc;

	sc->sc_bus2.ub_methods = &xhci_bus_methods;
	sc->sc_bus2.ub_pipesize = sizeof(struct xhci_pipe);
	sc->sc_bus2.ub_revision = USBREV_2_0;
	sc->sc_bus2.ub_usedma = true;
	sc->sc_bus2.ub_hcpriv = sc;
	sc->sc_bus2.ub_dmatag = sc->sc_bus.ub_dmatag;

	caplength = xhci_read_1(sc, XHCI_CAPLENGTH);
	hciversion = xhci_read_2(sc, XHCI_HCIVERSION);

	if (hciversion < XHCI_HCIVERSION_0_96 || hciversion >= 0x0200) {
		aprint_normal_dev(sc->sc_dev,
				"xHCI version %x.%x not known to be supported\n",
				(hciversion >> 8) & 0xff, (hciversion >> 0) & 0xff);
	} else {
		aprint_verbose_dev(sc->sc_dev, "xHCI version %x.%x\n",
				(hciversion >> 8) & 0xff, (hciversion >> 0) & 0xff);
	}

	if (bus_space_subregion(sc->sc_iot, sc->sc_ioh, 0, caplength, &sc->sc_cbh)
			!= 0) {
		aprint_error_dev(sc->sc_dev, "capability subregion failure\n");
		return ENOMEM;
	}

	hcs1 = xhci_cap_read_4(sc, XHCI_HCSPARAMS1);
	sc->sc_maxslots = XHCI_HCS1_MAXSLOTS(hcs1);
	sc->sc_maxintrs = XHCI_HCS1_MAXINTRS(hcs1);
	sc->sc_maxports = XHCI_HCS1_MAXPORTS(hcs1);
	hcs2 = xhci_cap_read_4(sc, XHCI_HCSPARAMS2);
	hcs3 = xhci_cap_read_4(sc, XHCI_HCSPARAMS3);
	aprint_debug_dev(sc->sc_dev, "hcs1=%"PRIx32" hcs2=%"PRIx32" hcs3=%"PRIx32"\n", hcs1, hcs2, hcs3);

	sc->sc_hcc = xhci_cap_read_4(sc, XHCI_HCCPARAMS);
	sc->sc_ctxsz = XHCI_HCC_CSZ(sc->sc_hcc) ? 64 : 32;

	char sbuf[128];
	if (hciversion < XHCI_HCIVERSION_1_0)
		snprintb(sbuf, sizeof(sbuf), XHCI_HCCPREV1_BITS, sc->sc_hcc);
	else
		snprintb(sbuf, sizeof(sbuf), XHCI_HCCV1_x_BITS, sc->sc_hcc);
	aprint_debug_dev(sc->sc_dev, "hcc=%s\n", sbuf);
	aprint_debug_dev(sc->sc_dev, "xECP %" __PRIxBITS "\n",
			XHCI_HCC_XECP(sc->sc_hcc) * 4);
	if (hciversion >= XHCI_HCIVERSION_1_1) {
		sc->sc_hcc2 = xhci_cap_read_4(sc, XHCI_HCCPARAMS2);
		snprintb(sbuf, sizeof(sbuf), XHCI_HCC2_BITS, sc->sc_hcc2);
		aprint_debug_dev(sc->sc_dev, "hcc2=%s\n", sbuf);
	}

	/* default all ports to bus 0, i.e. usb 3 */
	sc->sc_ctlrportbus = kmem_zalloc(
			howmany(sc->sc_maxports * sizeof(uint8_t), NBBY), KM_SLEEP);
	sc->sc_ctlrportmap = kmem_zalloc(sc->sc_maxports * sizeof(int), KM_SLEEP);

	/* controller port to bus roothub port map */
	for (size_t j = 0; j < __arraycount(sc->sc_rhportmap); j++) {
		sc->sc_rhportmap[j] = kmem_zalloc(sc->sc_maxports * sizeof(int),
				KM_SLEEP);
	}

	/*
	 * Process all Extended Capabilities
	 */
	xhci_ecp(sc);

	bsz = XHCI_PORTSC(sc->sc_maxports);
	if (bus_space_subregion(sc->sc_iot, sc->sc_ioh, caplength, bsz, &sc->sc_obh)
			!= 0) {
		aprint_error_dev(sc->sc_dev, "operational subregion failure\n");
		return ENOMEM;
	}

	dboff = xhci_cap_read_4(sc, XHCI_DBOFF);
	if (bus_space_subregion(sc->sc_iot, sc->sc_ioh, dboff, sc->sc_maxslots * 4,
			&sc->sc_dbh) != 0) {
		aprint_error_dev(sc->sc_dev, "doorbell subregion failure\n");
		return ENOMEM;
	}

	rtsoff = xhci_cap_read_4(sc, XHCI_RTSOFF);
	if (bus_space_subregion(sc->sc_iot, sc->sc_ioh, rtsoff,
			sc->sc_maxintrs * 0x20, &sc->sc_rbh) != 0) {
		aprint_error_dev(sc->sc_dev, "runtime subregion failure\n");
		return ENOMEM;
	}

	int rv;
	rv = xhci_hc_reset(sc);
	if (rv != 0) {
		return rv;
	}

	if (sc->sc_vendor_init)
		sc->sc_vendor_init(sc);

	pagesize = xhci_op_read_4(sc, XHCI_PAGESIZE);
	aprint_debug_dev(sc->sc_dev, "PAGESIZE 0x%08x\n", pagesize);
	pagesize = ffs(pagesize);
	if (pagesize == 0) {
		aprint_error_dev(sc->sc_dev, "pagesize is 0\n");
		return EIO;
	}
	sc->sc_pgsz = 1 << (12 + (pagesize - 1));
	aprint_debug_dev(sc->sc_dev, "sc_pgsz 0x%08x\n", (uint32_t) sc->sc_pgsz);
	aprint_debug_dev(sc->sc_dev, "sc_maxslots 0x%08x\n",
			(uint32_t) sc->sc_maxslots);
	aprint_debug_dev(sc->sc_dev, "sc_maxports %d\n", sc->sc_maxports);

	int err;
	sc->sc_maxspbuf = XHCI_HCS2_MAXSPBUF(hcs2);
	aprint_debug_dev(sc->sc_dev, "sc_maxspbuf %d\n", sc->sc_maxspbuf);
	if (sc->sc_maxspbuf != 0) {
		err = usb_allocmem(&sc->sc_bus, sizeof(uint64_t) * sc->sc_maxspbuf,
				sizeof(uint64_t), USBMALLOC_COHERENT | USBMALLOC_ZERO,
				&sc->sc_spbufarray_dma);
		if (err) {
			aprint_error_dev(sc->sc_dev, "spbufarray init fail, err %d\n", err);
			return ENOMEM;
		}

		sc->sc_spbuf_dma = kmem_zalloc(
				sizeof(*sc->sc_spbuf_dma) * sc->sc_maxspbuf, KM_SLEEP);
		uint64_t *spbufarray = KERNADDR(&sc->sc_spbufarray_dma, 0);
		for (i = 0; i < sc->sc_maxspbuf; i++) {
			usb_dma_t *const dma = &sc->sc_spbuf_dma[i];
			/* allocate contexts */
			err = usb_allocmem(&sc->sc_bus, sc->sc_pgsz, sc->sc_pgsz,
					USBMALLOC_COHERENT | USBMALLOC_ZERO, dma);
			if (err) {
				aprint_error_dev(sc->sc_dev,
						"spbufarray_dma init fail, err %d\n", err);
				rv = ENOMEM;
				goto bad1;
			}
			spbufarray[i] = htole64(DMAADDR(dma, 0));
			usb_syncmem(dma, 0, sc->sc_pgsz,
					BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
		}

		usb_syncmem(&sc->sc_spbufarray_dma, 0,
				sizeof(uint64_t) * sc->sc_maxspbuf, BUS_DMASYNC_PREWRITE);
	}

	config = xhci_op_read_4(sc, XHCI_CONFIG);
	config &= ~0xFF;
	config |= sc->sc_maxslots & 0xFF;
	xhci_op_write_4(sc, XHCI_CONFIG, config);

	err = xhci_ring_init(sc, &sc->sc_cr, XHCI_COMMAND_RING_TRBS,
	XHCI_COMMAND_RING_SEGMENTS_ALIGN);
	if (err) {
		aprint_error_dev(sc->sc_dev, "command ring init fail, err %d\n", err);
		rv = ENOMEM;
		goto bad1;
	}

	err = xhci_ring_init(sc, &sc->sc_er, XHCI_EVENT_RING_TRBS,
	XHCI_EVENT_RING_SEGMENTS_ALIGN);
	if (err) {
		aprint_error_dev(sc->sc_dev, "event ring init fail, err %d\n", err);
		rv = ENOMEM;
		goto bad2;
	}

	usb_dma_t *dma;
	size_t size;
	size_t align;

	dma = &sc->sc_eventst_dma;
	size = roundup2(XHCI_EVENT_RING_SEGMENTS * XHCI_ERSTE_SIZE,
	XHCI_EVENT_RING_SEGMENT_TABLE_ALIGN);
	KASSERTMSG(size <= (512 * 1024), "eventst size %zu too large", size);
	align = XHCI_EVENT_RING_SEGMENT_TABLE_ALIGN;
	err = usb_allocmem(&sc->sc_bus, size, align,
			USBMALLOC_COHERENT | USBMALLOC_ZERO, dma);
	if (err) {
		aprint_error_dev(sc->sc_dev, "eventst init fail, err %d\n", err);
		rv = ENOMEM;
		goto bad3;
	}

	aprint_debug_dev(sc->sc_dev, "eventst: 0x%016jx %p %zx\n",
			(uintmax_t) DMAADDR(&sc->sc_eventst_dma, 0),
			KERNADDR(&sc->sc_eventst_dma, 0),
			sc->sc_eventst_dma.udma_block->size);

	dma = &sc->sc_dcbaa_dma;
	size = (1 + sc->sc_maxslots) * sizeof(uint64_t);
	KASSERTMSG(size <= 2048, "dcbaa size %zu too large", size);
	align = XHCI_DEVICE_CONTEXT_BASE_ADDRESS_ARRAY_ALIGN;
	err = usb_allocmem(&sc->sc_bus, size, align,
			USBMALLOC_COHERENT | USBMALLOC_ZERO, dma);
	if (err) {
		aprint_error_dev(sc->sc_dev, "dcbaa init fail, err %d\n", err);
		rv = ENOMEM;
		goto bad4;
	}
	aprint_debug_dev(sc->sc_dev, "dcbaa: 0x%016jx %p %zx\n",
			(uintmax_t) DMAADDR(&sc->sc_dcbaa_dma, 0),
			KERNADDR(&sc->sc_dcbaa_dma, 0), sc->sc_dcbaa_dma.udma_block->size);

	if (sc->sc_maxspbuf != 0) {
		/*
		 * DCBA entry 0 hold the scratchbuf array pointer.
		 */
		*(uint64_t*) KERNADDR(dma, 0) = htole64(
				DMAADDR(&sc->sc_spbufarray_dma, 0));
		usb_syncmem(dma, 0, size, BUS_DMASYNC_PREWRITE);
	}

	sc->sc_slots = kmem_zalloc(sizeof(*sc->sc_slots) * sc->sc_maxslots,
			KM_SLEEP);
	if (sc->sc_slots == NULL) {
		aprint_error_dev(sc->sc_dev, "slots init fail, err %d\n", err);
		rv = ENOMEM;
		goto bad;
	}

	sc->sc_xferpool = pool_cache_init(sizeof(struct xhci_xfer), 0, 0, 0,
			"xhcixfer", NULL, IPL_USB, NULL, NULL, NULL);
	if (sc->sc_xferpool == NULL) {
		aprint_error_dev(sc->sc_dev, "pool_cache init fail, err %d\n", err);
		rv = ENOMEM;
		goto bad;
	}

	cv_init(&sc->sc_command_cv, "xhcicmd");
	cv_init(&sc->sc_cmdbusy_cv, "xhcicmdq");
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_SOFTUSB);
	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_USB);

	struct xhci_erste *erst;
	erst = KERNADDR(&sc->sc_eventst_dma, 0);
	erst[0].erste_0 = htole64(xhci_ring_trbp(sc->sc_er, 0));
	erst[0].erste_2 = htole32(sc->sc_er->xr_ntrb);
	erst[0].erste_3 = htole32(0);
	usb_syncmem(&sc->sc_eventst_dma, 0,
	XHCI_ERSTE_SIZE * XHCI_EVENT_RING_SEGMENTS, BUS_DMASYNC_PREWRITE);

	xhci_rt_write_4(sc, XHCI_ERSTSZ(0), XHCI_EVENT_RING_SEGMENTS);
	xhci_rt_write_8(sc, XHCI_ERSTBA(0), DMAADDR(&sc->sc_eventst_dma, 0));
	xhci_rt_write_8(sc, XHCI_ERDP(0), xhci_ring_trbp(sc->sc_er, 0) |
	XHCI_ERDP_BUSY);

	xhci_op_write_8(sc, XHCI_DCBAAP, DMAADDR(&sc->sc_dcbaa_dma, 0));
	xhci_op_write_8(sc, XHCI_CRCR,
			xhci_ring_trbp(sc->sc_cr, 0) | sc->sc_cr->xr_cs);

	xhci_barrier(sc, BUS_SPACE_BARRIER_WRITE);

	HEXDUMP("eventst", KERNADDR(&sc->sc_eventst_dma, 0),
			XHCI_ERSTE_SIZE * XHCI_EVENT_RING_SEGMENTS);

	if ((sc->sc_quirks & XHCI_DEFERRED_START) == 0)
		xhci_start(sc);

	return 0;

 bad:
	if (sc->sc_xferpool) {
		pool_cache_destroy(sc->sc_xferpool);
		sc->sc_xferpool = NULL;
	}

	if (sc->sc_slots) {
		kmem_free(sc->sc_slots, sizeof(*sc->sc_slots) * sc->sc_maxslots);
		sc->sc_slots = NULL;
	}

	usb_freemem(&sc->sc_bus, &sc->sc_dcbaa_dma);
 bad4:
	usb_freemem(&sc->sc_bus, &sc->sc_eventst_dma);
 bad3:
	xhci_ring_free(sc, &sc->sc_er);
 bad2:
	xhci_ring_free(sc, &sc->sc_cr);
	i = sc->sc_maxspbuf;
 bad1:
	for (int j = 0; j < i; j++)
		usb_freemem(&sc->sc_bus, &sc->sc_spbuf_dma[j]);
	usb_freemem(&sc->sc_bus, &sc->sc_spbufarray_dma);

	return rv;
}
