/*	$OpenBSD: apic.c,v 1.6 2009/08/13 13:24:48 kettenis Exp $	*/
/* $NetBSD: apic.c,v 1.1.2.2 2000/02/21 18:51:00 sommerfeld Exp $ */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by RedBack Networks Inc.
 *
 * Author: Bill Sommerfeld
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

#include <sys/cdefs.h>

#include <sys/types.h>
#include <sys/systm.h>

#include <machine/apic/apic.h>
#include <machine/apic/lapicreg.h>

const char redirlofmt[] = "\177\20"
	"f\0\10vector\0"
	"f\10\3delmode\0"
	"b\13logical\0"
	"b\14pending\0"
	"b\15actlo\0"
	"b\16irrpending\0"
	"b\17level\0"
	"b\20masked\0"
	"f\22\1dest\0" "=\1self" "=\2all" "=\3all-others";

const char redirhifmt[] = "\177\20"
	"f\30\10target\0";

void
apic_format_redir(where1, where2, idx, type, redirhi, redirlo)
	const char *where1;
	const char *where2;
	int idx, type;
	u_int32_t redirhi;
	u_int32_t redirlo;
{
	char buf[256];

	printf("%s: %s%d%d %s", where1, where2, idx, type, bitmask_snprintf(redirlo, redirlofmt, buf, sizeof(buf)));

	if ((redirlo & LAPIC_DEST_MASK) == 0) {
		printf(" %s", bitmask_snprintf(redirhi, redirhifmt, buf, sizeof(buf)));
	}

	printf("\n");
}
