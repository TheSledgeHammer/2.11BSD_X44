/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)device.h	8.2 (Berkeley) 2/17/94
 */

#ifndef _SYS_DEVICE_H_
#define	_SYS_DEVICE_H_

#include <sys/queue.h>

/*
 * Minimal device structures.
 * Note that all ``system'' device types are listed here.
 */
enum devclass {
	DV_DULL,		/* generic, no special info */
	DV_CPU,			/* CPU (carries resource utilization) */
	DV_DISK,		/* disk drive (label, etc) */
	DV_IFNET,		/* network interface */
	DV_TAPE,		/* tape device */
	DV_TTY			/* serial line interface (???) */
};

/*
 * Actions for cd_activate.
 */
enum devact {
	DVACT_ACTIVATE,		/* activate the device */
	DVACT_DEACTIVATE	/* deactivate the device */
};

TAILQ_HEAD(devicelist, device);
struct device {
	enum devclass		dv_class;				/* this device's classification */
	TAILQ_ENTRY(device) dv_list;				/* entry on list of all devices */
	struct cfdata		*dv_cfdata;				/* config data that found us */
	int					dv_unit;				/* device unit number */
	char				dv_xname[16];			/* external name (name + unit) */
	struct device 		*dv_parent;				/* pointer to parent device */
	int					dv_flags;				/* misc. flags; see below */
	struct cfhint 		*dv_hint;				/* config hints per device */
};

/* dv_flags */
#define	DVF_ACTIVE		0x0001					/* device is activated */

/* `event' counters (use zero or more per device instance, as needed) */
TAILQ_HEAD(evcntlist, evcnt);
struct evcnt {
	TAILQ_ENTRY(evcnt)	ev_list;				/* linked list */
	struct device 		*ev_dev;				/* associated device */
	int					ev_count;				/* how many have occurred */
	const char			*ev_name;				/* what to call them (systat display) */
	unsigned char		ev_type;				/* counter type; see below */
	unsigned char		ev_grouplen;			/* 'group' len, excluding NUL */
	unsigned char		ev_namelen;				/* 'name' len, excluding NUL */
	const struct evcnt 	*ev_parent;				/* parent, for hierarchical ctrs */
	const char			*ev_group;				/* name of group */
};

/* maximum group/name lengths, including trailing NUL */
#define	EVCNT_STRING_MAX	255

/* ev_type values */
#define	EVCNT_TYPE_MISC		0	/* miscellaneous; catch all */
#define	EVCNT_TYPE_INTR		1	/* interrupt; count with vmstat -i */
#define	EVCNT_TYPE_TRAP		2	/* processor trap/execption */

/*
 * initializer for an event count structure.  the lengths are initted and
 * it is added to the evcnt list at attach time.
 */
#define	EVCNT_INITIALIZER(type, parent, group, name) 		\
	{								                    	\
        { 0 },				/* ev_list */					\
		NULL,				/* ev_dev */					\
		0,					/* ev_count */					\
		name, 				/* ev_name */					\
		type, 	    		/* ev_type */					\
		0,			    	/* ev_grouplen */				\
		0,					/* ev_namelen */				\
		parent,				/* ev_parent */					\
		group, 				/* ev_group */					\
	}

#define	EVCNT_INITIALIZER1(type, dev, parent, group, name) 	\
	{								                    	\
        { 0 },				/* ev_list */					\
		dev,				/* ev_dev */					\
		0,					/* ev_count */					\
		name, 				/* ev_name */					\
		type, 	    		/* ev_type */					\
		0,			    	/* ev_grouplen */				\
		0,					/* ev_namelen */				\
		parent,				/* ev_parent */					\
		group, 				/* ev_group */					\
	}

/*
 * Configuration data (i.e., data placed in ioconf.c).
 */
struct cfdata {
	struct cfattach 	*cf_attach;				/* config attachment */
	struct cfdriver 	*cf_driver;				/* config driver */
	short				cf_unit;				/* unit number */
	short				cf_fstate;				/* finding state (below) */
	int					*cf_loc;				/* locators (machine dependent) */
	int					cf_flags;				/* flags from config */
	short				*cf_parents;			/* potential parents */
	const char 			**cf_locnames;			/* locator names (machine dependent) */
};
#define FSTATE_NOTFOUND	0						/* has not been found */
#define	FSTATE_FOUND	1						/* has been found */
#define	FSTATE_STAR		2						/* duplicable */

typedef int 			(*cfmatch_t)(struct device *, struct cfdata *, void *);
typedef void 			(*cfattach_t)(struct device *, struct device *, void *);
typedef int				(*cfdetach_t)(struct device *, int);
typedef int				(*cfactivate_t)(struct device *, enum devact);

/*
 * `configuration' driver (what the machine-independent autoconf uses).
 * As devices are found, they are applied against all the potential matches.
 * The one with the best match is taken, and a device structure (plus any
 * other data desired) is allocated.  Pointers to these are placed into
 * an array of pointers.  The array itself must be dynamic since devices
 * can be found long after the machine is up and running.
 */
