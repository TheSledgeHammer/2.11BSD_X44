/*	$NetBSD: ipsec.c,v 1.10 2003/08/07 11:15:19 agc Exp $	*/
/*	$KAME: ipsec.c,v 1.33 2003/07/25 09:54:32 itojun Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, 1998, and 1999 WIDE Project.
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Copyright (c) 1983, 1988, 1993
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
#ifndef lint
#if 0
static char sccsid[] = "from: @(#)inet.c	8.4 (Berkeley) 4/20/94";
#else
#ifdef __NetBSD__
__RCSID("$NetBSD: ipsec.c,v 1.10 2003/08/07 11:15:19 agc Exp $");
#endif
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/socket.h>

#include <netinet/in.h>

#ifdef IPSEC
#include <netinet6/ipsec.h>
#include <netkey/keysock.h>
#endif

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "netstat.h"

#ifdef IPSEC
struct val2str {
	int val;
	const char *str;
};

static struct val2str ipsec_ahnames[] = {
	{ SADB_AALG_NONE, "none", },
	{ SADB_AALG_MD5HMAC, "hmac-md5", },
	{ SADB_AALG_SHA1HMAC, "hmac-sha1", },
	{ SADB_X_AALG_MD5, "md5", },
	{ SADB_X_AALG_SHA, "sha", },
	{ SADB_X_AALG_NULL, "null", },
	{ SADB_X_AALG_SHA2_256, "hmac-sha2-256", },
	{ SADB_X_AALG_SHA2_384, "hmac-sha2-384", },
	{ SADB_X_AALG_SHA2_512, "hmac-sha2-512", },
	{ SADB_X_AALG_RIPEMD160HMAC, "hmac-ripemd160", },
	{ SADB_X_AALG_AES_XCBC_MAC, "aes-xcbc-mac", },
	{ -1, NULL },
};

static struct val2str ipsec_espnames[] = {
	{ SADB_EALG_NONE, "none", },
	{ SADB_EALG_DESCBC, "des-cbc", },
	{ SADB_EALG_3DESCBC, "3des-cbc", },
	{ SADB_EALG_NULL, "null", },
	{ SADB_X_EALG_CAST128CBC, "cast128-cbc", },
	{ SADB_X_EALG_BLOWFISHCBC, "blowfish-cbc", },
	{ SADB_X_EALG_RIJNDAELCBC, "rijndael-cbc", },
	{ SADB_X_EALG_AESCTR, "aes-ctr", },
	{ -1, NULL },
};

static struct val2str ipsec_compnames[] = {
	{ SADB_X_CALG_NONE, "none", },
	{ SADB_X_CALG_OUI, "oui", },
	{ SADB_X_CALG_DEFLATE, "deflate", },
	{ SADB_X_CALG_LZS, "lzs", },
	{ -1, NULL },
};

static struct ipsecstat ipsecstat;

static void print_ipsecstats(void);
static void ipsec_hist(const u_quad_t *, size_t, const struct val2str *, size_t, const char *);

/*
 * Dump IPSEC statistics structure.
 */
static void
ipsec_hist(hist, histmax, name, namemax, title)
	const u_quad_t *hist;
	size_t histmax;
	const struct val2str *name;
	size_t namemax;
	const char *title;
{
	int first;
	size_t proto;
	const struct val2str *p;

	first = 1;
	for (proto = 0; proto < histmax; proto++) {
		if (hist[proto] <= 0)
			continue;
		if (first) {
			printf("\t%s histogram:\n", title);
			first = 0;
		}
		for (p = name; p && p->str; p++) {
			if (p->val == proto)
				break;
		}
		if (p && p->str) {
			printf("\t\t%s: %llu\n", p->str, (unsigned long long)hist[proto]);
		} else {
			printf("\t\t#%ld: %llu\n", (long)proto,
			    (unsigned long long)hist[proto]);
		}
	}
}

static void
print_ipsecstats(void)
{
#define	p(f, m) if (ipsecstat.f || sflag <= 1) \
    printf(m, (unsigned long long)ipsecstat.f, plural(ipsecstat.f))
#define	pes(f, m) if (ipsecstat.f || sflag <= 1) \
    printf(m, (unsigned long long)ipsecstat.f, plurales(ipsecstat.f))
#define hist(f, n, t) \
    ipsec_hist((f), sizeof(f)/sizeof(f[0]), (n), sizeof(n)/sizeof(n[0]), (t));

	p(in_success, "\t%llu inbound packet%s processed successfully\n");
	p(in_polvio, "\t%llu inbound packet%s violated process security "
	    "policy\n");
	p(in_nosa, "\t%llu inbound packet%s with no SA available\n");
	p(in_inval, "\t%llu invalid inbound packet%s\n");
	p(in_nomem, "\t%llu inbound packet%s failed due to insufficient memory\n");
	p(in_badspi, "\t%llu inbound packet%s failed getting SPI\n");
	p(in_ahreplay, "\t%llu inbound packet%s failed on AH replay check\n");
	p(in_espreplay, "\t%llu inbound packet%s failed on ESP replay check\n");
	p(in_ahauthsucc, "\t%llu inbound packet%s considered authentic\n");
	p(in_ahauthfail, "\t%llu inbound packet%s failed on authentication\n");
	hist(ipsecstat.in_ahhist, ipsec_ahnames, "AH input");
	hist(ipsecstat.in_esphist, ipsec_espnames, "ESP input");
	hist(ipsecstat.in_comphist, ipsec_compnames, "IPComp input");

	p(out_success, "\t%llu outbound packet%s processed successfully\n");
	p(out_polvio, "\t%llu outbound packet%s violated process security "
	    "policy\n");
	p(out_nosa, "\t%llu outbound packet%s with no SA available\n");
	p(out_inval, "\t%llu invalid outbound packet%s\n");
	p(out_nomem, "\t%llu outbound packet%s failed due to insufficient memory\n");
	p(out_noroute, "\t%llu outbound packet%s with no route\n");
	hist(ipsecstat.out_ahhist, ipsec_ahnames, "AH output");
	hist(ipsecstat.out_esphist, ipsec_espnames, "ESP output");
	hist(ipsecstat.out_comphist, ipsec_compnames, "IPComp output");
	p(spdcachelookup, "\t%llu SPD cache lookup%s\n");
	pes(spdcachemiss, "\t%llu SPD cache miss%s\n");
#undef p
#undef pes
#undef hist
}

void
ipsec_stats(off, name)
	u_long off;
	char *name;
{
	if (off == 0)
		return;
	printf ("%s:\n", name);
	kread(off, (char *)&ipsecstat, sizeof (ipsecstat));

	print_ipsecstats();
}
