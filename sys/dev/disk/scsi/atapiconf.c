/*	$NetBSD: atapiconf.c,v 1.63 2003/10/17 00:19:46 mycroft Exp $	*/

/*
 * Copyright (c) 1996, 2001 Manuel Bouyer.  All rights reserved.
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
 *	This product includes software developed by Manuel Bouyer.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: atapiconf.c,v 1.63 2003/10/17 00:19:46 mycroft Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/kthread.h>

#include <dev/disk/scsi/scsipi_all.h>
#include <dev/disk/scsi/scsipiconf.h>
#include <dev/disk/scsi/atapiconf.h>

#include "locators.h"

#define SILENT_PRINTF(flags,string) if (!(flags & A_SILENT)) printf string
#define MAX_TARGET 1

const struct scsipi_periphsw atapi_probe_periphsw = {
	NULL,
	NULL,
	NULL,
	NULL,
};

int		atapibusmatch(struct device *, struct cfdata *, void *);
void	atapibusattach(struct device *, struct device *, void *);
int		atapibusactivate(struct device *, enum devact);
int		atapibusdetach(struct device *, int flags);

int		atapibussubmatch(struct device *, struct cfdata *, void *);

int		atapi_probe_bus(struct atapibus_softc *, int);

CFOPS_DECL(atapibus, atapibusmatch, atapibusattach, atapibusdetach, atapibusactivate);
CFDRIVER_DECL(NULL, atapibus, DV_DISK);
CFATTACH_DECL(atapibus, &atapibus_cd, &atapibus_cops, sizeof(struct atapibus_softc));

extern struct cfdriver atapibus_cd;

int atapibusprint (void *, const char *);

const struct scsi_quirk_inquiry_pattern atapi_quirk_patterns[] = {
	{{T_CDROM, T_REMOV,
	 "ALPS ELECTRIC CO.,LTD. DC544C", "", "SW03D"},	PQUIRK_NOTUR},
	{{T_CDROM, T_REMOV,
	 "CR-2801TE", "", "1.07"},		PQUIRK_NOSENSE},
	{{T_CDROM, T_REMOV,
	 "CREATIVECD3630E", "", "AC101"},	PQUIRK_NOSENSE},
	{{T_CDROM, T_REMOV,
	 "FX320S", "", "q01"},			PQUIRK_NOSENSE},
	{{T_CDROM, T_REMOV,
	 "GCD-R580B", "", "1.00"},		PQUIRK_LITTLETOC},
	{{T_CDROM, T_REMOV,
	 "HITACHI CDR-7730", "", "0008a"},      PQUIRK_NOSENSE},
	{{T_CDROM, T_REMOV,
	 "MATSHITA CR-574", "", "1.02"},	PQUIRK_NOCAPACITY},
	{{T_CDROM, T_REMOV,
	 "MATSHITA CR-574", "", "1.06"},	PQUIRK_NOCAPACITY},
	{{T_CDROM, T_REMOV,
	 "Memorex CRW-2642", "", "1.0g"},	PQUIRK_NOSENSE},
	{{T_CDROM, T_REMOV,
	 "NEC                 CD-ROM DRIVE:273", "", "4.21"}, PQUIRK_NOTUR},
	{{T_CDROM, T_REMOV,
	 "SANYO CRD-256P", "", "1.02"},		PQUIRK_NOCAPACITY},
	{{T_CDROM, T_REMOV,
	 "SANYO CRD-254P", "", "1.02"},		PQUIRK_NOCAPACITY},
	{{T_CDROM, T_REMOV,
	 "SANYO CRD-S54P", "", "1.08"},		PQUIRK_NOCAPACITY},
	{{T_CDROM, T_REMOV,
	 "CD-ROM  CDR-S1", "", "1.70"},		PQUIRK_NOCAPACITY}, /* Sanyo */
	{{T_CDROM, T_REMOV,
	 "CD-ROM  CDR-N16", "", "1.25"},	PQUIRK_NOCAPACITY}, /* Sanyo */
};

int
atapiprint(aux, pnp)
	void *aux;
	const char *pnp; 
{
	if (pnp)
		printf("atapibus at %s", pnp);
	return (UNCONF);
}

int
atapibusmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct scsipi_channel *chan = aux;

	if (chan == NULL)
		return (0);

	if (chan->chan_bustype->bustype_type != SCSIPI_BUSTYPE_ATAPI)
		return (0);

	return (1);
}

int
atapibussubmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct scsipibus_attach_args *sa = aux;
	struct scsipi_periph *periph = sa->sa_periph;

	if (cf->cf_loc[ATAPIBUSCF_DRIVE] != ATAPIBUSCF_DRIVE_DEFAULT &&
	    cf->cf_loc[ATAPIBUSCF_DRIVE] != periph->periph_target)
		return (0);
	return (config_match(parent, cf, aux));
}

