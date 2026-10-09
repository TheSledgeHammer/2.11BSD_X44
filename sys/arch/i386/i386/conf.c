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

//#include <dev/misc/cons/cons.h>

#include "audio.h"
#include "sequencer.h"
#include "midi.h"
#include "spkr.h"

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
#include "md.h"
#include "fdc.h"

#include "rnd.h"
#include "ksyms.h"
#include "cmos.h"

#include "bpfilter.h"
#include "tb.h"
#include "sl.h"
#include "ppp.h"
#include "strip.h"
#include "tun.h"

/*
#include "usb.h"
#include "uhid.h"
#include "ugen.h"
#include "ucom.h"
*/
#include "lpt.h"
#include "com.h"
#include "pty.h"
#include "video.h"
#include "agp.h"

#include "evdev.h"
#include "wsdisplay.h"
#include "wskbd.h"
#include "wsmouse.h"
#include "wsmux.h"
#include "wsfont.h"

#include "opencrypto.h"

#include "scsibus.h"
#include "pci.h"

/* devsw init (i.e. bdevsw, cdevsw, linesw) */
/* devsw switch table */
const struct bdevsw *bdevsw0[] = {
		bdevsw_init(NWD, wd_bdevsw),				/* ATA: ST506/ESDI/IDE disk */
		bdevsw_init(1, swap_bdevsw),				/* swap interfaces */
		bdevsw_init(NFDC, fd_bdevsw),				/* floppy diskette */
		bdevsw_init(NSD, sd_bdevsw),				/* SCSI disk */
		bdevsw_init(NST, st_bdevsw),				/* SCSI tape */
		bdevsw_init(NCD, cd_bdevsw),				/* SCSI CD-ROM */
		bdevsw_init(NVND, vnd_bdevsw),				/* vnode disk driver */
		bdevsw_init(NCCD, ccd_bdevsw),				/* "Concatenated" disk driver */
		bdevsw_init(NMD, md_bdevsw),				/* memory disk driver */
};

const struct cdevsw *cdevsw0[] = {
		cdevsw_init(1, cons_cdevsw),				/* virtual console */
		cdevsw_init(1, ctty_cdevsw),				/* ctty controlling terminal */
		cdevsw_init(1, mm_cdevsw),					/* /dev/{null,mem,kmem,...} */
		cdevsw_init(NWD, wd_cdevsw),				/* ATA: ST506/ESDI/IDE disk */
		cdevsw_init(1, swap_cdevsw),				/* swap interfaces */
		cdevsw_init(NPTY, pts_cdevsw),				/* pts pseudo-tty slave, pseudo-tty master  */
		cdevsw_init(NPTY, ptc_cdevsw),				/* ptc pseudo-tty slave, pseudo-tty master  */
		cdevsw_init(1, log_cdevsw),				  	/* log interfaces */
		cdevsw_init(NCOM, com_cdevsw),				/* Serial port */
		cdevsw_init(NFDC, fd_cdevsw),				/* floppy diskette */
		cdevsw_init(NSD, sd_cdevsw),				/* SCSI disk */
		cdevsw_init(NST, st_cdevsw),				/* SCSI tape */
		cdevsw_init(NCD, cd_cdevsw),				/* SCSI CD-ROM */
		cdevsw_init(NLPT, lpt_cdevsw),				/* parallel printer */
		cdevsw_init(NCH, ch_cdevsw),				/* SCSI autochanger */
		cdevsw_init(NCCD, ccd_cdevsw),				/* "Concatenated" disk driver */
		cdevsw_init(NSS, ss_cdevsw),				/* SCSI scanner */
		cdevsw_init(NUK, uk_cdevsw),				/* SCSI unknown  */
		cdevsw_init(NBPFILTER, bpf_cdevsw),			/* Berkeley packet filter */
		cdevsw_init(NMD, md_cdevsw),				/* memory disk driver */
		cdevsw_init(NSPKR, spkr_cdevsw),			/* PC Speaker */
		cdevsw_init(NTUN, tun_cdevsw),				/* network tunnel */
		cdevsw_init(NVND, vnd_cdevsw),				/* vnode disk driver */
		cdevsw_init(NAUDIO, audio_cdevsw),			/* generic audio I/O */
		cdevsw_init(NRND, rnd_cdevsw),				/* Random device */
		cdevsw_init(NWSDISPLAY, wsdisplay_cdevsw), 	/* Wscons Display */
		cdevsw_init(NWSKBD, wskbd_cdevsw),			/* Wscons Keyboard */
		cdevsw_init(NWSMOUSE, wsmouse_cdevsw),		/* Wscons Mouse */
		cdevsw_init(NWSMUX, wsmux_cdevsw),			/* Wscons Multiplexor */
		cdevsw_init(NWSFONT, wsfont_cdevsw),		/* Wscons Wsfont */
		cdevsw_init(NEVDEV, evdev_cdevsw),			/* Evdev Keyboard & Mouse*/
		cdevsw_init(NMIDI, midi_cdevsw),			/* MIDI I/O */
		cdevsw_init(NSEQUENCER, sequencer_cdevsw),	/* MIDI Sequencer I/O */
		cdevsw_init(NSES, ses_cdevsw),				/* SCSI ses */
		cdevsw_init(NAGP, agp_cdevsw),				/* AGP graphics aperture device */
		cdevsw_init(NKSYMS, ksyms_cdevsw),			/* Kernel symbols device */
		cdevsw_init(1, cmos_cdevsw),			    /* CMOS Interface */
        cdevsw_init(NOPENCRYPTO, crypto_cdevsw),    /* Opencrypto */
        cdevsw_init(NVIDEO, video_cdevsw),          /* generic video I/O */
        cdevsw_init(NPCI, pci_cdevsw),              /* PCI bus access device */
        cdevsw_init(NSCSIBUS, scsibus_cdevsw),      /* SCSI bus */
		/*
		cdevsw_init(NUSB, usb),
		cdevsw_init(NUHID, uhid),
		cdevsw_init(NUGEN, ugen),
		cdevsw_init(NUCOM, ucom),
		*/
};

