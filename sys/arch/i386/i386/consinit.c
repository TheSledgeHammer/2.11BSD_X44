/*	$NetBSD: consinit.c,v 1.9 2001/11/20 08:43:27 lukem Exp $	*/

/*
 * Copyright (c) 1998
 *	Matthias Drochner.  All rights reserved.
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
 *
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/bootinfo.h>

#include "vga.h"
#include "ega.h"
#include "pcdisplay.h"
#if (NVGA > 0) || (NEGA > 0) || (NPCDISPLAY > 0)
#include <dev/core/ic/mc6845reg.h>
//#include <dev/misc/pccons/pcdisplayvar.h>
#if (NVGA > 0)
#include <dev/video/vga/vgareg.h>
#include <dev/video/vga/vgavar.h>
#endif
#if (NEGA > 0)
#include <dev/video/ega/egavar.h>
#endif
#if (NPCDISPLAY > 0)
#include <dev/misc/pccons/pcdisplayvar.h>
#endif
#endif

#include "pckbc.h"
#if (NPCKBC > 0)
#include <dev/core/isa/isareg.h>
#include <dev/core/ic/i8042reg.h>
#include <dev/misc/pccons/pckbcvar.h>
#include <dev/misc/pccons/pckbportvar.h>
#endif
#include "pckbd.h"

#include "com.h"
#if (NCOM > 0)
#include <sys/termios.h>
#include <dev/core/ic/comreg.h>
#include <dev/core/ic/comvar.h>
#endif

//#include "ukbd.h"
#if (NUKBD > 0)
#include <dev/usb/ukbdvar.h>
#endif

#ifndef CONSDEVNAME
#define CONSDEVNAME "pc"
#endif
#if (NCOM > 0)
#ifndef CONADDR
#define CONADDR 	0x3f8
#endif
#ifndef CONSPEED
#define CONSPEED 	TTYDEF_SPEED
#endif
#ifndef CONMODE
#define CONMODE 	((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#endif
int comcnmode = 	CONMODE;
#endif /* NCOM */

const struct bootinfo_console default_consinfo = {
		.bi_devname = CONSDEVNAME,
#if (NCOM > 0)
		.bi_addr = CONADDR,
		.bi_speed = CONSPEED
#else
		.bi_addr = 0,
		.bi_speed = 0
#endif
};

void consinit_io(struct bootinfo_console *);
void consinit_com(struct bootinfo_console *);

/*
 * consinit:
 * initialize the system console.
 * XXX - shouldn't deal with this initted thing, but then,
 * it shouldn't be called from init386 either.
 */
void
consinit(void)
{
	struct bootinfo_console *consinfo;
	static int initted;

	if (initted) {
		return;
	}
	initted = 1;

#ifndef CONS_OVERRIDE
	consinfo = bootinfo_lookup(BOOTINFO_CONSOLE);
	if (!consinfo) {
#endif
		consinfo = &default_consinfo;
	}

#if (NPC > 0) || (NVGA > 0) || (NEGA > 0) || (NPCDISPLAY > 0)
	consinit_io(consinfo);
#endif /* PC | VT | VGA | PCDISPLAY */
#if (NCOM > 0)
	consinit_com(consinfo);
#endif
	panic("invalid console device %s", consinfo->bi_devname);
}

/* consinit keyboard, mouse & display */
void
consinit_io(consinfo)
	struct bootinfo_console *consinfo;
{
	if (!strcmp(consinfo->bi_devname, "pc")) {
#if (NVGA > 0)
		if (!vga_cnattach(I386_BUS_SPACE_IO, I386_BUS_SPACE_MEM, -1, 1)) {
			goto dokbd;
		}
#endif /* NVGA > 0 */
#if (NEGA > 0)
		if (!ega_cnattach(I386_BUS_SPACE_IO, I386_BUS_SPACE_MEM)) {
			goto dokbd;
		}
#endif /* NEGA > 0 */
#if (NPCDISPLAY > 0)
		if (!pcdisplay_cnattach(I386_BUS_SPACE_IO, I386_BUS_SPACE_MEM)) {
			goto dokbd;
		}
#endif /* NPCDISPLAY > 0 */
	}
	if (0) {
		goto dokbd;
	}
dokbd:
#if (NPCKBC > 0)
	pckbc_cnattach(I386_BUS_SPACE_IO, IO_KBD, KBCMDP, PCKBC_KBD_SLOT);
#endif /* NPCKBC > 0 */
#if NPCKBC == 0 && NUKBD > 0
	ukbd_cnattach();
#endif
	return;
}

/* consinit com */
void
consinit_com(consinfo)
	struct bootinfo_console *consinfo;
{
#if (NCOM > 0)
	if (!strcmp(consinfo->bi_devname, "com")) {
		bus_space_tag_t tag = I386_BUS_SPACE_IO;

		if (comcnattach(tag, consinfo->bi_addr, consinfo->bi_speed, COM_FREQ, comcnmode)) {
			panic("can't init serial console @%x", consinfo->bi_addr);
		}
		return;
	}
#endif
}
