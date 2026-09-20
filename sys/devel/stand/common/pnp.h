/*-
 * Copyright (c) 1998 Michael Smith <msmith@freebsd.org>
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

#ifndef _PNP_H_
#define _PNP_H_

#include <sys/types.h>
#include <sys/queue.h>

/*
 * Plug-and-play enumerator/configurator interface.
 */
struct pnphandler {
    const char				*pp_name;					/* handler/bus name */
    void					(*pp_enumerate)(void);		/* enumerate PnP devices, add to chain */
};

struct pnpident {
    char					*id_ident;		/* ASCII identifier, actual format varies with bus/handler */
    STAILQ_ENTRY(pnpident)	id_link;
};

struct pnpinfo {
    char					*pi_desc;		/* ASCII description, optional */
    int						pi_revision;	/* optional revision (or -1) if not supported */
    char					*pi_module;		/* module/args nominated to handle device */
    int						pi_argc;		/* module arguments */
    char					**pi_argv;
    struct pnphandler		*pi_handler;	/* handler which detected this device */
    STAILQ_HEAD(, pnpident) pi_ident;		/* list of identifiers */
    STAILQ_ENTRY(pnpinfo)	pi_link;
};

STAILQ_HEAD(pnpinfo_stql, pnpinfo);

extern struct pnphandler *pnphandlers[];	/* provided by MD code */

/*
 *  < 0	- No ISA in system
 * == 0	- Maybe ISA, search for read data port
 *  > 0	- ISA in system, value is read data port address
 */
extern int isapnp_readport;

/* pnp.c */
void pnp_addident(struct pnpinfo *, char *);
struct pnpinfo *pnp_allocinfo(void);
void pnp_freeinfo(struct pnpinfo *);
void pnp_addinfo(struct pnpinfo *);
char *pnp_eisaformat(uint8_t *);

#endif /* !_PNP_H_ */
