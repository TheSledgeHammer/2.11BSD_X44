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
#include <sys/user.h>

/*
 * Machine-independent device driver configuration.
 * Runs "machine/autoconf.c"
 */

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

/* Add console driver configuration */
void
console_init(devsw)
	struct devswtable *devsw;
{
	DEVSWIO_CONFIG_INIT(devsw, 1, NULL, &cons_cdevsw, NULL);				/* virtual console */
}

/* Add disk driver configuration */
void
disk_init(devsw)
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

	/* Pseudo Devices */
	DEVSWIO_CONFIG_INIT(devsw, NVND, &vnd_bdevsw, &vnd_cdevsw, NULL);		/* vnode disk driver */
	DEVSWIO_CONFIG_INIT(devsw, NCCD, &ccd_bdevsw, &ccd_cdevsw, NULL);		/* "Concatenated" disk driver */
}

/* Add miscellaneous driver configuration */
void
misc_init(devsw)
	struct devswtable *devsw;
{
	DEVSWIO_CONFIG_INIT(devsw, NCOM, NULL, &com_cdevsw, NULL);				/* Serial port */
	DEVSWIO_CONFIG_INIT(devsw, NKSYMS, NULL, &ksyms_cdevsw, NULL);			/* Kernel symbols device */
}

/* Add network driver configuration */
void
network_init(devsw)
	struct devswtable *devsw;
{
	/* TODO: Need to be configured or fix missing */
	DEVSWIO_CONFIG_INIT(devsw, NBPFILTER, NULL, &bpf_cdevsw, NULL);			/* Berkeley packet filter */
}

/* tty global driver configuration */
void
tty_init(devsw)
	struct devswtable *devsw;
{
	ctty_init(&sys_devsw); 													/* tty_ctty.c: controlling terminal */
	pty_init(&sys_devsw);													/* tty_pty.c: pseudo-tty slave, pseudo-tty master */
	tty_conf_init(&sys_devsw);												/* tty_conf.c: pseudo-tty ptm device */
}

/* Add usb driver configuration */
void
usb_init(devsw)
	struct devswtable *devsw;
{
	DEVSWIO_CONFIG_INIT(devsw, NUSB, NULL, &usb_cdevsw, NULL);				/* USB controller */
	DEVSWIO_CONFIG_INIT(devsw, NUHID, NULL, &uhid_cdevsw, NULL);			/* USB generic HID */
	DEVSWIO_CONFIG_INIT(devsw, NUGEN, NULL, &ugen_cdevsw, NULL);			/* USB generic driver */
	DEVSWIO_CONFIG_INIT(devsw, NUCOM, NULL, &ucom_cdevsw, NULL);			/* USB tty */
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
}