const struct linesw *linesw0[] = {
		linesw_init(1, ttydisc),					/* 0- TTYDISC */
		linesw_init(1, nttydisc),					/* 1- NTTYDISC */
		linesw_init(1, ottydisc),					/* 2- OTTYDISC */
		linesw_init(0, netldisc),					/* 3- NETLDISC */
		linesw_init(NTB, tabldisc),					/* 4- TABLDISC */
		linesw_init(NSL, slipdisc),					/* 5- SLIPDISC */
		linesw_init(NPPP, pppdisc),					/* 6- PPPDISC */
		linesw_init(NSTRIP, stripdisc),				/* 7- STRIPDISC */
};

/*
const struct bdevsw *bdevsw0;
const struct cdevsw *cdevsw0;
const struct linesw *linesw0;
*/
const struct bdevsw **bdevsw = bdevsw0;
const struct cdevsw **cdevsw = cdevsw0;
const struct linesw **linesw = linesw0;
const int sys_bdevsws = __arraycount(bdevsw0);
const int sys_cdevsws = __arraycount(cdevsw0);
const int sys_linesws = __arraycount(linesw0);
int max_bdevsws = __arraycount(bdevsw0);
int max_cdevsws = __arraycount(cdevsw0);
int max_linesws = __arraycount(linesw0);

int	nblkdev = sys_bdevsws;
int	nchrdev = sys_cdevsws;

#ifdef deprecated
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
//	DEVSWIO_CONFIG_INIT(devsw, NTB, NULL, NULL, &tabldisc);					/* 4- TABLDISC */
	DEVSWIO_CONFIG_INIT(devsw, NSL, NULL, NULL, &slipdisc);					/* 5- SLIPDISC */
	DEVSWIO_CONFIG_INIT(devsw, NPPP, NULL, NULL, &pppdisc);					/* 6- PPPDISC */
	DEVSWIO_CONFIG_INIT(devsw, NSTRIP, NULL, NULL, &stripdisc);				/* 7- STRIPDISC */
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
	DEVSWIO_CONFIG_INIT(devsw, NCOM, NULL, &com_cdevsw, NULL);				/* Serial port */
	DEVSWIO_CONFIG_INIT(devsw, NLPT, NULL, &lpt_cdevsw, NULL);				/* parallel printer */
}

/* Add disk driver configuration */
void
disks_init(devsw)
	struct devswtable *devsw;
{
	/* ATA Devices */
	DEVSWIO_CONFIG_INIT(devsw, NWD, &wd_bdevsw, &wd_cdevsw, NULL);  		/* ATA: ST506/ESDI/IDE disk */

	/* Floppy Devices */
	DEVSWIO_CONFIG_INIT(devsw, NFDC, &fd_bdevsw, &fd_cdevsw, NULL);			/* floppy diskette */

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
	DEVSWIO_CONFIG_INIT(devsw, NMD, &md_bdevsw, &md_cdevsw, NULL);			/* memory disk driver */
}

/* Add miscellaneous driver configuration */
void
misc_init(devsw)
	struct devswtable *devsw;
{
	//DEVSWIO_CONFIG_INIT(devsw, 1, NULL, &apm_cdevsw, NULL);					/* Power Management (APM) Interface */
	DEVSWIO_CONFIG_INIT(devsw, 1, NULL, &cmos_cdevsw, NULL);				/* CMOS Interface */
	DEVSWIO_CONFIG_INIT(devsw, 1, NULL, &mm_cdevsw, NULL);					/* /dev/{null,mem,kmem,...} */
	DEVSWIO_CONFIG_INIT(devsw, NKSYMS, NULL, &ksyms_cdevsw, NULL);			/* Kernel symbols device */
	DEVSWIO_CONFIG_INIT(devsw, NRND, NULL, &rnd_cdevsw, NULL);				/* Random device */
}

/* Add network driver configuration */
void
network_init(devsw)
	struct devswtable *devsw;
{
	DEVSWIO_CONFIG_INIT(devsw, NBPFILTER, NULL, &bpf_cdevsw, NULL);			/* Berkeley packet filter */
	DEVSWIO_CONFIG_INIT(devsw, NTUN, NULL, &tun_cdevsw, NULL);				/* network tunnel */
	//DEVSWIO_CONFIG_INIT(devsw, NOPENCRYPTO, NULL, &crypto_cdevsw, NULL);		/* Opencrypto */
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
	//DEVSWIO_CONFIG_INIT(devsw, NVIDEO , NULL, &video_cdevsw, NULL);			/* generic video I/O */
	DEVSWIO_CONFIG_INIT(devsw, NAGP, NULL, &agp_cdevsw, NULL);				/* AGP Video */
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

	DEVSWIO_CONFIG_INIT(devsw, NEVDEV, NULL, &evdev_cdevsw, NULL);		    	/* Evdev Keyboard & Mouse*/
}
#endif /* deprecated */
