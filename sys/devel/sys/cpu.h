/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2005-2007 Nate Lawson (SDG)
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

#ifndef _SYS_CPU_H_
#define _SYS_CPU_H_

#include <sys/queue.h>

/*
 * CPU frequency control interface.
 */

/* Each driver's CPU frequency setting is exported in this format. */
struct cf_setting {
	int						freq;	/* CPU clock in Mhz or 100ths of a percent. */
	int						volts;	/* Voltage in mV. */
	int						power;	/* Power consumed in mW. */
	int						lat;	/* Transition latency in us. */
	struct device 			*dev;	/* Driver providing this setting. */
	int						spec[4];/* Driver-specific storage for non-standard info. */
};

/* Maximum number of settings a given driver can have. */
#define MAX_SETTINGS		256

/* A combination of settings is a level. */
struct cf_level {
	struct cf_setting		total_set;
	struct cf_setting		abs_set;
	struct cf_setting		rel_set[MAX_SETTINGS];
	int						rel_count;
	TAILQ_ENTRY(cf_level)	link;
};

TAILQ_HEAD(cf_level_lst, cf_level);

#endif /* _SYS_CPU_H_ */
