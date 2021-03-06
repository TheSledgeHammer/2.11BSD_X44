/*	$NetBSD: ch.c,v 1.21.4.1 1996/11/14 08:36:25 thorpej Exp $	*/

/*
 * Copyright (c) 1996 Jason R. Thorpe <thorpej@and.com>
 * All rights reserved.
 *
 * Partially based on an autochanger driver written by Stefan Grefen
 * and on an autochanger driver written by the Systems Programming Group
 * at the University of Utah Computer Science Department.
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
 *    must display the following acknowledgements:
 *	This product includes software developed by Jason R. Thorpe
 *	for And Communications, http://www.and.com/
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/chio.h> 
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/conf.h>
#include <sys/fcntl.h>

#include <dev/disk/scsi/scsi_all.h>
#include <dev/disk/scsi/scsi_changer.h>
#include <dev/disk/scsi/scsiconf.h>

#define CHRETRIES	2
#define CHUNIT(x)	(minor((x)))

struct ch_softc {
	struct device		sc_dev;		/* generic device info */
	struct scsi_link 	*sc_link;	/* link in the SCSI bus */

	int					sc_picker;	/* current picker */

	/*
	 * The following information is obtained from the
	 * element address assignment page.
	 */
	int					sc_firsts[4];	/* firsts, indexed by CHET_* */
	int					sc_counts[4];	/* counts, indexed by CHET_* */

	/*
	 * The following mask defines the legal combinations
	 * of elements for the MOVE MEDIUM command.
	 */
	u_int8_t			sc_movemask[4];

	/*
	 * As above, but for EXCHANGE MEDIUM.
	 */
	u_int8_t			sc_exchangemask[4];

	int					flags;		/* misc. info */
};

/* sc_flags */
#define CHF_ROTATE	0x01		/* picker can rotate */

/* Autoconfiguration glue */
int		chmatch (struct device *, void *, void *);
void	chattach (struct device *, struct device *, void *);

struct cfdriver ch_cd = {
	NULL, "ch", chmatch, chattach, DV_DULL, sizeof(struct ch_softc)
};

struct scsi_inquiry_pattern ch_patterns[] =
		{ { T_CHANGER, T_REMOV, "", "", "" }, };


static dev_type_open(chopen);
static dev_type_close(chclose);
static dev_type_ioctl(chioctl);
static dev_type_poll(chpoll);

const struct cdevsw ch_cdevsw = {
	.d_open = chopen,
	.d_close = chclose,
	.d_read = noread,
	.d_write = nowrite,
	.d_ioctl = chioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = chpoll,
	.d_mmap = nommap,
	.d_discard = nodiscard,
	.d_type = D_OTHER
};


/* SCSI glue */
struct scsi_device ch_switch = {
	NULL, NULL, NULL, NULL
};

int	ch_move (struct ch_softc *, struct changer_move *);
int	ch_exchange (struct ch_softc *, struct changer_exchange *);
int	ch_position (struct ch_softc *, struct changer_position *);
int	ch_usergetelemstatus (struct ch_softc *, int, u_int8_t *);
int	ch_getelemstatus (struct ch_softc *, int, int, caddr_t, size_t);
int	ch_get_params (struct ch_softc *, int);

int
chmatch(parent, match, aux)
	struct device *parent;
	void *match, *aux;
{
	struct scsibus_attach_args *sa = aux;
	int priority;

	(void)scsi_inqmatch(sa->sa_inqbuf,
	    (caddr_t)ch_patterns, sizeof(ch_patterns)/sizeof(ch_patterns[0]),
	    sizeof(ch_patterns[0]), &priority);

	return (priority);
}

