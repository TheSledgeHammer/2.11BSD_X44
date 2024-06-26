/* $NetBSD: wsdisplayvar.h,v 1.25.4.2 2004/06/07 09:47:19 tron Exp $ */

/*
 * Copyright (c) 1996, 1997 Christopher G. Demetriou.  All rights reserved.
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

struct device;

/*
 * WSDISPLAY interfaces
 */

/*
 * Emulation functions, for displays that can support glass-tty terminal
 * emulations.  These are character oriented, with row and column
 * numbers starting at zero in the upper left hand corner of the
 * screen.
 *
 * These are used only when emulating a terminal.  Therefore, displays
 * drivers which cannot emulate terminals do not have to provide them.
 *
 * There is a "void *" cookie provided by the display driver associated
 * with these functions, which is passed to them when they are invoked.
 */
struct wsdisplay_emulops {
	void	(*cursor)(void *c, int on, int row, int col);
	int		(*mapchar)(void *, int, unsigned int *);
	void	(*putchar)(void *c, int row, int col, u_int uc, long attr);
	void	(*copycols)(void *c, int row, int srccol, int dstcol, int ncols);
	void	(*erasecols)(void *c, int row, int startcol, int ncols, long);
	void	(*copyrows)(void *c, int srcrow, int dstrow, int nrows);
	void	(*eraserows)(void *c, int row, int nrows, long);
	int		(*allocattr)(void *c, int fg, int bg, int flags, long *);
/* fg / bg values. Made identical to ANSI terminal color codes. */
#define WSCOL_BLACK			0
#define WSCOL_RED			1
#define WSCOL_GREEN			2
#define WSCOL_BROWN			3
#define WSCOL_BLUE			4
#define WSCOL_MAGENTA		5
#define WSCOL_CYAN			6
#define WSCOL_WHITE			7
#define WSCOL_LIGHT_GREY	(WSCOL_BLACK+8)
#define WSCOL_LIGHT_RED		(WSCOL_RED+8)
#define WSCOL_LIGHT_GREEN	(WSCOL_GREEN+8)
#define WSCOL_LIGHT_BROWN	(WSCOL_BROWN+8)
#define WSCOL_LIGHT_BLUE	(WSCOL_BLUE+8)
#define WSCOL_LIGHT_MAGENTA	(WSCOL_MAGENTA+8)
#define WSCOL_LIGHT_CYAN	(WSCOL_CYAN+8)
#define WSCOL_LIGHT_WHITE	(WSCOL_WHITE+8)
/* flag values: */
#define WSATTR_REVERSE		1
#define WSATTR_HILIT		2
#define WSATTR_BLINK		4
#define WSATTR_UNDERLINE 	8
#define WSATTR_WSCOLORS 	16
#define WSATTR_USERMASK 	0x0fff
/* private flags used by the driver */
#define WSATTR_PRIVATE1  	4096
#define WSATTR_PRIVATE2  	8192
#define WSATTR_PRIVATE3 	16384
#define WSATTR_PRIVATE4 	32768
	/* XXX need a free_attr() ??? */
};

struct wsscreen_descr {
	char *name;
	int ncols, nrows;
	const struct wsdisplay_emulops *textops;
	int fontwidth, fontheight;
	int capabilities;
#define WSSCREEN_WSCOLORS	1	/* minimal color capability */
#define WSSCREEN_REVERSE	2	/* can display reversed */
#define WSSCREEN_HILIT		4	/* can highlight (however) */
#define WSSCREEN_BLINK		8	/* can blink */
#define WSSCREEN_UNDERLINE	16	/* can underline */
	void *modecookie;
};

struct wsdisplay_font;
struct wsdisplay_char;
/*
 * Display access functions, invoked by user-land programs which require
 * direct device access, such as X11.
 *
 * There is a "void *" cookie provided by the display driver associated
 * with these functions, which is passed to them when they are invoked.
 */
struct wsdisplay_accessops {
	int		(*ioctl)(void *, u_long, caddr_t, int, struct proc *);
	caddr_t	(*mmap)(void *, off_t, int);
	int		(*alloc_screen)(void *, const struct wsscreen_descr *, void **, int *, int *, long *);
	void	(*free_screen)(void *, void *);
	int		(*show_screen)(void *, void *, int, void (*) (void *, int, int), void *);
	int		(*load_font)(void *, void *, struct wsdisplay_font *);
	void	(*pollc)(void *, int);
	int		(*getwschar)(void *, struct wsdisplay_char *);
	int		(*putwschar)(void *, struct wsdisplay_char *);
	void	(*scroll)(void *, void *, int);
};