void
atapibusattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct atapibus_softc *sc = (void *) self;
	struct scsipi_channel *chan = aux;

	sc->sc_channel = chan;

	chan->chan_name = sc->sc_dev.dv_xname;

	/* ATAPI has no LUNs. */
	chan->chan_nluns = 1;
	printf(": %d targets\n", chan->chan_ntargets);

	/* Initialize the channel. */
	chan->chan_init_cb = NULL;
	chan->chan_init_cb_arg = NULL;
	scsipi_channel_init(chan);

	/* Probe the bus for devices. */
	atapi_probe_bus(sc, -1);
}

int
atapibusactivate(self, act)
	struct device *self;
	enum devact act;
{
	struct atapibus_softc *sc = (void *) self;
	struct scsipi_channel *chan = sc->sc_channel;
	struct scsipi_periph *periph;
	int target, error = 0, s;

	s = splbio();
	switch (act) {
	case DVACT_ACTIVATE:
		error = EOPNOTSUPP;
		break;

	case DVACT_DEACTIVATE:
		for (target = 0; target < chan->chan_ntargets; target++) {
			periph = scsipi_lookup_periph(chan, target, 0);
			if (periph == NULL)
				continue;
			error = config_deactivate(periph->periph_dev);
			if (error)
				goto out;
		}
		break;
	}
 out:
	splx(s);
	return (error);
}

int
atapibusdetach(self, flags)
	struct device *self;
	int flags;
{
	struct atapibus_softc *sc = (void *)self;
	struct scsipi_channel *chan = sc->sc_channel;
	struct scsipi_periph *periph;
	int target, error;

	/*
	 * Shut down the channel.
	 */
	scsipi_channel_shutdown(chan);

	/*
	 * Now detach all of the periphs.
	 */
	for (target = 0; target < chan->chan_ntargets; target++) {
		periph = scsipi_lookup_periph(chan, target, 0);
		if (periph == NULL)
			continue;
		error = config_detach(periph->periph_dev, flags);
		if (error)
			return (error);

		scsipi_remove_periph(chan, periph);
		free(periph, M_DEVBUF);
	}
	return (0);
}

int
atapi_probe_bus(sc, target)
	struct atapibus_softc *sc;
	int target;
{
	struct scsipi_channel *chan = sc->sc_channel;
	int maxtarget, mintarget;
	int error;
	struct atapi_adapter *atapi_adapter;

	if (target == -1) {
		maxtarget = 1;
		mintarget = 0;
	} else {
		if (target < 0 || target >= chan->chan_ntargets)
			return (ENXIO);
		maxtarget = mintarget = target;
	}

	if ((error = scsipi_adapter_addref(chan->chan_adapter)) != 0)
		return (error);
	atapi_adapter = (struct atapi_adapter*)chan->chan_adapter;
	for (target = mintarget; target <= maxtarget; target++)
		atapi_adapter->atapi_probe_device(sc, target);
	scsipi_adapter_delref(chan->chan_adapter);
	return (0);
}

void *
atapi_probe_device(sc, target, periph, sa)
	struct atapibus_softc *sc;
	int target;
	struct scsipi_periph *periph;
	struct scsipibus_attach_args *sa;
{
	struct scsipi_channel *chan = sc->sc_channel;
	struct scsi_quirk_inquiry_pattern *finger;
	struct cfdata *cf;
	int priority, quirks;

	finger = (struct scsi_quirk_inquiry_pattern *)scsipi_inqmatch(
	    &sa->sa_inqbuf, (caddr_t)atapi_quirk_patterns,
	    sizeof(atapi_quirk_patterns) /
	        sizeof(atapi_quirk_patterns[0]),
	    sizeof(atapi_quirk_patterns[0]), &priority);

	if (finger != NULL)
		quirks = finger->quirks;
	else
		quirks = 0;

	/*
	 * Now apply any quirks from the table.
	 */
	periph->periph_quirks |= quirks;

	if ((cf = config_search(atapibussubmatch, &sc->sc_dev,
	    sa)) != 0) {
		scsipi_insert_periph(chan, periph);
		/*
		 * XXX Can't assign periph_dev here, because we'll
		 * XXX need it before config_attach() returns.  Must
		 * XXX assign it in periph driver.
		 */
		return config_attach(&sc->sc_dev, cf, sa,
		    atapibusprint);
	} else {
		atapibusprint(sa, sc->sc_dev.dv_xname);
		printf(" not configured\n");
		free(periph, M_DEVBUF);
		return NULL;
	}
}

int
atapibusprint(aux, pnp)
	void *aux;
	const char *pnp;
{
	struct scsipibus_attach_args *sa = aux;
	struct scsipi_inquiry_pattern *inqbuf;
	char *dtype;

	if (pnp != NULL)
		printf("%s", pnp);

	inqbuf = &sa->sa_inqbuf;

	dtype = scsipi_dtype(inqbuf->type & SID_TYPE);
	printf(" drive %d: <%s, %s, %s> %s %s",
	    sa->sa_periph->periph_target, inqbuf->vendor,
	    inqbuf->product, inqbuf->revision, dtype,
	    inqbuf->removable ? "removable" : "fixed");
	return (UNCONF);
}
