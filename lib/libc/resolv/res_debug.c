/*
 * Portions Copyright (C) 2004, 2005, 2008, 2009  Internet Systems Consortium, Inc. ("ISC")
 * Portions Copyright (C) 1996-2003  Internet Software Consortium.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Copyright (c) 1985
 *    The Regents of the University of California.  All rights reserved.
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

/*
 * Portions Copyright (c) 1993 by Digital Equipment Corporation.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies, and that
 * the name of Digital Equipment Corporation not be used in advertising or
 * publicity pertaining to distribution of the document or software without
 * specific, written prior permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND DIGITAL EQUIPMENT CORP. DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS.   IN NO EVENT SHALL DIGITAL EQUIPMENT
 * CORPORATION BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

/*
 * Portions Copyright (c) 1995 by International Business Machines, Inc.
 *
 * International Business Machines, Inc. (hereinafter called IBM) grants
 * permission under its copyrights to use, copy, modify, and distribute this
 * Software with or without fee, provided that the above copyright notice and
 * all paragraphs of this notice appear in all copies, and that the name of IBM
 * not be used in connection with the marketing of any product incorporating
 * the Software or modifications thereof, without specific, written prior
 * permission.
 *
 * To the extent it has a right to do so, IBM grants an immunity from suit
 * under its patents, if any, for the use, sale or manufacture of products to
 * the extent that such products are used for performing Domain Name System
 * dynamic updates in TCP/IP networks by means of the Software.  No immunity is
 * granted for any product per se or for any other function of any product.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", AND IBM DISCLAIMS ALL WARRANTIES,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE.  IN NO EVENT SHALL IBM BE LIABLE FOR ANY SPECIAL,
 * DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE, EVEN
 * IF IBM IS APPRISED OF THE POSSIBILITY OF SUCH DAMAGES.
 */
/*
 * Copyright (c) 1985 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)res_debug.c	5.22 (Berkeley) 3/7/88";
#endif
#endif /* LIBC_SCCS and not lint */

#if defined(lint) && !defined(DEBUG)
#define DEBUG
#endif

#include "namespace.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <netdb.h>
#include <resolv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "res_private.h"

#if 0
#ifdef __weak_alias
__weak_alias(p_query,__p_query)
__weak_alias(p_cdname,__p_cdname)
__weak_alias(p_rr,__p_rr)
__weak_alias(p_fqname,__p_fqname)
__weak_alias(p_class,__p_class)
__weak_alias(p_type,__p_type)
__weak_alias(p_name,__p_name)
__weak_alias(p_option,__p_option)
__weak_alias(fp_query,__fp_query)
__weak_alias(fp_resstat,__fp_resstat)
#endif
#endif

const char *_res_opcodes[] = {
		"QUERY",
		"IQUERY",
		"CQUERYM",
		"CQUERYU",
		"4",
		"5",
		"6",
		"7",
		"8",
		"UPDATEA",
		"UPDATED",
		"UPDATEDA",
		"UPDATEM",
		"UPDATEMA",
		"ZONEINIT",
		"ZONEREF",
};

const char *_res_resultcodes[] = {
		"NOERROR",
		"FORMERR",
		"SERVFAIL",
		"NXDOMAIN",
		"NOTIMP",
		"REFUSED",
		"6",
		"7",
		"8",
		"9",
		"10",
		"11",
		"12",
		"13",
		"14",
		"NOCHANGE",
};

const char *_res_classcodes[] = {
		"IN",
		"CH",
		"CHAOS",
		"HS",
		"HESIOD",
		"ANY",
		"NONE",
};

