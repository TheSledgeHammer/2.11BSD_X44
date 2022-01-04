/*	$OpenBSD: usb_port.h,v 1.18 2000/09/06 22:42:10 rahnds Exp $ */
/*	$NetBSD: usb_port.h,v 1.62 2003/02/15 18:33:30 augustss Exp $	*/
/*	$FreeBSD: src/sys/dev/usb/usb_port.h,v 1.21 1999/11/17 22:33:47 n_hibma Exp $	*/

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at
 * Carlstedt Research & Technology.
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

#ifndef _USB_PORT_H
#define _USB_PORT_H

#include <sys/callout.h>

#define USB_USE_SOFTINTR

#ifdef USB_DEBUG
#define UKBD_DEBUG 		1
#define UHIDEV_DEBUG 	1
#define UHID_DEBUG 		1
#define OHCI_DEBUG 		1
#define UGEN_DEBUG 		1
#define UHCI_DEBUG 		1
#define UHUB_DEBUG 		1
#define ULPT_DEBUG 		1
#define UCOM_DEBUG 		1
#define UPLCOM_DEBUG 	1
#define UMCT_DEBUG 		1
#define UMODEM_DEBUG 	1
#define UAUDIO_DEBUG 	1
#define AUE_DEBUG 		1
#define CUE_DEBUG 		1
#define KUE_DEBUG 		1
#define URL_DEBUG 		1
#define UMASS_DEBUG 	1
#define UVISOR_DEBUG 	1
#define UPL_DEBUG 		1
#define UZCOM_DEBUG 	1
#define URIO_DEBUG 		1
#define UFTDI_DEBUG 	1
#define USCANNER_DEBUG 	1
#define USSCANNER_DEBUG 1
#define EHCI_DEBUG 		1
#define UIRDA_DEBUG 	1
#define USTIR_DEBUG 	1
#define UISDATA_DEBUG 	1
#define UDSBR_DEBUG 	1
#define UBT_DEBUG 		1
#define UAX_DEBUG 		1
#endif

#define SCSI_MODE_SENSE			MODE_SENSE

#define USBBASEDEVICE 			struct device
#define USBDEV(bdev) 			(&(bdev))
#define USBDEVNAME(bdev) 		((bdev).dv_xname)
#define USBDEVUNIT(bdev) 		((bdev).dv_unit)
#define USBDEVPTRNAME(bdevptr) 	((bdevptr)->dv_xname)
#define USBGETSOFTC(d) 			((void *)(d))

typedef char usb_callout_t;
#define usb_callout_init(h)		callout_init(&(h))
#define usb_callout(h, t, f, d) callout_reset(&(h), (t), (f), (d))
#define usb_uncallout(h, f, d) 	callout_stop(&(h))

#endif /* _USB_PORT_H */
