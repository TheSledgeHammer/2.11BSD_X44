/*-
 * Copyright (c) 2014 Jakub Wojciech Klama <jceel@FreeBSD.org>
 * Copyright (c) 2015-2016 Vladimir Kondratyev <wulf@FreeBSD.org>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#ifndef	_DEV_EVDEV_EVDEV_PRIVATE_H
#define	_DEV_EVDEV_EVDEV_PRIVATE_H

#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/lock.h>
#include <sys/queue.h>
#include <sys/sysctl.h>

#include <dev/misc/evdev/evdev.h>
#include <dev/misc/evdev/input.h>
#include <dev/misc/evdev/freebsd-bitstring.h>

#define M_EVDEV		91
#define	NAMELEN		80

/*
 * bitstr_t implementation must be identical to one found in EVIOCG*
 * libevdev ioctls. Our bitstring(3) API is compatible since r299090.
 */
_Static_assert(sizeof(bitstr_t) == sizeof(unsigned long), "bitstr_t size mismatch");

struct evdev_client;
struct evdev_mt;

#define	CURRENT_MT_SLOT(evdev)	((evdev)->ev_absinfo[ABS_MT_SLOT].value)
#define	MAXIMAL_MT_SLOT(evdev)	((evdev)->ev_absinfo[ABS_MT_SLOT].maximum)

enum evdev_key_events {
	KEY_EVENT_UP,
	KEY_EVENT_DOWN,
	KEY_EVENT_REPEAT
};

/* evdev clock IDs in Linux semantic */
enum evdev_clock_id {
	EV_CLOCK_REALTIME = 0,	/* UTC clock */
	EV_CLOCK_MONOTONIC,	/* monotonic, stops on suspend */
	EV_CLOCK_BOOTTIME	/* monotonic, suspend-awared */
};

/*
 * Locking.
 *
 * Internal  evdev structures are protected with next locks:
 * State lock		(s) - Internal state. The data it protects is changed
 *			      by incoming evdev events and some ioctls.
 * Client list epoch	(l) - Read access to client list.
 * Client list lock	(l) - Write access to client list.
 * Client queue locks	(q) - One lock per client to serialize access to data
 *			      available through character device node.
 *
 * Depending on evdev_register_() suffix evdev can run in following modes:
 * 1. Internal epoch. evdev_register(). All locks are internal.
 * 2. External epoch. Evdev expects to be run under input epoch entered by
 *    parent driver. The mode is enabled with EVDEV_FLAG_EXT_EPOCH flag.
 * 3. External mutex. evdev_register_mtx(). Evdev uses mutex provided by parent
 *    driver as both "State lock" and "Client list lock". This mode is
 *    deprecated as it causes ev_open and ev_close handlers to be called with
 *    parent driver mutex taken.
 */
#define	INPUT_EPOCH	global_epoch_preempt

enum evdev_lock_type {
	EV_LOCK_INTERNAL = 0,	/* Internal epoch */
	EV_LOCK_MTX,			/* Driver`s mutex */
};

struct evdev_dev {
	char						ev_name[NAMELEN];
	char						ev_shortname[NAMELEN];
	char						ev_serial[NAMELEN];
	int							ev_unit;

	enum evdev_lock_type		ev_lock_type;
	struct lock					ev_lock;			/* Internal state lock */
	struct lock_object			ev_mtx;
	struct input_id				ev_id;
	struct evdev_client 		*ev_grabber;						/* (s) */
	size_t						ev_report_size;

	/* Supported features: */
	bitstr_t					bit_decl(ev_prop_flags, INPUT_PROP_CNT);
	bitstr_t					bit_decl(ev_type_flags, EV_CNT);
	bitstr_t					bit_decl(ev_key_flags, KEY_CNT);
	bitstr_t					bit_decl(ev_rel_flags, REL_CNT);
	bitstr_t					bit_decl(ev_abs_flags, ABS_CNT);
	bitstr_t					bit_decl(ev_msc_flags, MSC_CNT);
	bitstr_t					bit_decl(ev_led_flags, LED_CNT);
	bitstr_t					bit_decl(ev_snd_flags, SND_CNT);
	bitstr_t					bit_decl(ev_sw_flags, SW_CNT);
	struct input_absinfo 		*ev_absinfo;						/* (s) */
	bitstr_t					bit_decl(ev_flags, EVDEV_FLAG_CNT);

	/* Repeat parameters & callout: */
	int							ev_rep[REP_CNT];					/* (s) */
	struct callout				ev_rep_callout;						/* (s) */
	uint16_t					ev_rep_key;							/* (s) */

	/* State: */
	bitstr_t					bit_decl(ev_key_states, KEY_CNT); 	/* (s) */
	bitstr_t					bit_decl(ev_led_states, LED_CNT); 	/* (s) */
	bitstr_t					bit_decl(ev_snd_states, SND_CNT); 	/* (s) */
	bitstr_t					bit_decl(ev_sw_states, SW_CNT);		/* (s) */
	bool_t						ev_report_opened;					/* (s) */

	/* KDB state: */
	bool_t						ev_kdb_active;
	bitstr_t					bit_decl(ev_kdb_led_states, LED_CNT);

	/* Multitouch protocol type B state: */
	struct evdev_mt 			*ev_mt;								/* (s) */

	/* Counters: */
	uint64_t					ev_event_count;						/* (s) */
	uint64_t					ev_report_count;					/* (s) */

	/* Parent driver callbacks: */
	const struct evdev_methods 	*ev_methods;
	void 						*ev_softc;