const char *_res_typecodes[] = {
		"A",		/* address */
		"NS",		/* name server */
#ifdef OLDRR
		"MD",		/* mail destination */
		"MF",		/* mail forwarder */
#endif
		"CNAME",	/* connonical name */
		"SOA",		/* start of authority zone */
		"MB",		/* mailbox domain name */
		"MG",		/* mail group member */
		"MX",		/* mail routing info (exchanger) */
		"MR",		/* mail rename name */
		"TXT",		/* text */
		"RP",		/* responsible person */
		"AFSDB",	/* DCE or AFS server */
		"X25",		/* X25 address */
		"ISDN",		/* ISDN address */
		"RT",		/* router */
		"NSAP",		/* nsap address */
		"NSAP_PTR",	/* domain name pointer */
		"SIG",		/* signature */
		"KEY",		/* key */
		"PX",		/* mapping information */
		"GPOS",		/* geographical position (withdrawn) */
		"AAAA",		/* IPv6 address */
		"LOC",		/* location */
		"NXT",		/* next valid name (unimplemented) */
		"EID",		/* endpoint identifier (unimplemented) */
		"NIMLOC",	/* NIMROD locator (unimplemented) */
		"SRV",		/* server selection */
		"ATMA",		/* ATM address (unimplemented) */
		"NAPTR",	/* naptr */
		"KX",		/* key exchange */
		"CERT",		/* certificate */
		"A",		/* IPv6 address (experimental) */
		"DNAME",	/* non-terminal redirection */
		"OPT",		/* opt */
		"apl",		/* apl */
		"DS",		/* delegation signer */
		"SSFP",		/* SSH fingerprint */
		"IPSECKEY",	/* IPSEC key */
		"RRSIG",	/* rrsig */
		"NSEC",		/* nsec */
		"DNSKEY",	/* DNS key */
		"DHCID",    	/* dynamic host configuration identifier */
		"NSEC3",	/* nsec3 */
		"NSEC3PARAM", 	/* NSEC3 parameters */
		"HIP",		/* host identity protocol */
		"SPF",		/* sender policy framework */
		"TKEY",		/* tkey */
		"TSIG",		/* transaction signature */
		"NULL",		/* null resource record */
		"WKS",		/* well known service */
		"PTR", 		/* domain name pointer */
		"HINFO",	/* host information */
		"MINFO",	/* mailbox information */
		"IXFR",		/* incremental zone transfer */
		"AXFR",		/* zone transfer */
		"ZXFR", 	/* compressed zone transfer */
		"MAILB",	/* mail box */
		"MAILA",	/* mail address */
		"NAPTR",	/* URN Naming Authority */
		"KX",		/* Key Exchange */
		"CERT",		/* Certificate */
		"A6",		/* IPv6 Address */
		"DNAME",	/* dname */
		"SINK",		/* Kitchen Sink (experimental) */
		"OPT",		/* EDNS Options */
		"ANY",		/* matches any type */
		"DLV",		/* DNSSEC look-aside validation */
		"UINFO",
		"UID",
		"GID",
#ifdef ALLOW_T_UNSPEC
		"UNSPEC",
#endif
};

const char *_res_optioncodes[] = {
		"init",
		"debug",
		"aaonly",
		"usevc",
		"primry",
		"igntc",
		"recurs",
		"defnam",
		"styopn",
		"dnsrch",
#ifdef RES_USE_EDNS0	/* KAME extension */
		"edns0",
		"nsid",
#endif
#ifdef RES_USE_DNAME
		"dname",
#endif
#ifdef RES_USE_DNSSEC
		"dnssec",
#endif
#ifdef RES_NOTLDQUERY
		"no-tld-query",
#endif
#ifdef RES_NO_NIBBLE2
		"no-nibble2",
#endif
};

static char symbuf[20];
static const char *p_name_lookup(const char **, int, int, int *);

void
p_query(msg)
	const u_char *msg;
{
	fp_query(msg, stdout);
}

/*
 * Print the current options.
 * This is intended to be primarily a debugging routine.
 */
void
fp_resstat(statp, file)
	res_state statp;
	FILE *file;
{
	int bit;

	fprintf(file, ";; res options:");
	if (!statp)
		statp = &_res;
	for (bit = 0; bit < 32; bit++) { /* XXX 32 - bad assumption! */
		if (statp->options & (1 << bit))
			fprintf(file, " %s", p_option(1 << bit));
	}
	putc('\n', file);
}

/*
 * Print the contents of a query.
 * This is intended to be primarily a debugging routine.
 */
