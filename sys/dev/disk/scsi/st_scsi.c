/*	$NetBSD: st_scsi.c,v 1.10.8.1 2004/09/11 12:59:02 he Exp $ */

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Originally written by Julian Elischer (julian@tfs.com)
 * for TRW Financial Systems for use under the MACH(2.5) operating system.
 *
 * TRW Financial Systems, in accordance with their agreement with Carnegie
 * Mellon University, makes this software available to CMU to distribute
 * or use in any manner that they see fit as long as this message is kept with
 * the software. For this reason TFS also grants any other persons or
 * organisations permission to use or modify this software.
 *
 * TFS supplies this software to be publicly redistributed
 * on the understanding that TFS is not responsible for the correct
 * functioning of this software in any circumstances.
 *
 * Ported to run under 386BSD by Julian Elischer (julian@tfs.com) Sept 1992
 * major changes by Julian Elischer (julian@jules.dialix.oz.au) May 1993
 *
 * A lot of rewhacking done by mjacob (mjacob@nas.nasa.gov).
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: st_scsi.c,v 1.10.8.1 2004/09/11 12:59:02 he Exp $");

#include "opt_scsi.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <dev/disk/scsi/stvar.h>
#include <dev/disk/scsi/scsi_tape.h>
#include <dev/disk/scsi/scsi_all.h>

int		st_scsibus_match(struct device *, struct cfdata *, void *);
void	st_scsibus_attach(struct device *, struct device *, void *);
int		st_scsibus_ops(struct st_softc *, int, int);
int		st_scsibus_read_block_limits(struct st_softc *, int);
int		st_scsibus_mode_sense(struct st_softc *, int);
int		st_scsibus_mode_select(struct st_softc *, int);
int		st_scsibus_cmprss(struct st_softc *, int, int);

CFOPS_DECL(st_scsibus, st_scsibus_match, st_scsibus_attach, stdetach, stactivate);
CFATTACH_DECL(st_scsibus, &st_cd, &st_scsibus_cops, sizeof(struct st_softc));

const struct scsipi_inquiry_pattern st_scsibus_patterns[] = {
	{T_SEQUENTIAL, T_REMOV,
	 "",         "",                 ""},
};

int
st_scsibus_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct scsipibus_attach_args *sa = aux;
	int priority;

	if (scsipi_periph_bustype(sa->sa_periph) != SCSIPI_BUSTYPE_SCSI)
		return (0);

	(void)scsipi_inqmatch(&sa->sa_inqbuf,
	    (caddr_t)st_scsibus_patterns,
	    sizeof(st_scsibus_patterns)/sizeof(st_scsibus_patterns[0]),
	    sizeof(st_scsibus_patterns[0]), &priority);
	return (priority);
}

void
st_scsibus_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct st_softc *st = (void *)self;

	st->ops = st_scsibus_ops;
	stattach(parent, st, aux);
}

int
st_scsibus_ops(st, op, flags)
	struct st_softc *st;
	int op;
	int flags;
{
	switch(op) {
	case ST_OPS_RBL:
		return st_scsibus_read_block_limits(st, flags);
	case ST_OPS_MODESENSE:
		return st_scsibus_mode_sense(st, flags);
	case ST_OPS_MODESELECT:
		return st_scsibus_mode_select(st, flags);
	case ST_OPS_CMPRSS_ON:
	case ST_OPS_CMPRSS_OFF:
		return st_scsibus_cmprss(st, flags,
		    (op == ST_OPS_CMPRSS_ON) ? 1 : 0);
	default:
		panic("st_scsibus_ops: invalid op");
		return 0; /* XXX to appease gcc */
	}
	/* NOTREACHED */
}

/*
 * Ask the drive what it's min and max blk sizes are.
 */
int
st_scsibus_read_block_limits(st, flags)
	struct st_softc *st;
	int flags;
{
	struct scsi_block_limits cmd;
	struct scsi_block_limits_data block_limits;
	struct scsipi_periph *periph = st->sc_periph;
	int error;

	/*
	 * do a 'Read Block Limits'
	 */
	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode = READ_BLOCK_LIMITS;

	/*
	 * do the command, update the global values
	 */
	error = scsipi_command(periph, NULL, (struct scsipi_generic *)&cmd,
	    sizeof(cmd), (u_char *)&block_limits, sizeof(block_limits),
	    ST_RETRIES, ST_CTL_TIME, NULL,
	    flags | XS_CTL_DATA_IN | XS_CTL_DATA_ONSTACK);
	if (error)
		return (error);

	st->blkmin = _2btol(block_limits.min_length);
	st->blkmax = _3btol(block_limits.max_length);

	SC_DEBUG(periph, SCSIPI_DB3,
	    ("(%d <= blksize <= %d)\n", st->blkmin, st->blkmax));
	return (0);
}

