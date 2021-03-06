/* $NetBSD: wsconsio.h,v 1.16 1999/02/12 11:14:49 drochner Exp $ */

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

#ifndef _DEV_WSCONS_WSCONSIO_H_
#define	_DEV_WSCONS_WSCONSIO_H_

/*
 * WSCONS (wsdisplay, wskbd, wsmouse) exported interfaces.
 *
 * Ioctls are all in group 'W'.  Ioctl number space is partitioned like:
 *	0-31	keyboard ioctls (WSKBDIO)
 *	32-63	mouse ioctls (WSMOUSEIO)
 *	64-95	display ioctls (WSDISPLAYIO)
 *	96-255	reserved for future use
 */

#include <sys/types.h>
#include <sys/ioccom.h>
#include <dev/misc/wscons/wsksymvar.h>

/*
 * Common event structure (used by keyboard and mouse)
 */
struct wscons_event {
	u_int			type;
	int				value;
	struct timespec	time;
};

/* Event type definitions.  Comment for each is information in value. */
#define	WSCONS_EVENT_KEY_UP				1	/* key code */
#define	WSCONS_EVENT_KEY_DOWN			2	/* key code */
#define	WSCONS_EVENT_ALL_KEYS_UP		3	/* void */
#define	WSCONS_EVENT_MOUSE_UP			4	/* button # (leftmost = 0) */
#define	WSCONS_EVENT_MOUSE_DOWN			5	/* button # (leftmost = 0)  */
#define	WSCONS_EVENT_MOUSE_DELTA_X		6	/* X delta amount */
#define	WSCONS_EVENT_MOUSE_DELTA_Y		7	/* Y delta amount */
#define	WSCONS_EVENT_MOUSE_ABSOLUTE_X	8	/* X location */
#define	WSCONS_EVENT_MOUSE_ABSOLUTE_Y	9	/* Y location */
#define	WSCONS_EVENT_MOUSE_DELTA_Z		10	/* Z delta amount */
#define	WSCONS_EVENT_MOUSE_ABSOLUTE_Z	11	/* Z location */


/*
 * Keyboard ioctls (0 - 31)
 */

/* Get keyboard type. */
#define	WSKBDIO_GTYPE					_IOR('W', 0, u_int)
#define		WSKBD_TYPE_LK201			1	/* lk-201 */
#define		WSKBD_TYPE_LK401			2	/* lk-401 */
#define		WSKBD_TYPE_PC_XT			3	/* PC-ish, XT scancode */
#define		WSKBD_TYPE_PC_AT			4	/* PC-ish, AT scancode */
#define		WSKBD_TYPE_USB				5	/* USB, XT scancode */
#define		WSKBD_TYPE_NEXT				6	/* NeXT keyboard */

/* Manipulate the keyboard bell. */
struct wskbd_bell_data {
	u_int	which;				/* values to get/set */
	u_int	pitch;				/* pitch, in Hz */
	u_int	period;				/* period, in milliseconds */
	u_int	volume;				/* percentage of max volume */
};
#define		WSKBD_BELL_DOPITCH			0x1		/* get/set pitch */
#define		WSKBD_BELL_DOPERIOD			0x2		/* get/set period */
#define		WSKBD_BELL_DOVOLUME			0x4		/* get/set volume */
#define		WSKBD_BELL_DOALL			0x7		/* all of the above */

#define	WSKBDIO_BELL					_IO('W', 1)
#define	WSKBDIO_COMPLEXBELL				_IOW('W', 2, struct wskbd_bell_data)
#define	WSKBDIO_SETBELL					_IOW('W', 3, struct wskbd_bell_data)
#define	WSKBDIO_GETBELL					_IOR('W', 4, struct wskbd_bell_data)
#define	WSKBDIO_SETDEFAULTBELL			_IOW('W', 5, struct wskbd_bell_data)
#define	WSKBDIO_GETDEFAULTBELL			_IOR('W', 6, struct wskbd_bell_data)

/* Manipulate the emulation key repeat settings. */
struct wskbd_keyrepeat_data {
	u_int	which;				/* values to get/set */
	u_int	del1;				/* delay before first, ms */
	u_int	delN;				/* delay before rest, ms */
};
#define		WSKBD_KEYREPEAT_DODEL1		0x1		/* get/set del1 */
#define		WSKBD_KEYREPEAT_DODELN		0x2		/* get/set delN */
#define		WSKBD_KEYREPEAT_DOALL		0x3		/* all of the above */

