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

struct device {
	enum devclass		dv_class;				/* this device's classification */
	struct	device 		*dv_next;				/* next in list of all */
	struct	device 		*dv_prev;				/* prev in list of all */
	struct	cfdata		*dv_cfdata;				/* config data that found us */
	int					dv_unit;				/* device unit number */
	char				dv_xname[16];			/* external name (name + unit) */
	struct	device 		*dv_parent;				/* pointer to parent device */
	int					dv_flags;				/* misc. flags; see below */
	struct cfhint 		*dv_hint;
};

/* dv_flags */
#define	DVF_ACTIVE		0x0001					/* device is activated */

/* `event' counters (use zero or more per device instance, as needed) */
struct evcnt {
	struct	evcnt 		*ev_next;				/* linked list */
	struct	device 		*ev_dev;				/* associated device */
	int					ev_count;				/* how many have occurred */
	char				ev_name[8];				/* what to call them (systat display) */
};

/*
 * Configuration data (i.e., data placed in ioconf.c).
 */
struct cfdata {
	struct	cfdriver 	*cf_driver;				/* config driver */
	short				cf_unit;				/* unit number */
	short				cf_fstate;				/* finding state (below) */
	int					*cf_loc;				/* locators (machine dependent) */
	int					cf_flags;				/* flags from config */
	short				*cf_parents;			/* potential parents */
	void				(**cf_ivstubs)();		/* config-generated vectors, if any */
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
struct cfdriver {
	void				**cd_devs;				/* devices found */
	char				*cd_name;				/* device name */
	struct cfops		*cd_ops;				/* config driver operations: see below */
	enum devclass 		cd_class;				/* device classification */
	size_t				cd_devsize;				/* size of dev data (for malloc) */
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
 * Actions for cd_activate.
 */
enum devact {
	DVACT_ACTIVATE,		/* activate the device */
	DVACT_DEACTIVATE	/* deactivate the device */
};

/*
 * Configuration printing functions, and their return codes.  The second
 * argument is NULL if the device was configured; otherwise it is the name
 * of the parent device.  The return value is ignored if the device was
 * configured, so most functions can return UNCONF unconditionally.
 */
typedef int 			(*cfprint_t) (void *, char *);
#define	QUIET			0		/* print nothing */
#define	UNCONF			1		/* print " not configured\n" */
#define	UNSUPP			2		/* print " not supported\n" */

/*
 * Pseudo-device attach information (function + number of pseudo-devs).
 */
struct pdevinit {
	void				(*pdev_attach) (int);
	int					pdev_count;
};

/* configure hints */
struct cfhint {
	char				*ch_name;		/* device name */
	int					ch_unit;		/**< current unit number */
	char*				ch_nameunit;	/**< name+unit e.g. foodev0 */
	int					ch_rescount;
	struct cfresource	*ch_resources;
};

typedef enum {
	RES_INT, RES_STRING, RES_LONG
} resource_type;

struct cfresource {
	char 				*cr_name;
	resource_type		cr_type;
	union {
		long			longval;
		int				intval;
		char*			stringval;
	} cr_u;
};

/* cfdriver and cfops declarations */
#define CFDRIVER_DECL(devs, name, cops, class, size) 	\
	struct cfdriver (name##_cd) = { (devs), (#name), (cops), (class), (size) }

#define CFOPS_DECL(name, matfn, attfn, detfn, actfn) 	\
	struct cfops (name##_cops) = { (#name), (matfn), (attfn), (detfn), (actfn) }

struct	device 	*alldevs;			/* head of list of all devices */
struct	evcnt 	*allevents;			/* head of list of all events */
struct cfhint 	*allhints;			/* head of list of device hints */
int 			cfhint_count; 		/* hint count */

int				config_match(struct device *, struct cfdata *, void *);
struct cfdata 	*config_search (cfmatch_t, struct device *, void *);
struct cfdata 	*config_rootsearch (cfmatch_t, char *, void *);
int 			config_found (struct device *, void *, cfprint_t);
struct device 	*config_found_sm(struct device *, void *, cfprint_t, cfmatch_t);
int 			config_rootfound (char *, void *);
void 			config_attach (struct device *, struct cfdata *, void *, cfprint_t);
int				config_detach(struct device *, int);
int 			config_activate (struct device *);
int 			config_deactivate (struct device *);
int     		config_hint_enabled(struct device *);
int     		config_hint_disabled(struct device *);
void 			evcnt_attach (struct device *, const char *, struct evcnt *);

/* Access functions for device resources. */
int				resource_int_value(const char *name, int unit, const char *resname, int *result);
int				resource_long_value(const char *name, int unit, const char *resname, long *result);
int				resource_string_value(const char *name, int unit, const char *resname, const char **result);
int     		resource_enabled(const char *name, int unit);
int     		resource_disabled(const char *name, int unit);
int				resource_query_string(int i, const char *resname, const char *value);
char			*resource_query_name(int i);
int				resource_query_unit(int i);
int				resource_locate(int i, const char *resname);
int				resource_set_int(const char *name, int unit, const char *resname, int value);
int				resource_set_long(const char *name, int unit, const char *resname, long value);
int				resource_set_string(const char *name, int unit, const char *resname, const char *value);
int				resource_count(void);
void			resource_init();

#endif /* !_SYS_DEVICE_H_ */