/*
 * Get the scsi driver to send a full inquiry to the
 * device and use the results to fill out the global
 * parameter structure.
 *
 * called from:
 * attach
 * open
 * ioctl (to reset original blksize)
 */
int
st_scsibus_mode_sense(st, flags)
	struct st_softc *st;
	int flags;
{
	u_int scsipi_sense_len;
	int error;
	struct scsipi_sense {
		struct scsipi_mode_header header;
		struct scsi_blk_desc blk_desc;
		u_char sense_data[MAX_PAGE_0_SIZE];
	} scsipi_sense;
	struct scsipi_periph *periph = st->sc_periph;

	scsipi_sense_len = 12 + st->page_0_size;

	/*
	 * Set up a mode sense
	 * We don't need the results. Just print them for our interest's sake,
	 * if asked, or if we need it as a template for the mode select store
	 * it away.
	 */
	error = scsipi_mode_sense(st->sc_periph, 0, SMS_PAGE_CTRL_CURRENT,
	    &scsipi_sense.header, scsipi_sense_len, flags | XS_CTL_DATA_ONSTACK,
	    ST_RETRIES, ST_CTL_TIME);
	if (error)
		return (error);

	st->numblks = _3btol(scsipi_sense.blk_desc.nblocks);
	st->media_blksize = _3btol(scsipi_sense.blk_desc.blklen);
	st->media_density = scsipi_sense.blk_desc.density;
	if (scsipi_sense.header.dev_spec & SMH_DSP_WRITE_PROT)
		st->flags |= ST_READONLY;
	else
		st->flags &= ~ST_READONLY;
	SC_DEBUG(periph, SCSIPI_DB3,
	    ("density code %d, %d-byte blocks, write-%s, ",
	    st->media_density, st->media_blksize,
	    st->flags & ST_READONLY ? "protected" : "enabled"));
	SC_DEBUG(periph, SCSIPI_DB3,
	    ("%sbuffered\n",
	    scsipi_sense.header.dev_spec & SMH_DSP_BUFF_MODE ? "" : "un"));
	if (st->page_0_size)
		memcpy(st->sense_data, scsipi_sense.sense_data,
		    st->page_0_size);
	periph->periph_flags |= PERIPH_MEDIA_LOADED;
	return (0);
}

/*
 * Send a filled out parameter structure to the drive to
 * set it into the desire modes etc.
 */
int
st_scsibus_mode_select(st, flags)
	struct st_softc *st;
	int flags;
{
	u_int scsi_select_len;
	struct scsi_select {
		struct scsipi_mode_header header;
		struct scsi_blk_desc blk_desc;
		u_char sense_data[MAX_PAGE_0_SIZE];
	} scsi_select;
	struct scsipi_periph *periph = st->sc_periph;

	scsi_select_len = 12 + st->page_0_size;

	/*
	 * This quirk deals with drives that have only one valid mode
	 * and think this gives them license to reject all mode selects,
	 * even if the selected mode is the one that is supported.
	 */
	if (st->quirks & ST_Q_UNIMODAL) {
		SC_DEBUG(periph, SCSIPI_DB3,
		    ("not setting density 0x%x blksize 0x%x\n",
		    st->density, st->blksize));
		return (0);
	}

	/*
	 * Set up for a mode select
	 */
	memset(&scsi_select, 0, scsi_select_len);
	scsi_select.header.blk_desc_len = sizeof(struct scsi_blk_desc);
	scsi_select.header.dev_spec &= ~SMH_DSP_BUFF_MODE;
	scsi_select.blk_desc.density = st->density;
	if (st->flags & ST_DONTBUFFER)
		scsi_select.header.dev_spec |= SMH_DSP_BUFF_MODE_OFF;
	else
		scsi_select.header.dev_spec |= SMH_DSP_BUFF_MODE_ON;
	if (st->flags & ST_FIXEDBLOCKS)
		_lto3b(st->blksize, scsi_select.blk_desc.blklen);
	if (st->page_0_size)
		memcpy(scsi_select.sense_data, st->sense_data, st->page_0_size);

	/*
	 * do the command
	 */
	return scsipi_mode_select(periph, 0, &scsi_select.header, 
	    scsi_select_len, flags | XS_CTL_DATA_ONSTACK,
	    ST_RETRIES, ST_CTL_TIME);
}