#define	WSKBDIO_SETKEYREPEAT			_IOW('W', 7, struct wskbd_keyrepeat_data)
#define	WSKBDIO_GETKEYREPEAT			_IOR('W', 8, struct wskbd_keyrepeat_data)
#define	WSKBDIO_SETDEFAULTKEYREPEAT 	_IOW('W', 9, struct wskbd_keyrepeat_data)
#define	WSKBDIO_GETDEFAULTKEYREPEAT     _IOR('W', 10, struct wskbd_keyrepeat_data)

/* Get/set keyboard leds */
#define		WSKBD_LED_CAPS				0x01
#define		WSKBD_LED_NUM				0x02
#define		WSKBD_LED_SCROLL			0x04
#define		WSKBD_LED_COMPOSE			0x08

#define	WSKBDIO_SETLEDS					_IOW('W', 11, int)
#define	WSKBDIO_GETLEDS					_IOR('W', 12, int)

/* Manipulate keysym groups. */
struct wskbd_map_data {
	u_int								maplen;		/* number of entries in map */
	struct wscons_keymap 				*map;		/* map to get or set */
};
#define WSKBDIO_GETMAP					_IOWR('W', 13, struct wskbd_map_data)
#define WSKBDIO_SETMAP					_IOW('W', 14, struct wskbd_map_data)
#define WSKBDIO_GETENCODING				_IOR('W', 15, int)
#define WSKBDIO_SETENCODING				_IOW('W', 16, int)

/* internal use only */
#define WSKBDIO_SETMODE					_IOW('W', 19, int)
#define WSKBDIO_GETMODE					_IOR('W', 20, int)
#define		WSKBD_TRANSLATED			0
#define		WSKBD_RAW					1

/*
 * Mouse ioctls (32 - 63)
 */

/* Get mouse type */
#define	WSMOUSEIO_GTYPE					_IOR('W', 32, u_int)
#define		WSMOUSE_TYPE_VSXXX			1	/* DEC TC(?) serial */
#define		WSMOUSE_TYPE_PS2			2	/* PS/2-compatible */
#define		WSMOUSE_TYPE_USB			3	/* USB mouse */
#define		WSMOUSE_TYPE_LMS			4	/* Logitech busmouse */
#define		WSMOUSE_TYPE_MMS			5	/* Microsoft InPort mouse */

/* Set resolution.  Not applicable to all mouse types. */
#define	WSMOUSEIO_SRES					_IOW('W', 33, u_int)
#define	WSMOUSE_RES_MIN					0
#define	WSMOUSE_RES_DEFAULT				75
#define	WSMOUSE_RES_MAX					100

/*
 * Display ioctls (64 - 95)
 */

/* Get display type */
#define	WSDISPLAYIO_GTYPE				_IOR('W', 64, u_int)
#define		WSDISPLAY_TYPE_UNKNOWN		0	/* unknown */
#define		WSDISPLAY_TYPE_PM_MONO		1	/* ??? */
#define		WSDISPLAY_TYPE_PM_COLOR		2	/* ??? */
#define		WSDISPLAY_TYPE_CFB			3	/* DEC TC CFB */
#define		WSDISPLAY_TYPE_XCFB			4	/* ??? */
#define		WSDISPLAY_TYPE_MFB			5	/* DEC TC MFB */
#define		WSDISPLAY_TYPE_SFB			6	/* DEC TC SFB */
#define		WSDISPLAY_TYPE_ISAVGA		7	/* (generic) ISA VGA */
#define		WSDISPLAY_TYPE_PCIVGA		8	/* (generic) PCI VGA */
#define		WSDISPLAY_TYPE_TGA			9	/* DEC PCI TGA */
#define		WSDISPLAY_TYPE_SFBP			10	/* DEC TC SFB+ */
#define		WSDISPLAY_TYPE_PCIMISC		11	/* (generic) PCI misc. disp. */
#define		WSDISPLAY_TYPE_NEXTMONO		12	/* NeXT mono display */

/* Basic display information.  Not applicable to all display types. */
struct wsdisplay_fbinfo {
	u_int	height;				/* height in pixels */
	u_int	width;				/* width in pixels */
	u_int	depth;				/* bits per pixel */
	u_int	cmsize;				/* color map size (entries) */
};
#define	WSDISPLAYIO_GINFO				_IOR('W', 65, struct wsdisplay_fbinfo)