/*
 * Attachment information provided by wsdisplaydev devices when attaching
 * wsdisplay units.
 */
struct wsdisplaydev_attach_args {
	const struct wsdisplay_accessops 	*accessops;		/* access ops */
	void								*accesscookie;	/* access cookie */
};

/* passed to wscons by the video driver to tell about its capabilities */
struct wsscreen_list {
	int 						nscreens;
	const struct wsscreen_descr **screens;
};

/*
 * Attachment information provided by wsemuldisplaydev devices when attaching
 * wsdisplay units.
 */
struct wsemuldisplaydev_attach_args {
	int									console;		/* is it console? */
	const struct wsscreen_list 			*scrdata;		/* screen cfg info */
	const struct wsdisplay_accessops 	*accessops;		/* access ops */
	void								*accesscookie;	/* access cookie */
};

#include "locators.h"

#define WSEMULDISPLAYDEVCF_CONSOLE				0
#define WSEMULDISPLAYDEVCF_CONSOLE_DEFAULT		-1	/* spec'd as console? */
#define	WSEMULDISPLAYDEVCF_CONSOLE_UNK			(WSEMULDISPLAYDEVCF_CONSOLE_DEFAULT)
#define WSEMULDISPLAYDEVCF_KBDMUX				1
#define WSDISPLAYDEVCF_KBDMUX					0

struct wscons_syncops {
	int 	(*detach)(void *, int, void (*)(void *, int, int), void *);
	int 	(*attach)(void *, int, void (*)(void *, int, int), void *);
	int 	(*check)(void *);
	void 	(*destroy)(void *);
};

/*
 * Autoconfiguration helper functions.
 */
void	wsdisplay_cnattach(const struct wsscreen_descr *, void *, int, int, long);
int		wsdisplaydevprint(void *, const char *);
int		wsemuldisplaydevprint(void *, const char *);

/*
 * Console interface.
 */
void	wsdisplay_cnputc(dev_t dev, int i);

/*
 * for use by compatibility code
 */
struct wsdisplay_softc;
struct wsscreen;
int wsscreen_attach_sync(struct wsscreen *, const struct wscons_syncops *, void *);
int wsscreen_detach_sync(struct wsscreen *);
int wsscreen_lookup_sync(struct wsscreen *, const struct wscons_syncops *, void **);

int wsdisplay_maxscreenidx(struct wsdisplay_softc *);
int wsdisplay_screenstate(struct wsdisplay_softc *, int);
int wsdisplay_getactivescreen(struct wsdisplay_softc *);
int wsscreen_switchwait(struct wsdisplay_softc *, int);

int wsdisplay_internal_ioctl(struct wsdisplay_softc *, struct wsscreen *, u_long , caddr_t, int, struct proc *);

int wsdisplay_usl_ioctl1(struct wsdisplay_softc *, u_long, caddr_t, int, struct proc *);

int wsdisplay_usl_ioctl2(struct wsdisplay_softc *, struct wsscreen *, u_long, caddr_t, int, struct proc *);

int wsdisplay_stat_ioctl(struct wsdisplay_softc *, u_long, caddr_t, int, struct proc *);

int wsdisplay_cfg_ioctl(struct wsdisplay_softc *, u_long, caddr_t, int, struct proc *);

#ifdef WSDISPLAY_SCROLLSUPPORT
void wsdisplay_scroll(void *v, int op);
#endif

#define WSDISPLAY_SCROLL_BACKWARD	1
#define WSDISPLAY_SCROLL_FORWARD	(1 << 1)
#define WSDISPLAY_SCROLL_RESET		(1 << 2)
#define WSDISPLAY_SCROLL_LOW		(1 << 3)

int wsdisplay_stat_inject(struct device *, u_int, int);

/*
 * for general use
 */
#define WSDISPLAY_NULLSCREEN	-1
void wsdisplay_switchtoconsole(void);
const struct wsscreen_descr *
    wsdisplay_screentype_pick(const struct wsscreen_list *, const char *);