void
chattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct ch_softc *sc = (struct ch_softc *)self;
	struct scsibus_attach_args *sa = aux;
	struct scsi_link *link = sa->sa_sc_link;

	/* Glue into the SCSI bus */
	sc->sc_link = link;
	link->device = &ch_switch;
	link->device_softc = sc;
	link->openings = 1;

	printf("\n");

	/*
	 * Get information about the device.  Note we can't use
	 * interrupts yet.
	 */
	if (ch_get_params(sc, SCSI_AUTOCONF))
		printf("%s: offline\n", sc->sc_dev.dv_xname);
	else {
		printf("%s: %d slot%s, %d drive%s, %d picker%s", sc->sc_dev.dv_xname,
				sc->sc_counts[CHET_ST], (sc->sc_counts[CHET_ST] > 1) ? "s" : "",
				sc->sc_counts[CHET_DT], (sc->sc_counts[CHET_DT] > 1) ? "s" : "",
				sc->sc_counts[CHET_MT],
				(sc->sc_counts[CHET_MT] > 1) ? "s" : "");
		if (sc->sc_counts[CHET_IE])
			printf(", %d portal%s", sc->sc_counts[CHET_IE],
					(sc->sc_counts[CHET_IE] > 1) ? "s" : "");
		printf("\n");
#ifdef CHANGER_DEBUG
		printf("%s: move mask: 0x%x 0x%x 0x%x 0x%x\n",
		    sc->sc_dev.dv_xname,
		    sc->sc_movemask[CHET_MT], sc->sc_movemask[CHET_ST],
		    sc->sc_movemask[CHET_IE], sc->sc_movemask[CHET_DT]);
		printf("%s: exchange mask: 0x%x 0x%x 0x%x 0x%x\n",
		    sc->sc_dev.dv_xname,
		    sc->sc_exchangemask[CHET_MT], sc->sc_exchangemask[CHET_ST],
		    sc->sc_exchangemask[CHET_IE], sc->sc_exchangemask[CHET_DT]);
#endif /* CHANGER_DEBUG */
	}

	/* Default the current picker. */
	sc->sc_picker = sc->sc_firsts[CHET_MT];
}

int
chopen(dev, flags, fmt, p)
	dev_t dev;
	int flags, fmt;
	struct proc *p;
{
	struct ch_softc *sc;
	int unit, error = 0;

	unit = CHUNIT(dev);
	if ((unit >= ch_cd.cd_ndevs) || ((sc = ch_cd.cd_devs[unit]) == NULL))
		return (ENXIO);

	/*
	 * Only allow one open at a time.
	 */
	if (sc->sc_link->flags & SDEV_OPEN)
		return (EBUSY);

	sc->sc_link->flags |= SDEV_OPEN;

	/*
	 * Absorb any unit attention errors.  Ignore "not ready"
	 * since this might occur if e.g. a tape isn't actually
	 * loaded in the drive.
	 */
	error = scsi_test_unit_ready(sc->sc_link,
	SCSI_IGNORE_NOT_READY | SCSI_IGNORE_MEDIA_CHANGE);
	if (error)
		goto bad;

	/*
	 * Make sure our parameters are up to date.
	 */
	if ((error = ch_get_params(sc, 0)) != 0)
		goto bad;

	return (0);

	bad: sc->sc_link->flags &= ~SDEV_OPEN;
	return (error);
}

int
chclose(dev, flags, fmt, p)
	dev_t dev;
	int flags, fmt;
	struct proc *p;
{
	struct ch_softc *sc = ch_cd.cd_devs[CHUNIT(dev)];

	sc->sc_link->flags &= ~SDEV_OPEN;
	return (0);
}

