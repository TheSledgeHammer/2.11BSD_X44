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


/* bdevsw table */
const struct bdevsw *bdevsw0[] = {
		bdevsw_init(NWD, wd_bdevsw),				/* 0- ATA: ST506/ESDI/IDE disk */
		bdevsw_init(1, swap_bdevsw),				/* 1- swap interfaces */
		bdevsw_init(NFDC, fd_bdevsw),				/* 2- floppy diskette */
		bdevsw_init(NSD, sd_bdevsw),				/* 3- SCSI disk */
		bdevsw_init(NST, st_bdevsw),				/* 4- SCSI tape */
		bdevsw_init(NCD, cd_bdevsw),				/* 5- SCSI CD-ROM */
		bdevsw_init(NVND, vnd_bdevsw),				/* 6- vnode disk driver */
		bdevsw_init(NCCD, ccd_bdevsw),				/* 7- "Concatenated" disk driver */
		bdevsw_init(NMD, md_bdevsw),				/* 8- memory disk driver */
};

/* cdevsw table */
const struct cdevsw *cdevsw0[] = {
		cdevsw_init(1, cons_cdevsw),				/* 0- virtual console */
		cdevsw_init(1, ctty_cdevsw),				/* 1- ctty controlling terminal */
		cdevsw_init(1, mm_cdevsw),					/* 2- /dev/{null,mem,kmem,...} */
		cdevsw_init(NWD, wd_cdevsw),				/* 3- ATA: ST506/ESDI/IDE disk */
		cdevsw_init(1, swap_cdevsw),				/* 4- swap interfaces */
		cdevsw_init(NPTY, pts_cdevsw),				/* 5- pts pseudo-tty slave, pseudo-tty master  */
		cdevsw_init(NPTY, ptc_cdevsw),				/* 6- ptc pseudo-tty slave, pseudo-tty master  */
		cdevsw_init(1, log_cdevsw),				  	/* 7- log interfaces */
		cdevsw_init(NCOM, com_cdevsw),				/* 8- Serial port */
		cdevsw_init(NFDC, fd_cdevsw),				/* 9- floppy diskette */
		cdevsw_init(NSD, sd_cdevsw),				/* 10- SCSI disk */
		cdevsw_init(NST, st_cdevsw),				/* 11- SCSI tape */
		cdevsw_init(NCD, cd_cdevsw),				/* 12- SCSI CD-ROM */
		cdevsw_init(NLPT, lpt_cdevsw),				/* 13- parallel printer */
		cdevsw_init(NCH, ch_cdevsw),				/* 14- SCSI autochanger */
		cdevsw_init(NCCD, ccd_cdevsw),				/* 15- "Concatenated" disk driver */
		cdevsw_init(NSS, ss_cdevsw),				/* 16- SCSI scanner */
		cdevsw_init(NUK, uk_cdevsw),				/* 17- SCSI unknown  */
		cdevsw_init(NBPFILTER, bpf_cdevsw),			/* 18- Berkeley packet filter */
		cdevsw_init(NMD, md_cdevsw),				/* 19- memory disk driver */
		cdevsw_init(NSPKR, spkr_cdevsw),			/* 20- PC Speaker */
		cdevsw_init(NTUN, tun_cdevsw),				/* 21- network tunnel */
		cdevsw_init(NVND, vnd_cdevsw),				/* 22- vnode disk driver */
		cdevsw_init(NAUDIO, audio_cdevsw),			/* 23- generic audio I/O */
		cdevsw_init(NRND, rnd_cdevsw),				/* 24- Random device */
		cdevsw_init(NWSDISPLAY, wsdisplay_cdevsw), 	/* 25- Wscons Display */
		cdevsw_init(NWSKBD, wskbd_cdevsw),			/* 26- Wscons Keyboard */
		cdevsw_init(NWSMOUSE, wsmouse_cdevsw),		/* 27- Wscons Mouse */
		cdevsw_init(NWSMUX, wsmux_cdevsw),			/* 28- Wscons Multiplexor */
		cdevsw_init(NWSFONT, wsfont_cdevsw),		/* 29- Wscons Wsfont */
		cdevsw_init(NEVDEV, evdev_cdevsw),			/* 30- Evdev Keyboard & Mouse*/
		cdevsw_init(NMIDI, midi_cdevsw),			/* 31- MIDI I/O */
		cdevsw_init(NSEQUENCER, sequencer_cdevsw),	/* 32- MIDI Sequencer I/O */
		cdevsw_init(NSES, ses_cdevsw),				/* 33- SCSI ses */
		cdevsw_init(NAGP, agp_cdevsw),				/* 34- AGP graphics aperture device */
		cdevsw_init(NKSYMS, ksyms_cdevsw),			/* 35- Kernel symbols device */
		cdevsw_init(1, cmos_cdevsw),			    /* 36- CMOS Interface */
		cdevsw_init(NOPENCRYPTO, crypto_cdevsw),    /* 37- Opencrypto */
		cdevsw_init(NVIDEO, video_cdevsw),          /* 38- generic video I/O */
		cdevsw_init(NPCI, pci_cdevsw),              /* 39- PCI bus access device */
		cdevsw_init(NSCSIBUS, scsibus_cdevsw),      /* 40- SCSI bus */
		/*
		cdevsw_init(NUSB, usb),
		cdevsw_init(NUHID, uhid),
		cdevsw_init(NUGEN, ugen),
		cdevsw_init(NUCOM, ucom),
		*/
};

/* linesw table */
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
