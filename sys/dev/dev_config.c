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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/devsw.h>
#include <sys/user.h>

/* Machine-independent dev configuration.
 * Should be execute in autoconf.c/conf.c per machine architecture
 */

/* Initializes all dev config drivers */
void
dev_init(devsw)
	struct devswtable *devsw;
{
	audio_init(devsw);
	console_init(devsw);
	disk_init(devsw);
	pseudo_init(devsw);
	usb_init(devsw);
	video_init(devsw);
}

/* Add audio driver configuration */
void
audio_init(devsw)
	struct devswtable *devsw;
{

}

/* Add console driver configuration */
void
console_init(devsw)
	struct devswtable *devsw;
{
	DEVSWIO_CONFIG_INIT(devsw, 1, NULL, &cons_cdevsw, NULL);			/* virtual console */
}

/* Add disk driver configuration */
void
disk_init(devsw)
	struct devswtable *devsw;
{
	/* ATA Devices */
	DEVSWIO_CONFIG_INIT(devsw, NWD, &wd_bdevsw, &wd_cdevsw, NULL);  	/* ATA: ST506/ESDI/IDE disk */

	/* SCSI Devices */
	DEVSWIO_CONFIG_INIT(devsw, NSD, &sd_bdevsw, &sd_cdevsw, NULL);		/* SCSI disk */
	DEVSWIO_CONFIG_INIT(devsw, NST, &st_bdevsw, &st_cdevsw, NULL);		/* SCSI tape */
	DEVSWIO_CONFIG_INIT(devsw, NCD, &cd_bdevsw, &cd_cdevsw, NULL);		/* SCSI CD-ROM */
	DEVSWIO_CONFIG_INIT(devsw, NCH, NULL, &ch_cdevsw, NULL);			/* SCSI autochanger */
	DEVSWIO_CONFIG_INIT(devsw, NUK, NULL, &uk_cdevsw, NULL);			/* SCSI unknown  */
	DEVSWIO_CONFIG_INIT(devsw, NSS, NULL, &ss_cdevsw, NULL);			/* SCSI scanner */
}

/* Add ports driver configuration */
void
ports_init(devsw)
	struct devswtable *devsw;
{
	DEVSWIO_CONFIG_INIT(devsw, NCOM, NULL, &com_cdevsw, NULL);			/* Serial port */
}

/* Add pseudo driver configuration */
void
pseudo_init(devsw)
	struct devswtable *devsw;
{
	DEVSWIO_CONFIG_INIT(devsw, NVND, &vnd_bdevsw, &vnd_cdevsw, NULL);	/* vnode disk driver */
	DEVSWIO_CONFIG_INIT(devsw, NCD, &ccd_bdevsw, &ccd_cdevsw, NULL);	/* "Concatenated" disk driver */
	DEVSWIO_CONFIG_INIT(devsw, NKSYMS, NULL, &ksyms_cdevsw, NULL);		/* Kernel symbols device */
}

/* Add usb driver configuration */
void
usb_init(devsw)
	struct devswtable *devsw;
{
	//DEVSWIO_CONFIG_INIT(devsw, NUSB, NULL, &usb_cdevsw, NULL);		/* USB controller */
}

/* Add video driver configuration */
void
video_init(devsw)
	struct devswtable *devsw;
{

}