int
chioctl(dev, cmd, data, flags, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flags;
	struct proc *p;
{
	struct ch_softc *sc = ch_cd.cd_devs[CHUNIT(dev)];
	int error = 0;

	/*
	 * If this command can change the device's state, we must
	 * have the device open for writing.
	 */
	switch (cmd) {
	case CHIOGPICKER:
	case CHIOGPARAMS:
	case CHIOGSTATUS:
		break;

	default:
		if ((flags & FWRITE) == 0)
			return (EBADF);
	}

	switch (cmd) {
	case CHIOMOVE:
		error = ch_move(sc, (struct changer_move*) data);
		break;

	case CHIOEXCHANGE:
		error = ch_exchange(sc, (struct changer_exchange*) data);
		break;

	case CHIOPOSITION:
		error = ch_position(sc, (struct changer_position*) data);
		break;

	case CHIOGPICKER:
		*(int*) data = sc->sc_picker - sc->sc_firsts[CHET_MT];
		break;

	case CHIOSPICKER: {
		int new_picker = *(int*) data;

		if (new_picker > (sc->sc_counts[CHET_MT] - 1))
			return (EINVAL);
		sc->sc_picker = sc->sc_firsts[CHET_MT] + new_picker;
		break;
	}

	case CHIOGPARAMS: {
		struct changer_params *cp = (struct changer_params*) data;

		cp->cp_curpicker = sc->sc_picker - sc->sc_firsts[CHET_MT];
		cp->cp_npickers = sc->sc_counts[CHET_MT];
		cp->cp_nslots = sc->sc_counts[CHET_ST];
		cp->cp_nportals = sc->sc_counts[CHET_IE];
		cp->cp_ndrives = sc->sc_counts[CHET_DT];
		break;
	}

	case CHIOGSTATUS: {
		struct changer_element_status *ces =
				(struct changer_element_status*) data;

		error = ch_usergetelemstatus(sc, ces->ces_type, ces->ces_data);
		break;
	}

		/* Implement prevent/allow? */

	default:
		error = scsi_do_ioctl(sc->sc_link, dev, cmd, data, flags, p);
		break;
	}

	return (error);
}

int
ch_move(sc, cm)
	struct ch_softc *sc;
	struct changer_move *cm;
{
	struct scsi_move_medium cmd;
	u_int16_t fromelem, toelem;

	/*
	 * Check arguments.
	 */
	if ((cm->cm_fromtype > CHET_DT) || (cm->cm_totype > CHET_DT))
		return (EINVAL);
	if ((cm->cm_fromunit > (sc->sc_counts[cm->cm_fromtype] - 1))
			|| (cm->cm_tounit > (sc->sc_counts[cm->cm_totype] - 1)))
		return (ENODEV);

	/*
	 * Check the request against the changer's capabilities.
	 */
	if ((sc->sc_movemask[cm->cm_fromtype] & (1 << cm->cm_totype)) == 0)
		return (EINVAL);

	/*
	 * Calculate the source and destination elements.
	 */
	fromelem = sc->sc_firsts[cm->cm_fromtype] + cm->cm_fromunit;
	toelem = sc->sc_firsts[cm->cm_totype] + cm->cm_tounit;

	/*
	 * Build the SCSI command.
	 */
	bzero(&cmd, sizeof(cmd));
	cmd.opcode = MOVE_MEDIUM;
	_lto2b(sc->sc_picker, cmd.tea);
	_lto2b(fromelem, cmd.src);
	_lto2b(toelem, cmd.dst);
	if (cm->cm_flags & CM_INVERT)
		cmd.flags |= MOVE_MEDIUM_INVERT;

	/*
	 * Send command to changer.
	 */
	return (scsi_scsi_cmd(sc->sc_link, (struct scsi_generic*) &cmd, sizeof(cmd),
			NULL, 0, CHRETRIES, 100000, NULL, 0));
}

int
ch_exchange(sc, ce)
	struct ch_softc *sc;
	struct changer_exchange *ce;
{
	struct scsi_exchange_medium cmd;
	u_int16_t src, dst1, dst2;

	/*
	 * Check arguments.
	 */
	if ((ce->ce_srctype > CHET_DT) || (ce->ce_fdsttype > CHET_DT)
			|| (ce->ce_sdsttype > CHET_DT))
		return (EINVAL);
	if ((ce->ce_srcunit > (sc->sc_counts[ce->ce_srctype] - 1))
			|| (ce->ce_fdstunit > (sc->sc_counts[ce->ce_fdsttype] - 1))
			|| (ce->ce_sdstunit > (sc->sc_counts[ce->ce_sdsttype] - 1)))
		return (ENODEV);

	/*
	 * Check the request against the changer's capabilities.
	 */
	if (((sc->sc_exchangemask[ce->ce_srctype] & (1 << ce->ce_fdsttype)) == 0)
			|| ((sc->sc_exchangemask[ce->ce_fdsttype] & (1 << ce->ce_sdsttype))
					== 0))
		return (EINVAL);

	/*
	 * Calculate the source and destination elements.
	 */
	src = sc->sc_firsts[ce->ce_srctype] + ce->ce_srcunit;
	dst1 = sc->sc_firsts[ce->ce_fdsttype] + ce->ce_fdstunit;
	dst2 = sc->sc_firsts[ce->ce_sdsttype] + ce->ce_sdstunit;

	/*
	 * Build the SCSI command.
	 */
	bzero(&cmd, sizeof(cmd));
	cmd.opcode = EXCHANGE_MEDIUM;
	_lto2b(sc->sc_picker, cmd.tea);
	_lto2b(src, cmd.src);
	_lto2b(dst1, cmd.fdst);
	_lto2b(dst2, cmd.sdst);
	if (ce->ce_flags & CE_INVERT1)
		cmd.flags |= EXCHANGE_MEDIUM_INV1;
	if (ce->ce_flags & CE_INVERT2)
		cmd.flags |= EXCHANGE_MEDIUM_INV2;

	/*
	 * Send command to changer.
	 */
	return (scsi_scsi_cmd(sc->sc_link, (struct scsi_generic*) &cmd, sizeof(cmd),
			NULL, 0, CHRETRIES, 100000, NULL, 0));
}

int
ch_position(sc, cp)
	struct ch_softc *sc;
	struct changer_position *cp;
{
	struct scsi_position_to_element cmd;
	u_int16_t dst;

	/*
	 * Check arguments.
	 */
	if (cp->cp_type > CHET_DT)
		return (EINVAL);
	if (cp->cp_unit > (sc->sc_counts[cp->cp_type] - 1))
		return (ENODEV);

	/*
	 * Calculate the destination element.
	 */
	dst = sc->sc_firsts[cp->cp_type] + cp->cp_unit;

	/*
	 * Build the SCSI command.
	 */
	bzero(&cmd, sizeof(cmd));
	cmd.opcode = POSITION_TO_ELEMENT;
	_lto2b(sc->sc_picker, cmd.tea);
	_lto2b(dst, cmd.dst);
	if (cp->cp_flags & CP_INVERT)
		cmd.flags |= POSITION_TO_ELEMENT_INVERT;

	/*
	 * Send command to changer.
	 */
	return (scsi_scsi_cmd(sc->sc_link, (struct scsi_generic*) &cmd, sizeof(cmd),
			NULL, 0, CHRETRIES, 100000, NULL, 0));
}

/*
 * Perform a READ ELEMENT STATUS on behalf of the user, and return to
 * the user only the data the user is interested in (i.e. an array of
 * flags bytes).
 */
int
ch_usergetelemstatus(sc, chet, uptr)
	struct ch_softc *sc;
	int chet;
	u_int8_t *uptr;
{
	struct read_element_status_header *st_hdr;
	struct read_element_status_page_header *pg_hdr;
	struct read_element_status_descriptor *desc;
	caddr_t data = NULL;
	size_t size, desclen;
	int avail, i, error = 0;
	u_int8_t *user_data = NULL;

	/*
	 * If there are no elements of the requested type in the changer,
	 * the request is invalid.
	 */
	if (sc->sc_counts[chet] == 0)
		return (EINVAL);

	/*
	 * Request one descriptor for the given element type.  This
	 * is used to determine the size of the descriptor so that
	 * we can allocate enough storage for all of them.  We assume
	 * that the first one can fit into 1k.
	 */
	data = (caddr_t) malloc(1024, M_DEVBUF, M_WAITOK);
	error = ch_getelemstatus(sc, sc->sc_firsts[chet], 1, data, 1024);
	if (error)
		goto done;

	st_hdr = (struct read_element_status_header*) data;
	pg_hdr = (struct read_element_status_page_header*) ((u_long) st_hdr
			+ sizeof(struct read_element_status_header));
	desclen = _2btol(pg_hdr->edl);

	size = sizeof(struct read_element_status_header)
			+ sizeof(struct read_element_status_page_header)
			+ (desclen * sc->sc_counts[chet]);

	/*
	 * Reallocate storage for descriptors and get them from the
	 * device.
	 */
	free(data, M_DEVBUF);
	data = (caddr_t) malloc(size, M_DEVBUF, M_WAITOK);
	error = ch_getelemstatus(sc, sc->sc_firsts[chet], sc->sc_counts[chet], data,
			size);
	if (error)
		goto done;

	/*
	 * Fill in the user status array.
	 */
	st_hdr = (struct read_element_status_header*) data;
	avail = _2btol(st_hdr->count);
	if (avail != sc->sc_counts[chet])
		printf("%s: warning, READ ELEMENT STATUS avail != count\n",
				sc->sc_dev.dv_xname);

	user_data = (u_int8_t*) malloc(avail, M_DEVBUF, M_WAITOK);

	desc = (struct read_element_status_descriptor*) ((u_long) data
			+ sizeof(struct read_element_status_header)
			+ sizeof(struct read_element_status_page_header));
	for (i = 0; i < avail; ++i) {
		user_data[i] = desc->flags1;
		(u_long) desc += desclen;
	}

	/* Copy flags array out to userspace. */
	error = copyout(user_data, uptr, avail);

	done: if (data != NULL)
		free(data, M_DEVBUF);
	if (user_data != NULL)
		free(user_data, M_DEVBUF);
	return (error);
}

int
ch_getelemstatus(sc, first, count, data, datalen)
	struct ch_softc *sc;
	int first, count;
	caddr_t data;
	size_t datalen;
{
	struct scsi_read_element_status cmd;

	/*
	 * Build SCSI command.
	 */
	bzero(&cmd, sizeof(cmd));
	cmd.opcode = READ_ELEMENT_STATUS;
	_lto2b(first, cmd.sea);
	_lto2b(count, cmd.count);
	_lto3b(datalen, cmd.len);

	/*
	 * Send command to changer.
	 */
	return (scsi_scsi_cmd(sc->sc_link, (struct scsi_generic*) &cmd, sizeof(cmd),
			(u_char*) data, datalen, CHRETRIES, 100000, NULL, 0));
}


/*
 * Ask the device about itself and fill in the parameters in our
 * softc.
 */
int
ch_get_params(sc, scsiflags)
	struct ch_softc *sc;
	int scsiflags;
{
	struct scsi_mode_sense cmd;
	struct scsi_mode_sense_data {
		struct scsi_mode_header header;
		union {
			struct page_element_address_assignment ea;
			struct page_transport_geometry_parameters tg;
			struct page_device_capabilities cap;
		} pages;
	} sense_data;
	int error, from;
	u_int8_t *moves, *exchanges;

	/*
	 * Grab info from the element address assignment page.
	 */
	bzero(&cmd, sizeof(cmd));
	bzero(&sense_data, sizeof(sense_data));
	cmd.opcode = MODE_SENSE;
	cmd.byte2 |= 0x08; /* disable block descriptors */
	cmd.page = 0x1d;
	cmd.length = (sizeof(sense_data) & 0xff);
	error = scsi_scsi_cmd(sc->sc_link, (struct scsi_generic*) &cmd, sizeof(cmd),
			(u_char*) &sense_data, sizeof(sense_data), CHRETRIES, 6000, NULL,
			scsiflags | SCSI_DATA_IN);
	if (error) {
		printf("%s: could not sense element address page\n",
				sc->sc_dev.dv_xname);
		return (error);
	}

	sc->sc_firsts[CHET_MT] = _2btol(sense_data.pages.ea.mtea);
	sc->sc_counts[CHET_MT] = _2btol(sense_data.pages.ea.nmte);
	sc->sc_firsts[CHET_ST] = _2btol(sense_data.pages.ea.fsea);
	sc->sc_counts[CHET_ST] = _2btol(sense_data.pages.ea.nse);
	sc->sc_firsts[CHET_IE] = _2btol(sense_data.pages.ea.fieea);
	sc->sc_counts[CHET_IE] = _2btol(sense_data.pages.ea.niee);
	sc->sc_firsts[CHET_DT] = _2btol(sense_data.pages.ea.fdtea);
	sc->sc_counts[CHET_DT] = _2btol(sense_data.pages.ea.ndte);

	/* XXX ask for page trasport geom */

	/*
	 * Grab info from the capabilities page.
	 */
	bzero(&cmd, sizeof(cmd));
	bzero(&sense_data, sizeof(sense_data));
	cmd.opcode = MODE_SENSE;
	cmd.byte2 |= 0x08; /* disable block descriptors */
	cmd.page = 0x1f;
	cmd.length = (sizeof(sense_data) & 0xff);
	error = scsi_scsi_cmd(sc->sc_link, (struct scsi_generic*) &cmd, sizeof(cmd),
			(u_char*) &sense_data, sizeof(sense_data), CHRETRIES, 6000, NULL,
			scsiflags | SCSI_DATA_IN);
	if (error) {
		printf("%s: could not sense capabilities page\n", sc->sc_dev.dv_xname);
		return (error);
	}

	bzero(sc->sc_movemask, sizeof(sc->sc_movemask));
	bzero(sc->sc_exchangemask, sizeof(sc->sc_exchangemask));
	moves = &sense_data.pages.cap.move_from_mt;
	exchanges = &sense_data.pages.cap.exchange_with_mt;
	for (from = CHET_MT; from <= CHET_DT; ++from) {
		sc->sc_movemask[from] = moves[from];
		sc->sc_exchangemask[from] = exchanges[from];
	}

	sc->sc_link->flags |= SDEV_MEDIA_LOADED;
	return (0);
}
