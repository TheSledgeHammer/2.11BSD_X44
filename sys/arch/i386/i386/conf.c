/*
 * The 3-Clause BSD License:
 * Copyright (c) 2020 Martin Kelly
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

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/devsw.h>
#include <sys/tty.h>
#include <sys/null.h>

#include <dev/misc/cons/cons.h>

#include "audio.h"
#include "sequencer.h"
#include "spkr.h"
#include "midi.h"

#include "wd.h"
#include "sd.h"
#include "st.h"
#include "cd.h"
#include "uk.h"
#include "ch.h"
#include "ss.h"
#include "ses.h"
#include "vnd.h"
#include "ccd.h"

#include "ksyms.h"
#include "cmos.h"
/*
#include "bpfilter.h"

#include "usb.h"
#include "uhid.h"
#include "ugen.h"
#include "ucom.h"
*/
#include "com.h"
#include "pty.h"
#include "tb.h"
#include "video.h"

#include "wsdisplay.h"
#include "wskbd.h"
#include "wsmouse.h"
#include "wsmux.h"
#include "wsfont.h"

#include "scsibus.h"
#include "pci.h"

/* devsw switch table */
const struct bdevsw *bdevsw0;
const struct cdevsw *cdevsw0;
const struct linesw *linesw0;
const struct bdevsw **bdevsw = &bdevsw0;
const struct cdevsw **cdevsw = &cdevsw0;
const struct linesw **linesw = &linesw0;
const int sys_bdevsws = __arraycount(bdevsw0);
const int sys_cdevsws = __arraycount(cdevsw0);
const int sys_linesws = __arraycount(linesw0);
int max_bdevsws = __arraycount(bdevsw0);
int max_cdevsws = __arraycount(cdevsw0);
int max_linesws = __arraycount(linesw0);

int	nblkdev = sys_bdevsws;
int	nchrdev = sys_cdevsws;

//extern struct consdev wsdisplay_cons;
//extern struct consdev com_cons;

/* constab */
struct consdev constab[] = {
#if NWSDISPLAY > 0
		{ &wsdisplay_cons },
#endif
#if NCOM > 0
		{ &com_cons },
#endif
		{ 0 }
};

/* bdevsw */
const struct bdevsw swap_bdevsw;
const struct bdevsw wd_bdevsw;
const struct bdevsw sd_bdevsw;
const struct bdevsw st_bdevsw;
const struct bdevsw cd_bdevsw;
const struct bdevsw vnd_bdevsw;
const struct bdevsw ccd_bdevsw;

/* cdevsw */
const struct cdevsw log_cdevsw;
const struct cdevsw swap_cdevsw;
const struct cdevsw ctty_cdevsw;
const struct cdevsw ptc_cdevsw;
const struct cdevsw pts_cdevsw;
const struct cdevsw cons_cdevsw;
const struct cdevsw audio_cdevsw;
const struct cdevsw midi_cdevsw;
const struct cdevsw sequencer_cdevsw;
const struct cdevsw spkr_cdevsw;
const struct cdevsw pci_cdevsw;
const struct cdevsw com_cdevsw;
const struct cdevsw lpt_cdevsw;
const struct cdevsw wd_cdevsw;
const struct cdevsw sd_cdevsw;
const struct cdevsw st_cdevsw;
const struct cdevsw cd_cdevsw;
const struct cdevsw ch_cdevsw;
const struct cdevsw uk_cdevsw;
const struct cdevsw ss_cdevsw;
const struct cdevsw ses_cdevsw;
const struct cdevsw vnd_cdevsw;
const struct cdevsw ccd_cdevsw;
const struct cdevsw apm_cdevsw;
const struct cdevsw cmos_cdevsw;
const struct cdevsw ksyms_cdevsw;
const struct cdevsw bpf_cdevsw;
const struct cdevsw video_cdevsw;
const struct cdevsw wsdisplay_cdevsw;
const struct cdevsw wskbd_cdevsw;
const struct cdevsw wsmouse_cdevsw;
const struct cdevsw wsmux_cdevsw;
const struct cdevsw wsfont_cdevsw;
const struct cdevsw mm_cdevsw;

/* linesw */
const struct linesw ttydisc;
const struct linesw nttydisc;
const struct linesw ottydisc;
const struct linesw netldisc;
const struct linesw tabldisc;
const struct linesw slipdisc;
const struct linesw pppdisc;