void
fp_query(msg, file)
	const u_char *msg;
	FILE *file;
{
	register const u_char *cp;
	register const HEADER *hp;
	register int n;

	/*
	 * Print header fields.
	 */
	hp = (const HEADER*) msg;
	cp = msg + sizeof(HEADER);
	fprintf(file, "HEADER:\n");
	fprintf(file, "\topcode = %s", _res_opcodes[hp->opcode]);
	fprintf(file, ", id = %d", ntohs(hp->id));
	fprintf(file, ", rcode = %s\n", _res_resultcodes[hp->rcode]);
	fprintf(file, "\theader flags: ");
	if (hp->qr)
		fprintf(file, " qr");
	if (hp->aa)
		fprintf(file, " aa");
	if (hp->tc)
		fprintf(file, " tc");
	if (hp->rd)
		fprintf(file, " rd");
	if (hp->ra)
		fprintf(file, " ra");
	if (hp->pr)
		fprintf(file, " pr");
	fprintf(file, "\n\tqdcount = %d", ntohs(hp->qdcount));
	fprintf(file, ", ancount = %d", ntohs(hp->ancount));
	fprintf(file, ", nscount = %d", ntohs(hp->nscount));
	fprintf(file, ", arcount = %d\n\n", ntohs(hp->arcount));
	/*
	 * Print question records.
	 */
	if ((n = ntohs(hp->qdcount))) {
		fprintf(file, "QUESTIONS:\n");
		while (--n >= 0) {
			fprintf(file, "\t");
			cp = p_cdname(cp, msg, file);
			if (cp == NULL)
				return;
			fprintf(file, ", type = %s", p_type(getshort(cp)));
			cp += sizeof(u_short);
			fprintf(file, ", class = %s\n\n", p_class(getshort(cp)));
			cp += sizeof(u_short);
		}
	}
	/*
	 * Print authoritative answer records
	 */
	if ((n = ntohs(hp->ancount))) {
		fprintf(file, "ANSWERS:\n");
		while (--n >= 0) {
			fprintf(file, "\t");
			cp = p_rr(cp, msg, file);
			if (cp == NULL)
				return;
		}
	}
	/*
	 * print name server records
	 */
	if ((n = ntohs(hp->nscount))) {
		fprintf(file, "NAME SERVERS:\n");
		while (--n >= 0) {
			fprintf(file, "\t");
			cp = p_rr(cp, msg, file);
			if (cp == NULL)
				return;
		}
	}
	/*
	 * print additional records
	 */
	if ((n = ntohs(hp->arcount))) {
		fprintf(file, "ADDITIONAL RECORDS:\n");
		while (--n >= 0) {
			fprintf(file, "\t");
			cp = p_rr(cp, msg, file);
			if (cp == NULL)
				return;
		}
	}
}

const u_char *
p_cdname(cp, msg, file)
	const u_char *cp, *msg;
	FILE *file;
{
	char name[MAXDNAME];
	int n, len;

	len = MAXCDNAME;
	if ((n = dn_expand(msg, msg + len, cp, name, sizeof(name))) < 0) {
		return (NULL);
	}
	if (name[0] == '\0') {
		name[0] = '.';
		name[1] = '\0';
	}
	fputs(name, file);
	return (cp + n);
}

const u_char *
p_fqname(cp, msg, file)
	const u_char *cp, *msg;
	FILE *file;
{
	char name[MAXDNAME];
	int n, len;

	len = MAXCDNAME;
	if ((n = dn_expand(msg, msg + len, cp, name, sizeof(name))) < 0) {
		return (NULL);
	}
	if (name[0] == '\0') {
		name[0] = '.';
		name[1] = '\0';
	}
	fputs(name, file);
	if (name[strlen(name) - 1] != '.') {
		name[0] = '.';
		name[1] = '\0';
	}
	return (cp + n);
}

/*
 * Print resource record fields in human readable form.
 */