int
st_scsibus_cmprss(st, flags, onoff)
	struct st_softc *st;
	int flags;
	int onoff;
{
	u_int scsi_dlen;
	int byte2, page;
	struct scsi_select {
		struct scsipi_mode_header header;
		struct scsi_blk_desc blk_desc;
		u_char pdata[MAX(sizeof(struct scsi_tape_dev_conf_page),
		    sizeof(struct scsi_tape_dev_compression_page))];
	} scsi_pdata;
	struct scsi_tape_dev_conf_page *ptr;
	struct scsi_tape_dev_compression_page *cptr;
	struct scsipi_periph *periph = st->sc_periph;
	int error, ison;

	scsi_dlen = sizeof(scsi_pdata);
	/*  
	 * Do DATA COMPRESSION page first.
	 */
	page = SMS_PAGE_CTRL_CURRENT | 0xf;
	byte2 = 0;

	/*
	 * Do the MODE SENSE command...
	 */
again:
	memset(&scsi_pdata, 0, scsi_dlen);
	error = scsipi_mode_sense(periph, byte2, page,
	    &scsi_pdata.header, scsi_dlen, flags | XS_CTL_DATA_ONSTACK,
	    ST_RETRIES, ST_CTL_TIME);

	if (error) {
		if (byte2 != SMS_DBD) {
			byte2 = SMS_DBD;
			goto again;
		}
		/*
		 * Try a different page?
		 */
		if (page == (SMS_PAGE_CTRL_CURRENT | 0xf)) {
			page = SMS_PAGE_CTRL_CURRENT | 0x10;
			byte2 = 0;
			goto again;
		}
		return (error);
	}

	if (scsi_pdata.header.blk_desc_len)
		ptr = (struct scsi_tape_dev_conf_page *) scsi_pdata.pdata;
	else
		ptr = (struct scsi_tape_dev_conf_page *) &scsi_pdata.blk_desc;

	if ((page & SMS_PAGE_CODE) == 0xf) {
		cptr = (struct scsi_tape_dev_compression_page *) ptr;
		ison = (cptr->dce_dcc & DCP_DCE) != 0;
		if (onoff)
			cptr->dce_dcc |= DCP_DCE;
		else
			cptr->dce_dcc &= ~DCP_DCE;
		cptr->pagecode &= ~0x80;
	} else {
		ison =  (ptr->sel_comp_alg != 0);
		if (onoff)
			ptr->sel_comp_alg = 1;
		else
			ptr->sel_comp_alg = 0;
		ptr->pagecode &= ~0x80;
		ptr->byte2 = 0;
		ptr->active_partition = 0;
		ptr->wb_full_ratio = 0;
		ptr->rb_empty_ratio = 0;
		ptr->byte8 &= ~0x30;
		ptr->gap_size = 0;
		ptr->byte10 &= ~0xe7;
		ptr->ew_bufsize[0] = 0;
		ptr->ew_bufsize[1] = 0;
		ptr->ew_bufsize[2] = 0;
		ptr->reserved = 0;
	}
	/*
	 * There might be a virtue in actually doing the MODE SELECTS,
	 * but let's not clog the bus over it.
	 */
	if (onoff == ison)
		return (0);

	/*
	 * Set up for a mode select
	 */

	scsi_pdata.header.data_length = 0;
	scsi_pdata.header.medium_type = 0;
	if ((st->flags & ST_DONTBUFFER) == 0)
		scsi_pdata.header.dev_spec = SMH_DSP_BUFF_MODE_ON;
	else
		scsi_pdata.header.dev_spec = 0;

	if (scsi_pdata.header.blk_desc_len) {
		scsi_pdata.blk_desc.density = 0;
		scsi_pdata.blk_desc.nblocks[0] = 0;
		scsi_pdata.blk_desc.nblocks[1] = 0;
		scsi_pdata.blk_desc.nblocks[2] = 0;
	}

	/*
	 * Do the command
	 */
	error = scsipi_mode_select(periph, SMS_PF, &scsi_pdata.header,
	    scsi_dlen, flags | XS_CTL_DATA_ONSTACK, ST_RETRIES, ST_CTL_TIME);

	if (error && (page & SMS_PAGE_CODE) == 0xf) {
		/*
		 * Try DEVICE CONFIGURATION page.
		 */
		page = SMS_PAGE_CTRL_CURRENT | 0x10;
		goto again;
	}
	return (error);
}
