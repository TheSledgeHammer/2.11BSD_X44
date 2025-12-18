/*	$NetBSD: ifconfig.c,v 1.141.4.2 2005/07/24 01:58:38 snj Exp $	*/

/*-
 * Copyright (c) 1997, 1998, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
 * Copyright (c) 1983, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <util.h>

#include "ifconfig.h"

#define	A_MEDIA			0x0001		/* media command */
#define	A_MEDIAOPTSET	0x0002		/* mediaopt command */
#define	A_MEDIAOPTCLR	0x0004		/* -mediaopt command */
#define	A_MEDIAOPT		(A_MEDIAOPTSET|A_MEDIAOPTCLR)
#define	A_MEDIAINST		0x0008		/* instance or inst command */
#define	A_MEDIAMODE		0x0010		/* mode command */

void	setmedia(const char *, int);
void	setmediamode(const char *, int);
void	setmediaopt(const char *, int);
void	unsetmediaopt(const char *, int);
void	setmediainst(const char *, int);

/*
 * Media stuff.  Whenever a media command is first performed, the
 * currently select media is grabbed for this interface.  If `media'
 * is given, the current media word is modifed.  `mediaopt' commands
 * only modify the set and clear words.  They then operate on the
 * current media word later.
 */
int	media_current;
int	mediaopt_set;
int	mediaopt_clear;

struct cmd media_cmds[] = {
		{ "media",	NEXTARG,	A_MEDIA,	setmedia },
		{ "mediaopt",	NEXTARG,	A_MEDIAOPTSET,	setmediaopt },
		{ "-mediaopt",	NEXTARG,	A_MEDIAOPTCLR,	unsetmediaopt },
		{ "mode",		NEXTARG,	A_MEDIAMODE,	setmediamode },
		{ "instance",	NEXTARG,	A_MEDIAINST,	setmediainst },
		{ "inst",		NEXTARG,	A_MEDIAINST,	setmediainst },
};

void	print_media_word(int, const char *);
void	init_current_media(void);

void
media_init(void)
{
	size_t i;

	for (i = 0; i < nitems(media_cmds);  i++) {
		cmd_register(&media_cmds[i]);
	}
}

static void
media_error(int type, const char *val, const char *opt)
{
	errx(EXIT_FAILURE, "unknown %s media %s: %s", get_media_type_string(type),
			opt, val);
}

void
init_current_media(void)
{
	struct ifmediareq ifmr;

	/*
	 * If we have not yet done so, grab the currently-selected
	 * media.
	 */
	if ((actions & (A_MEDIA|A_MEDIAOPT|A_MEDIAMODE)) == 0) {
		(void) memset(&ifmr, 0, sizeof(ifmr));
		(void) strncpy(ifmr.ifm_name, name, sizeof(ifmr.ifm_name));

		if (ioctl(s, SIOCGIFMEDIA, &ifmr) == -1) {
			/*
			 * If we get E2BIG, the kernel is telling us
			 * that there are more, so we can ignore it.
			 */
			if (errno != E2BIG)
				err(EXIT_FAILURE, "SGIOCGIFMEDIA");
		}

		media_current = ifmr.ifm_current;
	}

	/* Sanity. */
	if (IFM_TYPE(media_current) == 0) {
		errx(EXIT_FAILURE, "%s: no link type?", name);
	}
}

void
process_media_commands(void)
{

	if ((actions & (A_MEDIA|A_MEDIAOPT|A_MEDIAMODE)) == 0) {
		/* Nothing to do. */
		return;
	}

	/*
	 * Media already set up, and commands sanity-checked.  Set/clear
	 * any options, and we're ready to go.
	 */
	media_current |= mediaopt_set;
	media_current &= ~mediaopt_clear;

	strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	ifr.ifr_media = media_current;

	if (ioctl(s, SIOCSIFMEDIA, &ifr) == -1) {
		err(EXIT_FAILURE, "SIOCSIFMEDIA");
	}
}

void
setmedia(const char *val, int d)
{
	int type, subtype, inst;

	init_current_media();

	/* Only one media command may be given. */
	if (actions & A_MEDIA)
		errx(EXIT_FAILURE, "only one `media' command may be issued");

	/* Must not come after mode commands */
	if (actions & A_MEDIAMODE)
		errx(EXIT_FAILURE,
		    "may not issue `media' after `mode' commands");

	/* Must not come after mediaopt commands */
	if (actions & A_MEDIAOPT)
		errx(EXIT_FAILURE,
		    "may not issue `media' after `mediaopt' commands");

	/*
	 * No need to check if `instance' has been issued; setmediainst()
	 * craps out if `media' has not been specified.
	 */

	type = IFM_TYPE(media_current);
	inst = IFM_INST(media_current);

	/* Look up the subtype. */
	subtype = get_media_subtype(type, val);
	if (subtype == -1)
		media_error(type, val, "subtype");

	/* Build the new current media word. */
	media_current = IFM_MAKEWORD(type, subtype, 0, inst);

	/* Media will be set after other processing is complete. */
}