const u_char *
p_rr(cp, msg, file)
	const u_char *cp, *msg;
	FILE *file;
{
	int type, class, dlen, n, c;
	struct in_addr inaddr;
	const u_char *cp1;

	if ((cp = p_cdname(cp, msg, file)) == NULL)
		return (NULL); /* compression error */
	fprintf(file, "\n\ttype = %s", p_type(type = getshort(cp)));
	cp += sizeof(u_short);
	fprintf(file, ", class = %s", p_class(class = getshort(cp)));
	cp += sizeof(u_short);
	fprintf(file, ", ttl = %lu", getlong(cp));
	cp += sizeof(u_long);
	fprintf(file, ", dlen = %d\n", dlen = getshort(cp));
	cp += sizeof(u_short);
	cp1 = cp;
	/*
	 * Print type specific data, if appropriate
	 */
	switch (type) {
	case T_A:
		switch (class) {
		case C_IN:
			bcopy(cp, (char *) &inaddr, sizeof(inaddr));
			if (dlen == 4) {
				fprintf(file, "\tinternet address = %s\n", inet_ntoa(inaddr));
				cp += dlen;
			} else if (dlen == 7) {
				fprintf(file, "\tinternet address = %s", inet_ntoa(inaddr));
				fprintf(file, ", protocol = %d", cp[4]);
				fprintf(file, ", port = %d\n", (cp[5] << 8) + cp[6]);
				cp += dlen;
			}
			break;
		default:
			cp += dlen;
		}
		break;
	case T_CNAME:
	case T_MB:
#ifdef OLDRR
	case T_MD:
	case T_MF:
#endif /* OLDRR */
	case T_MG:
	case T_MR:
	case T_NS:
	case T_PTR:
		fprintf(file, "\tdomain name = ");
		cp = p_cdname(cp, msg, file);
		fprintf(file, "\n");
		break;

	case T_HINFO:
		if ((n = *cp++)) {
			fprintf(file, "\tCPU=%.*s\n", n, cp);
			cp += n;
		}
		if ((n = *cp++)) {
			fprintf(file, "\tOS=%.*s\n", n, cp);
			cp += n;
		}
		break;

	case T_SOA:
		fprintf(file, "\torigin = ");
		cp = p_cdname(cp, msg, file);
		fprintf(file, "\n\tmail addr = ");
		cp = p_cdname(cp, msg, file);
		fprintf(file, "\n\tserial=%ld", getlong(cp));
		cp += sizeof(u_long);
		fprintf(file, ", refresh=%ld", getlong(cp));
		cp += sizeof(u_long);
		fprintf(file, ", retry=%ld", getlong(cp));
		cp += sizeof(u_long);
		fprintf(file, ", expire=%ld", getlong(cp));
		cp += sizeof(u_long);
		fprintf(file, ", min=%ld\n", getlong(cp));
		cp += sizeof(u_long);
		break;

	case T_MX:
		fprintf(file, "\tpreference = %d,", getshort(cp));
		cp += sizeof(u_short);
		fprintf(file, " name = ");
		cp = p_cdname(cp, msg, file);
		break;

	case T_MINFO:
		fprintf(file, "\trequests = ");
		cp = p_cdname(cp, msg, file);
		fprintf(file, "\n\terrors = ");
		cp = p_cdname(cp, msg, file);
		break;

	case T_UINFO:
		fprintf(file, "\t%s\n", cp);
		cp += dlen;
		break;

	case T_UID:
	case T_GID:
		if (dlen == 4) {
			fprintf(file, "\t%ld\n", getlong(cp));
			cp += sizeof(int);
		}
		break;

	case T_WKS:
		if (dlen < sizeof(u_long) + 1)
			break;
		bcopy(cp, (char*) &inaddr, sizeof(inaddr));
		cp += sizeof(u_long);
		fprintf(file, "\tinternet address = %s, protocol = %d\n\t",
				inet_ntoa(inaddr), *cp++);
		n = 0;
		while (cp < cp1 + dlen) {
			c = *cp++;
			do {
				if (c & 0200)
					fprintf(file, " %d", n);
				c <<= 1;
			} while (++n & 07);
		}
		putc('\n', file);
		break;

#ifdef ALLOW_T_UNSPEC
	case T_UNSPEC:
		{
			int NumBytes = 8;
			const u_char *DataPtr;
			int i;

			if (dlen < NumBytes) {
                		NumBytes = dlen;
            		}
			fprintf(file, "\tFirst %d bytes of hex data:",
				NumBytes);
			for (i = 0, DataPtr = cp; i < NumBytes; i++, DataPtr++)
				fprintf(file, " %x", *DataPtr);
			fputs("\n", file);
			cp += dlen;
		}
		break;
#endif /* ALLOW_T_UNSPEC */

	default:
		fprintf(file, "\t???\n");
		cp += dlen;
	}
	if (cp != cp1 + dlen)
		fprintf(file, "packet size error (%p != %p)\n", __UNCONST(cp),
				__UNCONST(cp1 + dlen));
	fprintf(file, "\n");
	return (cp);
}

static const char *
p_name_lookup(array, number, length, success)
	const char **array;
	int number, length;
	int *success;
{
	int i;

	for (i = 0; i < length; i++) {
		if (i == number) {
			if (success) {
				*success = 1;
			}
			return (array[i]);
		}
	}
	(void) sprintf(symbuf, "%d", number);
	if (success) {
		*success = 0;
	}
	return (symbuf);
}

/*
 * Return a string for the type
 */
const char *
p_type(type)
	int type;
{
	const char *ptype;
	static char typebuf[20];
	int len, success;

	len = (sizeof(_res_typecodes)/sizeof(_res_typecodes[0]));
	ptype = p_name_lookup(_res_typecodes, type, len, &success);
	if (success) {
		return (ptype);
	}
	sprintf(typebuf, "TYPE%d", type);
	return (typebuf);
}

/*
 * Return a mnemonic for class
 */
const char *
p_class(class)
	int class;
{
	const char *pclass;
	static char classbuf[20];
	int len, success;

	len = (sizeof(_res_classcodes)/sizeof(_res_classcodes[0]));
	pclass = p_name_lookup(_res_classcodes, class, len, &success);
	if (success) {
		return (pclass);
	}
	sprintf(classbuf, "CLASS%d", class);
	return (classbuf);
}

/*
 * Return a mnemonic for an option
 */
const char *
p_option(option)
	u_int option;
{
	const char *poption;
	static char optionbuf[20];
	int len, success;

	len = (sizeof(_res_optioncodes)/sizeof(_res_optioncodes[0]));
	poption = p_name_lookup(_res_optioncodes, option, len, &success);
	if (success) {
		return (poption);
	}
	sprintf(optionbuf, "?0x%x?", option);
	return (optionbuf);
}