void kernel_init(struct devswtable *);
void device_init(struct devswtable *);
void audio_init(struct devswtable *);
void core_init(struct devswtable *);
void disks_init(struct devswtable *);
void misc_init(struct devswtable *);
void usb_init(struct devswtable *);
void video_init(struct devswtable *);
void wscons_init(struct devswtable *);
void network_init(struct devswtable *);

/*
 * Configure Initialization
 */
void
conf_init(devsw)
	struct devswtable *devsw;
{
	device_init(devsw);			/* device interfaces */
	kernel_init(devsw);			/* kernel interfaces */
	network_init(devsw);		/* network interfaces */
}

/* Add kernel driver configuration */
void
kernel_init(devsw)
	struct devswtable *devsw;
{
	DEVSWIO_CONFIG_INIT(devsw, 1, NULL, &log_cdevsw, NULL);			        /* log interfaces */
	DEVSWIO_CONFIG_INIT(devsw, 1, &swap_bdevsw, &swap_cdevsw, NULL);		/* swap interfaces */
	DEVSWIO_CONFIG_INIT(devsw, 0, NULL, NULL, &ttydisc);					/* 0- TTYDISC */
	DEVSWIO_CONFIG_INIT(devsw, 0, NULL, NULL, &nttydisc);					/* 1- NTTYDISC */
	DEVSWIO_CONFIG_INIT(devsw, 0, NULL, NULL, &ottydisc);					/* 2- OTTYDISC */
//	DEVSWIO_CONFIG_INIT(devsw, NBK, NULL, NULL, &netldisc);					/* 3- NETLDISC */
	DEVSWIO_CONFIG_INIT(devsw, NTB, NULL, NULL, &tabldisc);					/* 4- TABLDISC */
//	DEVSWIO_CONFIG_INIT(devsw, NSL, NULL, NULL, &slipdisc);					/* 5- SLIPDISC */
//	DEVSWIO_CONFIG_INIT(devsw, 0, NULL, NULL, &pppdisc);					/* 6- PPPDISC */
	DEVSWIO_CONFIG_INIT(devsw, 1, NULL, &cons_cdevsw, NULL);				/* virtual console */
	DEVSWIO_CONFIG_INIT(devsw, 1, NULL, &ctty_cdevsw, NULL);				/* ctty controlling terminal */
	DEVSWIO_CONFIG_INIT(devsw, NPTY, NULL, &ptc_cdevsw, NULL);				/* ptc pseudo-tty slave, pseudo-tty master  */
	DEVSWIO_CONFIG_INIT(devsw, NPTY, NULL, &pts_cdevsw, NULL);				/* pts pseudo-tty slave, pseudo-tty master  */
}

/* Add device driver configuration */
void
device_init(devsw)
	struct devswtable *devsw;
{
	core_init(devsw);			/* core interfaces */
	wscons_init(devsw);			/* wscons & pccons interfaces */
	video_init(devsw);			/* video interfaces */
	misc_init(devsw);			/* misc (ksyms) interfaces */
	disks_init(devsw);			/* disk interfaces */
	audio_init(devsw);			/* audio interfaces */
	usb_init(devsw);			/* usb interfaces */
}

/* Add audio driver configuration */
void
audio_init(devsw)
	struct devswtable *devsw;
{
	DEVSWIO_CONFIG_INIT(devsw, NAUDIO, NULL, &audio_cdevsw, NULL);			/* generic audio I/O */
	DEVSWIO_CONFIG_INIT(devsw, NMIDI, NULL, &midi_cdevsw, NULL);			/* MIDI I/O */
	DEVSWIO_CONFIG_INIT(devsw, NSEQUENCER, NULL, &sequencer_cdevsw, NULL);	/* MIDI Sequencer I/O */
	DEVSWIO_CONFIG_INIT(devsw, NSPKR, NULL, &spkr_cdevsw, NULL);			/* PC Speaker */
}

/* Add core driver configuration */
void
core_init(devsw)
	struct devswtable *devsw;
{
	DEVSWIO_CONFIG_INIT(devsw, NPCI, NULL, &pci_cdevsw, NULL);				/* PCI user  */
	DEVSWIO_CONFIG_INIT(devsw, NCOM, NULL, &com_cdevsw, NULL);				/* Serial port */
//	DEVSWIO_CONFIG_INIT(devsw, NLPT, NULL, &lpt_cdevsw, NULL);				/* parallel printer */
}