LIST_HEAD(cfattachlist, cfattach);
struct cfattach {
	const char			*ca_name;				/* name of attachment */
	struct cfdriver     *ca_driver;             /* the cfdriver i am linked too */
	struct cfops		*ca_ops;				/* cfdriver operations: see below */
	size_t				ca_devsize;				/* size of dev data (for malloc) */
	LIST_ENTRY(cfattach)ca_list;				/* list of attached cfdriver's */
};

struct cfdriver {
	void				**cd_devs;				/* devices found */
	char				*cd_name;				/* device name */
	enum devclass 		cd_class;				/* device classification */
	void				*cd_aux;				/* additional driver, if any */
	int					cd_ndevs;				/* size of cd_devs array */
};

/* config driver operations */
struct cfops {
	int 				(*cops_match)(struct device *, struct cfdata *, void *);
	void 				(*cops_attach)(struct device *, struct device *, void *);
	int 				(*cops_detach)(struct device *, int);
	int 				(*cops_activate)(struct device *, enum devact);
};

/* Flags given to config_detach(), and the ca_detach function. */
#define	DETACH_FORCE	0x01					/* force detachment; hardware gone */
#define	DETACH_QUIET	0x02					/* don't print a notice */

/*
 * Configuration printing functions, and their return codes.  The second
 * argument is NULL if the device was configured; otherwise it is the name
 * of the parent device.  The return value is ignored if the device was
 * configured, so most functions can return UNCONF unconditionally.
 */
typedef int 			(*cfprint_t)(void *, const char *);
#define	QUIET			0		/* print nothing */
#define	UNCONF			1		/* print " not configured\n" */
#define	UNSUPP			2		/* print " not supported\n" */

/*
 * Pseudo-device attach information (function + number of pseudo-devs).
 */
struct pdevinit {
	void				(*pdev_attach)(int);
	int					pdev_count;
};

/* deferred config queues */
TAILQ_HEAD(deferred_config_head, deferred_config);
struct deferred_config {
	TAILQ_ENTRY(deferred_config) 	dc_queue;
	struct device 					*dc_dev;
	void 							(*dc_func)(struct device *);
};

/* cfattach, cfdriver and cfops declarations */
#define CFOPS_DECL(name, matfn, attfn, detfn, actfn) 	\
	struct cfops (name##_cops) = {						\
			.cops_match = (matfn),						\
			.cops_attach = (attfn),						\
			.cops_detach = (detfn),						\
			.cops_activate = (actfn),					\
	}

#define CFDRIVER_DECL(devs, name, clas)					\
	struct cfdriver (name##_cd) = { 					\
			.cd_devs = (devs),							\
			.cd_name = (#name),							\
			.cd_class = (clas), 						\
	}

#define CFATTACH_DECL(name, driver, cops, size)			\
	struct cfattach (name##_ca) = {						\
			.ca_name = (#name),							\
			.ca_driver = (driver),						\
			.ca_ops = (cops),							\
			.ca_devsize = (size),						\
			.ca_list = { 0 },							\
	}

/*
 * Attach a static event counter.  This uses a link set to do the work.
 * NOTE: "ev" should not be a pointer to the object, but rather a direct
 * reference to the object itself.
 */
#define	EVCNT_ATTACH_STATIC(ev)		__link_set_add_data(evcnts, ev)

extern struct devicelist			alldevs;				/* head of list of all devices */
extern struct cfattachlist			allattachs;				/* head of list of all attachments */
extern struct deferred_config_head	deferred_config_queue;	/* head of deferred queue */
extern struct deferred_config_head	interrupt_config_queue;	/* head of interrupt queue */
extern struct evcntlist				allevents;				/* head of list of all events */

int				config_match(struct device *, struct cfdata *, void *);
struct cfdata 	*config_search(cfmatch_t, struct device *, void *);
struct cfdata 	*config_rootsearch(cfmatch_t, char *, void *);
struct device 	*config_found(struct device *, void *, cfprint_t);
struct device 	*config_found_sm(struct device *, void *, cfprint_t, cfmatch_t);
struct device 	*config_rootfound(char *, void *);
struct device 	*config_attach(struct device *, struct cfdata *, void *, cfprint_t);
int				config_detach(struct device *, int);
int 			config_activate(struct device *);
int 			config_deactivate(struct device *);
void 			config_defer(struct device *, void (*)(struct device *));
void 			config_interrupts(struct device *, void (*)(struct device *));
void 			config_pending_incr(void);
void 			config_pending_decr(void);
void 			evcnt_attach(struct evcnt *, struct device *, const char *, const char *);
void 			evcnt_detach(struct evcnt *);
void 			evcnt_attach_static(struct evcnt *);
void 			evcnt_attach_dynamic(struct evcnt *, int, const struct evcnt *, const char *, const char *);
int     		config_hint_enabled(struct device *);
int     		config_hint_disabled(struct device *);

#endif /* !_SYS_DEVICE_H_ */