void
setmediaopt(const char *val, int d)
{
	char *invalid;

	init_current_media();

	/* Can only issue `mediaopt' once. */
	if (actions & A_MEDIAOPTSET)
		errx(EXIT_FAILURE, "only one `mediaopt' command may be issued");

	/* Can't issue `mediaopt' if `instance' has already been issued. */
	if (actions & A_MEDIAINST)
		errx(EXIT_FAILURE, "may not issue `mediaopt' after `instance'");

	mediaopt_set = get_media_options(media_current, val, &invalid);
	if (mediaopt_set == -1)
		media_error(media_current, invalid, "option");

	/* Media will be set after other processing is complete. */
}

void
unsetmediaopt(const char *val, int d)
{
	char *invalid;

	init_current_media();

	/* Can only issue `-mediaopt' once. */
	if (actions & A_MEDIAOPTCLR)
		errx(EXIT_FAILURE,
		    "only one `-mediaopt' command may be issued");

	/* May not issue `media' and `-mediaopt'. */
	if (actions & A_MEDIA)
		errx(EXIT_FAILURE,
		    "may not issue both `media' and `-mediaopt'");

	/*
	 * No need to check for A_MEDIAINST, since the test for A_MEDIA
	 * implicitly checks for A_MEDIAINST.
	 */

	mediaopt_clear = get_media_options(media_current, val, &invalid);
	if (mediaopt_clear == -1)
		media_error(media_current, invalid, "option");

	/* Media will be set after other processing is complete. */
}

void
setmediainst(const char *val, int d)
{
	int type, subtype, options, inst;

	init_current_media();

	/* Can only issue `instance' once. */
	if (actions & A_MEDIAINST)
		errx(EXIT_FAILURE, "only one `instance' command may be issued");

	/* Must have already specified `media' */
	if ((actions & A_MEDIA) == 0)
		errx(EXIT_FAILURE, "must specify `media' before `instance'");

	type = IFM_TYPE(media_current);
	subtype = IFM_SUBTYPE(media_current);
	options = IFM_OPTIONS(media_current);

	inst = atoi(val);
	if (inst < 0 || inst > (int)IFM_INST_MAX)
		errx(EXIT_FAILURE, "invalid media instance: %s", val);

	media_current = IFM_MAKEWORD(type, subtype, options, inst);

	/* Media will be set after other processing is complete. */
}

void
setmediamode(const char *val, int d)
{
	int type, subtype, options, inst, mode;

	init_current_media();

	/* Can only issue `mode' once. */
	if (actions & A_MEDIAMODE)
		errx(EXIT_FAILURE, "only one `mode' command may be issued");

	type = IFM_TYPE(media_current);
	subtype = IFM_SUBTYPE(media_current);
	options = IFM_OPTIONS(media_current);
	inst = IFM_INST(media_current);

	mode = get_media_mode(type, val);
	if (mode == -1)
		media_error(type, val, "mode");

	media_current = IFM_MAKEWORD(type, subtype, options, inst) | mode;

	/* Media will be set after other processing is complete. */
}

void
print_media_word(int ifmw, const char *opt_sep)
{
	const char *str;

	printf("%s", get_media_subtype_string(ifmw));

	/* Find mode. */
	if (IFM_MODE(ifmw) != 0) {
		str = get_media_mode_string(ifmw);
		if (str != NULL)
			printf(" mode %s", str);
	}

	/* Find options. */
	for (; (str = get_media_option_string(&ifmw)) != NULL; opt_sep = ",")
		printf("%s%s", opt_sep, str);

	if (IFM_INST(ifmw) != 0)
		printf(" instance %d", IFM_INST(ifmw));
}

int
carrier(void)
{
	struct ifmediareq ifmr;

	(void) memset(&ifmr, 0, sizeof(ifmr));
	(void) strncpy(ifmr.ifm_name, name, sizeof(ifmr.ifm_name));

	if (ioctl(s, SIOCGIFMEDIA, &ifmr) == -1) {
		/*
		 * Interface doesn't support SIOC{G,S}IFMEDIA;
		 * assume ok.
		 */
		return 0;
	}
	if ((ifmr.ifm_status & IFM_AVALID) == 0) {
		/*
		 * Interface doesn't report media-valid status.
		 * assume ok.
		 */
		return 0;
	}
	/* otherwise, return ok for active, not-ok if not active. */
	return !(ifmr.ifm_status & IFM_ACTIVE);
}