	/* client callback: we use wsmux for multiplexing */
	struct evdev_client 		*ev_client;
};

#define	EVDEV_LOCK(evdev)			lockmgr((evdev)->ev_lock, LK_EXCLUSIVE, NULL)  //simple_lock((evdev)->ev_lock)
#define	EVDEV_UNLOCK(evdev)			lockmgr((evdev)->ev_lock, LK_RELEASE, NULL) //simple_unlock((evdev)->ev_lock)
#define	EVDEV_LOCK_ASSERT(evdev) 	KASSERT(lockstatus((evdev)->ev_lock) != 0)
#define	EVDEV_ENTER(evdev)  do {					\
	if ((evdev)->ev_lock_type == EV_LOCK_INTERNAL)	\
		EVDEV_LOCK(evdev);							\
	else											\
		EVDEV_LOCK_ASSERT(evdev);					\
} while (0)
#define	EVDEV_EXIT(evdev)  do {						\
	if ((evdev)->ev_lock_type == EV_LOCK_INTERNAL)	\
		EVDEV_UNLOCK(evdev);						\
} while (0)

struct evdev_client {
	struct evdev_dev 			*ec_evdev;			/* evdev pointer */
	struct wsevsrc				ec_base;			/* input event to wscons event */

	struct lock					ec_buffer_lock;
	size_t						ec_buffer_size;
	size_t						ec_buffer_ready;	/* (q) */

	enum evdev_clock_id			ec_clock_id;

	struct input_event			ec_buffer[];
};

#define	EVDEV_CLIENT_LOCKQ(client)			lockmgr((client)->ec_buffer_lock, LK_EXCLUSIVE|LK_CANRECURSE, NULL)
#define	EVDEV_CLIENT_UNLOCKQ(client)		lockmgr((client)->ec_buffer_lock, LK_RELEASE, NULL)
#define	EVDEV_CLIENT_LOCKQ_ASSERT(client) 	KASSERT(lockstatus((client)->ec_buffer_lock) != 0)
#define	EVDEV_CLIENT_EMPTYQ(client) 		WSEVENT_EMPTYQ((client)->ec_base.me_evp)

/* softc */
struct evdev_softc {
	struct device 				sc_device;
	struct evdev_dev 			*sc_evdev;
	int			 				sc_evdev_state;
	int							sc_refcnt;
	u_char						sc_dying;
};

#include "locators.h"

#define EVDEVDEVCF_MUX 	0

/* Client Mux */
int		evdev_mux_open(struct wsevsrc *, struct wseventvar *);
int		evdev_mux_close(struct wsevsrc *);
int		evdev_add_mux(int, struct wsmux_softc *);

/* Input device interface: */
void 	evdev_send_event(struct evdev_dev *, uint16_t, uint16_t, int32_t);
int 	evdev_inject_event(struct evdev_dev *, uint16_t, uint16_t, int32_t);
bool_t 	evdev_event_supported(struct evdev_dev *, uint16_t);
void 	evdev_set_abs_bit(struct evdev_dev *, uint16_t);
void 	evdev_set_absinfo(struct evdev_dev *, uint16_t, struct input_absinfo *);
void 	evdev_restore_after_kdb(struct evdev_dev *);

/* Client interface: */
void	evdev_client_alloc(struct evdev_dev *, size_t);
void	evdev_client_free(struct evdev_client *);
int 	evdev_register_client(struct evdev_dev *, struct evdev_client *);
void 	evdev_dispose_client(struct evdev_dev *, struct evdev_client *);
int 	evdev_grab_client(struct evdev_dev *, struct evdev_client *);
int 	evdev_release_client(struct evdev_dev *, struct evdev_client *);
void 	evdev_client_push(struct evdev_client *, uint16_t, uint16_t, int32_t);
void 	evdev_notify_event(struct evdev_client *);
void 	evdev_revoke_client(struct evdev_client *);

/* Multitouch related functions: */
void 	evdev_mt_init(struct evdev_dev *);
void 	evdev_mt_free(struct evdev_dev *);
int32_t evdev_get_last_mt_slot(struct evdev_dev *);
void 	evdev_set_last_mt_slot(struct evdev_dev *, int32_t);
int32_t evdev_get_mt_value(struct evdev_dev *, int32_t, int16_t);
void 	evdev_set_mt_value(struct evdev_dev *, int32_t, int16_t, int32_t);
void 	evdev_send_mt_compat(struct evdev_dev *);
void 	evdev_send_mt_autorel(struct evdev_dev *);

/* Utility functions: */
void 	evdev_client_dumpqueue(struct evdev_client *);

/* evdev common routines */
void	evdev_attach_subr(struct evdev_softc *, int);
int		evdev_doactivate(struct evdev_softc *, struct device *, enum devact);
int		evdev_dodetach(struct evdev_softc *, struct device *, int);
int		evdev_doopen(struct evdev_dev *, struct proc *);
int		evdev_doclose(struct evdev_dev *, struct proc *);
int		evdev_doread(struct evdev_softc *, dev_t, struct uio *, int);
int		evdev_dowrite(struct evdev_softc *, dev_t, struct uio *, int);
int		evdev_dopoll(struct evdev_softc *, dev_t, int, struct proc *);
int		evdev_dokqfilter(struct evdev_softc *, dev_t, struct knote *);
int		evdev_doioctl(struct evdev_softc *, int, caddr_t, int, struct proc *);
#endif	/* _DEV_EVDEV_EVDEV_PRIVATE_H */