/* Add disk driver configuration */
void
disks_init(devsw)
	struct devswtable *devsw;
{
	/* ATA Devices */
	DEVSWIO_CONFIG_INIT(devsw, NWD, &wd_bdevsw, &wd_cdevsw, NULL);  		/* ATA: ST506/ESDI/IDE disk */

	/* SCSI Devices */
	DEVSWIO_CONFIG_INIT(devsw, NSD, &sd_bdevsw, &sd_cdevsw, NULL);			/* SCSI disk */
	DEVSWIO_CONFIG_INIT(devsw, NST, &st_bdevsw, &st_cdevsw, NULL);			/* SCSI tape */
	DEVSWIO_CONFIG_INIT(devsw, NCD, &cd_bdevsw, &cd_cdevsw, NULL);			/* SCSI CD-ROM */
	DEVSWIO_CONFIG_INIT(devsw, NCH, NULL, &ch_cdevsw, NULL);				/* SCSI autochanger */
	DEVSWIO_CONFIG_INIT(devsw, NUK, NULL, &uk_cdevsw, NULL);				/* SCSI unknown  */
	DEVSWIO_CONFIG_INIT(devsw, NSS, NULL, &ss_cdevsw, NULL);				/* SCSI scanner */
	DEVSWIO_CONFIG_INIT(devsw, NSES, NULL, &ses_cdevsw, NULL);				/* SCSI ses */

	/* Pseudo Devices */
	DEVSWIO_CONFIG_INIT(devsw, NVND, &vnd_bdevsw, &vnd_cdevsw, NULL);		/* vnode disk driver */
	DEVSWIO_CONFIG_INIT(devsw, NCCD, &ccd_bdevsw, &ccd_cdevsw, NULL);		/* "Concatenated" disk driver */
}

/* Add miscellaneous driver configuration */
void
misc_init(devsw)
	struct devswtable *devsw;
{
	DEVSWIO_CONFIG_INIT(devsw, 1, NULL, &apm_cdevsw, NULL);					/* Power Management (APM) Interface */
	DEVSWIO_CONFIG_INIT(devsw, 1, NULL, &cmos_cdevsw, NULL);				/* CMOS Interface */
	DEVSWIO_CONFIG_INIT(devsw, 1, NULL, &mm_cdevsw, NULL);					/* /dev/{null,mem,kmem,...} */
	DEVSWIO_CONFIG_INIT(devsw, NKSYMS, NULL, &ksyms_cdevsw, NULL);			/* Kernel symbols device */
}

/* Add network driver configuration */
void
network_init(devsw)
	struct devswtable *devsw;
{
//	DEVSWIO_CONFIG_INIT(devsw, NBPFILTER, NULL, &bpf_cdevsw, NULL);			/* Berkeley packet filter */
}

/* Add usb driver configuration */
void
usb_init(devsw)
	struct devswtable *devsw;
{
	//DEVSWIO_CONFIG_INIT(devsw, NUSB, NULL, &usb_cdevsw, NULL);			/* USB controller */
	//DEVSWIO_CONFIG_INIT(devsw, NUHID, NULL, &uhid_cdevsw, NULL);			/* USB generic HID */
	//DEVSWIO_CONFIG_INIT(devsw, NUGEN, NULL, &ugen_cdevsw, NULL);			/* USB generic driver */
	//DEVSWIO_CONFIG_INIT(devsw, NUCOM, NULL, &ucom_cdevsw, NULL);			/* USB tty */
}

/* Add video driver configuration */
void
video_init(devsw)
	struct devswtable *devsw;
{
	DEVSWIO_CONFIG_INIT(devsw, NVIDEO , NULL, &video_cdevsw, NULL);			/* generic video I/O */
}

/* Add wscon driver configuration */
void
wscons_init(devsw)
	struct devswtable *devsw;
{
	DEVSWIO_CONFIG_INIT(devsw, NWSDISPLAY, NULL, &wsdisplay_cdevsw, NULL);	/* Wscons Display */
	DEVSWIO_CONFIG_INIT(devsw, NWSKBD, NULL, &wskbd_cdevsw, NULL);			/* Wscons Keyboard */
	DEVSWIO_CONFIG_INIT(devsw, NWSMOUSE, NULL, &wsmouse_cdevsw, NULL);		/* Wscons Mouse */
	DEVSWIO_CONFIG_INIT(devsw, NWSMUX, NULL, &wsmux_cdevsw, NULL);			/* Wscons Multiplexor */
	DEVSWIO_CONFIG_INIT(devsw, NWSFONT, NULL, &wsfont_cdevsw, NULL);		/* Wsfont */
}
