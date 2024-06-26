/*	$NetBSD: scsiconf.h,v 1.50.4.1 2004/09/11 12:52:42 he Exp $	*/

/*-
 * Copyright (c) 1998, 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum; by Jason R. Thorpe of the Numerical Aerospace
 * Simulation Facility, NASA Ames Research Center.
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
 */

#ifndef _DEV_SCSIPI_SCSICONF_H_
#define _DEV_SCSIPI_SCSICONF_H_

#include <dev/disk/scsi/scsipiconf.h>

#define SCSICF_CHANNEL				0
#define SCSICF_CHANNEL_DEFAULT		-1
#define SCSIBUSCF_TARGET 			0
#define SCSIBUSCF_TARGET_DEFAULT 	-1
#define SCSIBUSCF_LUN 				1
#define SCSIBUSCF_LUN_DEFAULT 		-1

int	scsiprint (void *, const char *);

struct scsibus_softc {
	struct device sc_dev;
	struct scsipi_channel *sc_channel;	/* our scsipi_channel */
	int	sc_flags;
};

/* sc_flags */
#define	SCSIBUSF_OPEN	0x00000001	/* bus is open */

extern const struct scsipi_bustype scsi_bustype;

int		scsi_change_def (struct scsipi_periph *, int);
void	scsi_kill_pending (struct scsipi_periph *);
void	scsi_print_addr (struct scsipi_periph *);
int		scsi_probe_bus (struct scsibus_softc *, int, int);
int		scsi_scsipi_cmd (struct scsipi_periph *, struct scsipi_xfer *,
	    struct scsipi_generic *, int, void *, size_t,
	    int, int, struct buf *, int);
#endif /* _DEV_SCSIPI_SCSICONF_H_ */
