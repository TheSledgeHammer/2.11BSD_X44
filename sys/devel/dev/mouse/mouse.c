/*-
 * Copyright (c) 1999 Kazutaka YOKOTA <yokota@zodiac.mech.utsunomiya-u.ac.jp>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer as
 *    the first lines of this file unmodified.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/dev/syscons/scmouse.c,v 1.12.2.3 2001/07/28 12:51:47 yokota Exp $
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/signalvar.h>
#include <sys/proc.h>
#include <sys/tty.h>

#include <devel/dev/mouse.h>

#include "syscons.h"

/* move mouse */
void
sc_mouse_move(scr_stat *scp, int x, int y)
{
	scp->mouse_xpos = scp->mouse_oldxpos = x;
	scp->mouse_ypos = scp->mouse_oldypos = y;
	if (scp->font_height <= 0) {
		scp->mouse_pos = scp->mouse_oldpos = 0;
	} else {
		scp->mouse_pos = scp->mouse_oldpos = (y / scp->font_height - scp->yoff)
				* scp->xsize + x / scp->font_width - scp->xoff;
	}
	scp->status |= MOUSE_MOVED;
}

/* adjust mouse position */
static void
set_mouse_pos(scr_stat *scp)
{
	if (scp->mouse_xpos < scp->xoff * scp->font_width)
		scp->mouse_xpos = scp->xoff * scp->font_width;
	if (scp->mouse_ypos < scp->yoff * scp->font_height)
		scp->mouse_ypos = scp->yoff * scp->font_height;
	if (ISGRAPHSC(scp)) {
		if (scp->mouse_xpos > scp->xpixel - 1)
			scp->mouse_xpos = scp->xpixel - 1;
		if (scp->mouse_ypos > scp->ypixel - 1)
			scp->mouse_ypos = scp->ypixel - 1;
		return;
	} else {
		if (scp->mouse_xpos > (scp->xsize + scp->xoff) * scp->font_width - 1)
			scp->mouse_xpos = (scp->xsize + scp->xoff) * scp->font_width - 1;
		if (scp->mouse_ypos > (scp->ysize + scp->yoff) * scp->font_height - 1)
			scp->mouse_ypos = (scp->ysize + scp->yoff) * scp->font_height - 1;
	}

	if (scp->mouse_xpos != scp->mouse_oldxpos
			|| scp->mouse_ypos != scp->mouse_oldypos) {
		scp->status |= MOUSE_MOVED;
		scp->mouse_pos = (scp->mouse_ypos / scp->font_height - scp->yoff)
				* scp->xsize + scp->mouse_xpos / scp->font_width - scp->xoff;
#ifndef SC_NO_CUTPASTE
		if ((scp->status & MOUSE_VISIBLE) && (scp->status & MOUSE_CUTTING))
			mouse_cut(scp);
#endif
	}
}

void
sc_draw_mouse_image(scr_stat *scp)
{
	if (ISGRAPHSC(scp))
		return;

	atomic_add_int(&scp->sc->videoio_in_progress, 1);
	(*scp->rndr->draw_mouse)(scp, scp->mouse_xpos, scp->mouse_ypos, TRUE);
	scp->mouse_oldpos = scp->mouse_pos;
	scp->mouse_oldxpos = scp->mouse_xpos;
	scp->mouse_oldypos = scp->mouse_ypos;
	scp->status |= MOUSE_VISIBLE;
	atomic_add_int(&scp->sc->videoio_in_progress, -1);
}

void
sc_remove_mouse_image(scr_stat *scp)
{
	int size;
	int i;

	if (ISGRAPHSC(scp))
		return;

	atomic_add_int(&scp->sc->videoio_in_progress, 1);
	(*scp->rndr->draw_mouse)(scp,
			(scp->mouse_oldpos % scp->xsize + scp->xoff) * scp->font_width,
			(scp->mouse_oldpos / scp->xsize + scp->yoff) * scp->font_height,
			FALSE);
	size = scp->xsize * scp->ysize;
	i = scp->mouse_oldpos;
	mark_for_update(scp, i);
	mark_for_update(scp, i);
	if (i + scp->xsize + 1 < size) {
		mark_for_update(scp, i + scp->xsize + 1);
	} else if (i + scp->xsize < size) {
		mark_for_update(scp, i + scp->xsize);
	} else if (i + 1 < size) {
		mark_for_update(scp, i + 1);
	}
	scp->status &= ~MOUSE_VISIBLE;
	atomic_add_int(&scp->sc->videoio_in_progress, -1);
}
