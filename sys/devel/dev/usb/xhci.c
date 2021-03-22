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
#include <sys/bus.h>
#include <sys/sysctl.h>

#include <machine/endian_machdep.h>

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

#define XHCI_DCI_SLOT 0
#define XHCI_DCI_EP_CONTROL 1

#define XHCI_ICI_INPUT_CONTROL 0

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

static usbd_status xhci_open(struct usbd_pipe *);
static void xhci_close_pipe(struct usbd_pipe *);
static int xhci_intr1(struct xhci_softc * const);
static void xhci_softintr(void *);
static void xhci_poll(struct usbd_bus *);
static struct usbd_xfer *xhci_allocx(struct usbd_bus *, unsigned int);
static void xhci_freex(struct usbd_bus *, struct usbd_xfer *);
static void xhci_abortx(struct usbd_xfer *);
static bool xhci_dying(struct usbd_bus *);
static void xhci_get_lock(struct usbd_bus *, struct lock_object **);
static usbd_status xhci_new_device(struct device *, struct usbd_bus *, int, int, int, struct usbd_port *);
static int xhci_roothub_ctrl(struct usbd_bus *, usb_device_request_t *, void *, int);

static usbd_status xhci_configure_endpoint(struct usbd_pipe *);
//static usbd_status xhci_unconfigure_endpoint(struct usbd_pipe *);
static usbd_status xhci_reset_endpoint(struct usbd_pipe *);
static usbd_status xhci_stop_endpoint(struct usbd_pipe *);

static void xhci_host_dequeue(struct xhci_ring * const);
static usbd_status xhci_set_dequeue(struct usbd_pipe *);

static usbd_status xhci_do_command(struct xhci_softc * const,
    struct xhci_soft_trb * const, int);
static usbd_status xhci_do_command_locked(struct xhci_softc * const, struct xhci_soft_trb * const, int);
static usbd_status xhci_init_slot(struct usbd_device *, uint32_t);
static void xhci_free_slot(struct xhci_softc *, struct xhci_slot *);
static usbd_status xhci_set_address(struct usbd_device *, uint32_t, bool);
static usbd_status xhci_enable_slot(struct xhci_softc * const, uint8_t * const);
static usbd_status xhci_disable_slot(struct xhci_softc * const, uint8_t);
static usbd_status xhci_address_device(struct xhci_softc * const, uint64_t, uint8_t, bool);
static void xhci_set_dcba(struct xhci_softc * const, uint64_t, int);
static usbd_status xhci_update_ep0_mps(struct xhci_softc * const, struct xhci_slot * const, u_int);
static usbd_status xhci_ring_init(struct xhci_softc * const, struct xhci_ring **, size_t, size_t);
static void xhci_ring_free(struct xhci_softc * const, struct xhci_ring ** const);

static void xhci_setup_ctx(struct usbd_pipe *);
static void xhci_setup_route(struct usbd_pipe *, uint32_t *);
static void xhci_setup_tthub(struct usbd_pipe *, uint32_t *);
static void xhci_setup_maxburst(struct usbd_pipe *, uint32_t *);
static uint32_t xhci_bival2ival(uint32_t, uint32_t);

static void xhci_noop(struct usbd_pipe *);

static usbd_status xhci_root_intr_transfer(struct usbd_xfer *);
static usbd_status xhci_root_intr_start(struct usbd_xfer *);
static void xhci_root_intr_abort(struct usbd_xfer *);
static void xhci_root_intr_close(struct usbd_pipe *);
static void xhci_root_intr_done(struct usbd_xfer *);

static usbd_status xhci_device_ctrl_transfer(struct usbd_xfer *);
static usbd_status xhci_device_ctrl_start(struct usbd_xfer *);
static void xhci_device_ctrl_abort(struct usbd_xfer *);
static void xhci_device_ctrl_close(struct usbd_pipe *);
static void xhci_device_ctrl_done(struct usbd_xfer *);

static usbd_status xhci_device_isoc_transfer(struct usbd_xfer *);
static usbd_status xhci_device_isoc_enter(struct usbd_xfer *);
static void xhci_device_isoc_abort(struct usbd_xfer *);
static void xhci_device_isoc_close(struct usbd_pipe *);
static void xhci_device_isoc_done(struct usbd_xfer *);

static usbd_status xhci_device_intr_transfer(struct usbd_xfer *);
static usbd_status xhci_device_intr_start(struct usbd_xfer *);
static void xhci_device_intr_abort(struct usbd_xfer *);
static void xhci_device_intr_close(struct usbd_pipe *);
static void xhci_device_intr_done(struct usbd_xfer *);

static usbd_status xhci_device_bulk_transfer(struct usbd_xfer *);
static usbd_status xhci_device_bulk_start(struct usbd_xfer *);
static void xhci_device_bulk_abort(struct usbd_xfer *);
static void xhci_device_bulk_close(struct usbd_pipe *);
static void xhci_device_bulk_done(struct usbd_xfer *);

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

}