/* Colormap operations.  Not applicable to all display types. */
struct wsdisplay_cmap {
	u_int	index;				/* first element (0 origin) */
	u_int	count;				/* number of elements */
	u_char	*red;				/* red color map elements */
	u_char	*green;				/* green color map elements */
	u_char	*blue;				/* blue color map elements */
};      
#define WSDISPLAYIO_GETCMAP				_IOW('W', 66, struct wsdisplay_cmap)
#define WSDISPLAYIO_PUTCMAP				_IOW('W', 67, struct wsdisplay_cmap)

/* Video control.  Not applicable to all display types. */
#define	WSDISPLAYIO_GVIDEO				_IOR('W', 68, u_int)
#define	WSDISPLAYIO_SVIDEO				_IOW('W', 69, u_int)
#define		WSDISPLAYIO_VIDEO_OFF		0	/* video off */
#define		WSDISPLAYIO_VIDEO_ON		1	/* video on */

/* Cursor control.  Not applicable to all display types. */
struct wsdisplay_curpos {			/* cursor "position" */
	u_int 								x, y;
};

struct wsdisplay_cursor {
	u_int								which;		/* values to get/set */
	u_int								enable;		/* enable/disable */
	struct wsdisplay_curpos 			pos;		/* position */
	struct wsdisplay_curpos 			hot;		/* hot spot */
	struct wsdisplay_cmap 				cmap;		/* color map info */
	struct wsdisplay_curpos 			size;		/* bit map size */
	u_char 								*image;		/* image data */
	u_char 								*mask;		/* mask data */
};
#define		WSDISPLAY_CURSOR_DOCUR		0x01	/* get/set enable */
#define		WSDISPLAY_CURSOR_DOPOS		0x02	/* get/set pos */
#define		WSDISPLAY_CURSOR_DOHOT		0x04	/* get/set hot spot */
#define		WSDISPLAY_CURSOR_DOCMAP		0x08	/* get/set cmap */
#define		WSDISPLAY_CURSOR_DOSHAPE	0x10	/* get/set img/mask */
#define		WSDISPLAY_CURSOR_DOALL		0x1f	/* all of the above */

/* Cursor control: get and set position */
#define	WSDISPLAYIO_GCURPOS				_IOR('W', 70, struct wsdisplay_curpos)
#define	WSDISPLAYIO_SCURPOS				_IOW('W', 71, struct wsdisplay_curpos)

/* Cursor control: get maximum size */
#define	WSDISPLAYIO_GCURMAX				_IOR('W', 72, struct wsdisplay_curpos)

/* Cursor control: get/set cursor attributes/shape */
#define	WSDISPLAYIO_GCURSOR				_IOWR('W', 73, struct wsdisplay_cursor)
#define	WSDISPLAYIO_SCURSOR				_IOW('W', 74, struct wsdisplay_cursor)

/* Display mode: Emulation (text) vs. Mapped (graphics) mode */
#define	WSDISPLAYIO_GMODE				_IOR('W', 75, u_int)
#define	WSDISPLAYIO_SMODE				_IOW('W', 76, u_int)
#define		WSDISPLAYIO_MODE_EMUL		0	/* emulation (text) mode */
#define		WSDISPLAYIO_MODE_MAPPED		1	/* mapped (graphics) mode */

/*
 * XXX WARNING
 * XXX The following 3 definitions are very preliminary and are likely
 * XXX to be changed without care about backwards compatibility!
 */
struct wsdisplay_font {
	char *name;
	int firstchar, numchars;
	int encoding;
#define WSDISPLAY_FONTENC_ISO 			0
#define WSDISPLAY_FONTENC_IBM 			1
#define WSDISPLAY_FONTENC_PCVT 			2
	int 								fontwidth, fontheight, stride; /* XXX endianness??? */
	void 								*data;
};
#define WSDISPLAYIO_LDFONT				_IOW('W', 77, struct wsdisplay_font)

struct wsdisplay_addscreendata {
	int 								idx; /* screen index */
	char 								*screentype;
	char 								*emul;
};
#define WSDISPLAYIO_ADDSCREEN 			_IOW('W', 78, struct wsdisplay_addscreendata)

struct wsdisplay_delscreendata {
	int 								idx; /* screen index */
	int 								flags;
#define WSDISPLAY_DELSCR_FORCE 			1
};
#define WSDISPLAYIO_DELSCREEN 			_IOW('W', 79, struct wsdisplay_delscreendata)

struct wsdisplay_usefontdata {
	char 								*name;
};
#define WSDISPLAYIO_USEFONT				_IOW('W', 80, struct wsdisplay_usefontdata)

/* XXX NOT YET DEFINED */
/* Mapping information retrieval. */

#endif /* _DEV_WSCONS_WSCONSIO_H_ */
