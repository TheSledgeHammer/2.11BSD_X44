/*	$NetBSD: comvar.h,v 1.5 1996/05/05 19:50:47 christos Exp $	*/

/*
 * Copyright (c) 1996 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
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

#include <sys/callout.h>
#include <sys/timepps.h>
#include <sys/lock.h>

int comcnattach(bus_space_tag_t, bus_addr_t, int, int, int, tcflag_t);
#ifdef KGDB
int com_kgdb_attach(bus_space_tag_t, bus_addr_t, int, int, int, tcflag_t);
#endif
int com_is_console(bus_space_tag_t, bus_addr_t, bus_space_handle_t *);

/* Hardware flag masks */
#define	COM_HW_NOIEN			0x01
#define	COM_HW_FIFO				0x02
#define	COM_HW_HAYESP			0x04
#define	COM_HW_FLOW				0x08
#define	COM_HW_DEV_OK			0x20
#define	COM_HW_CONSOLE			0x40
#define	COM_HW_KGDB				0x80

#define	COM_HW_TXFIFO_DISABLE	0x100
#define	COM_HW_NO_TXPRELOAD		0x200
#define	COM_HW_AFE				0x400
#define	COM_HW_POLL				0x800
/* Software flag masks */
#define	COM_SW_SOFTCAR			0x01
#define	COM_SW_CLOCAL			0x02
#define	COM_SW_CRTSCTS			0x04
#define	COM_SW_MDMBUF			0x08

/* Buffer size for character buffer */
#ifndef COM_RING_SIZE
#define	COM_RING_SIZE			2048
#endif

#define	COM_REGMAP_NENTRIES		19

#define	COM_IBUFSIZE			(32 * 512)
#define	COM_IHIGHWATER			((3 * COM_IBUFSIZE) / 4)

struct com_softc {
	struct device 		sc_dev;
	void 				*sc_si;
	struct tty 			*sc_tty;

	struct callout 		sc_diag_callout;

	int 				sc_iobase;
	int 				sc_frequency;

	bus_space_tag_t		sc_iot;
	bus_space_handle_t 	sc_ioh;
	bus_space_handle_t  sc_hayespioh;

	int 				sc_overflows;
	int 				sc_floods;
	int 				sc_errors;

	u_char 				sc_hwflags;
	u_char 				sc_swflags;
	u_int 				sc_fifolen;

	u_int 				sc_r_hiwat,
						sc_r_lowat;

	u_char *volatile	sc_rbget,
	       *volatile 	sc_rbput;
 	volatile u_int 		sc_rbavail;
	u_char 				*sc_rbuf,
	       	   	   	   	*sc_ebuf;

 	u_char 				*sc_tba;
 	u_int 				sc_tbc,
	      	  	  	  	sc_heldtbc;

	volatile u_char 	sc_rx_flags,
#define	RX_TTY_BLOCKED			0x01
#define	RX_TTY_OVERFLOWED		0x02
#define	RX_IBUF_BLOCKED			0x04
#define	RX_IBUF_OVERFLOWED		0x08
#define	RX_ANY_BLOCK			0x0f
						sc_tx_busy,
						sc_tx_done,
						sc_tx_stopped,
						sc_st_check,
						sc_rx_ready;

	volatile u_char 	sc_heldchange;
	volatile u_char 	sc_msr, sc_msr_delta, sc_msr_mask, sc_mcr, sc_mcr_active, sc_lcr, sc_ier, sc_fifo, sc_dlbl, sc_dlbh, sc_efr;
	u_char 				sc_mcr_dtr, sc_mcr_rts, sc_msr_cts, sc_msr_dcd;
#ifdef COM_HAYESP
	u_char 				sc_prescaler;
#endif
	int 				sc_type;
#define	COM_TYPE_NORMAL		0	/* normal 16x50 */
#define	COM_TYPE_HAYESP		1	/* Hayes ESP modem */
#define	COM_TYPE_PXA2x0		2	/* Intel PXA2x0 processor built-in */
#define	COM_TYPE_AU1x00		3	/* AMD/Alchemy Au1x000 proc. built-in */

	/* power management hooks */
	int 				(*enable)(struct com_softc *);
	void 				(*disable)(struct com_softc *);
	int 				enabled;

	/* PPS signal on DCD, with or without inkernel clock disciplining */
	u_char				sc_ppsmask;			/* pps signal mask */
	u_char				sc_ppsassert;		/* pps leading edge */
	u_char				sc_ppsclear;		/* pps trailing edge */
	pps_info_t 			ppsinfo;
	pps_params_t 		ppsparam;

	struct lock_object	sc_lock;
};

/* Macros to clear/set/test flags. */
//#define SET(t, f)	(t) |= (f)
//#define CLR(t, f)	(t) &= ~(f)
//#define ISSET(t, f)	((t) & (f))

int 	comprobe1(bus_space_tag_t, bus_space_handle_t);
int 	comintr(void *);
void 	com_attach_subr(struct com_softc *);
int 	cominit(bus_space_tag_t, bus_addr_t, int, int, int, tcflag_t, bus_space_handle_t *);
int 	com_detach(struct device *, int);
int 	com_activate(struct device *, enum devact);

#ifndef __GENERIC_SOFT_INTERRUPTS
#ifdef __NO_SOFT_SERIAL_INTERRUPT
#define	IPL_SERIAL		IPL_TTY
#define	splserial()		spltty()
#define	IPL_SOFTSERIAL	IPL_TTY
#define	splsoftserial()	spltty()
#endif
#endif
