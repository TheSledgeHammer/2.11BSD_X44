/*-
 * Copyright (c) 1985, 1993
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 * -
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
 * -
 * --Copyright--
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
static char sccsid[] = "@(#)res_mkquery.c	6.7 (Berkeley) 3/7/88";
static char sccsid[] = "@(#)res_mkquery.c	8.1 (Berkeley) 6/4/93";
static char rcsid[] = "$Id: res_mkquery.c,v 4.9.1.2 1993/05/17 10:00:01 vixie Exp $";
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"
#include <sys/types.h>
#include <netinet/in.h>

#include <arpa/nameser.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <resolv.h>

#include "res_private.h"

#if 0
#ifdef __weak_alias
__weak_alias(res_nmkquery,_res_nmkquery)
#endif
#endif

static struct rrec *res_newrr(const u_char *);
static int res_rr_nmkquery(res_state, int, const char *, int, int, const u_char *, int, struct rrec *, u_char *, int);

int
res_nmkquery(statp, op, dname, class, type, data, datalen, newrr_in, buf, buflen)
	res_state statp;
	int op;
	const char *dname;
	int class, type;
	const u_char *data;
	int datalen;
	const u_char *newrr_in;
	u_char *buf;
	int buflen;
{
	struct rrec *newrr;

	newrr = res_newrr(newrr_in);
	return (res_rr_nmkquery(statp, op, dname, class, type, data, datalen, newrr, buf, buflen));
}

static struct rrec *
res_newrr(newrr_in)
	const u_char *newrr_in;
{
	struct rrec *newrr;

	newrr = (struct rrec *)malloc(sizeof(struct rrec));
	if ((newrr != NULL) && (newrr_in != NULL)) {
		newrr->r_data = newrr_in;
		return (newrr);
	}
	free(newrr);
	return (NULL);
}

/*
 * Form all types of queries.
 * Returns the size of the result or -1.
 */
static int
res_rr_nmkquery(statp, op, dname, class, type, data, datalen, newrr, buf, buflen)
	res_state statp;	/* res state */
	int op;			/* opcode of query */
	const char *dname;		/* domain name */
	int class, type;	/* class and type of query */
	const u_char *data;		/* resource record data */
	int datalen;		/* length of data */
	struct rrec *newrr;	/* new rr for modify or append */
	u_char *buf;		/* buffer to put query */
	int buflen;		/* size of buffer */
{
	register HEADER *hp;
	register u_char *cp;
	register int n;
	char dnbuf[MAXDNAME];
	u_char *dnptrs[10], **dpp, **lastdnptr;

#ifdef DEBUG
	if (statp->options & RES_DEBUG)
		printf("res_mkquery(%d, %s, %d, %d)\n", op, dname, class, type);
#endif /* DEBUG */
	/*
	 * Initialize header fields.
	 */
	hp = (HEADER *) buf;
	statp->id = res_nrandomid(statp);
	hp->id = htons(++statp->id);
	hp->opcode = op;
	hp->qr = hp->aa = hp->tc = hp->ra = 0;
	hp->pr = (statp->options & RES_PRIMARY) != 0;
	hp->rd = (statp->options & RES_RECURSE) != 0;
	hp->rcode = NOERROR;
	hp->qdcount = 0;
	hp->ancount = 0;
	hp->nscount = 0;
	hp->arcount = 0;
	cp = buf + sizeof(HEADER);
	buflen -= sizeof(HEADER);
	dpp = dnptrs;
	*dpp++ = buf;
	*dpp++ = NULL;
	lastdnptr = dnptrs + sizeof(dnptrs)/sizeof(dnptrs[0]);
	/*
	 * If the domain name contains no dots (single label), then
	 * append the default domain name to the one given.
	 */
	if ((statp->options & RES_DEFNAMES) && dname != 0 && dname[0] != '\0' &&
	    strchr(dname, '.') == NULL) {
		if (!(statp->options & RES_INIT))
			if (res_ninit(statp) == -1)
				return(-1);
		if (statp->defdname[0] != '\0') {
			(void)snprintf(dnbuf, sizeof(dnbuf)+1, "%s.%s", dname, statp->defdname);
			dname = dnbuf;
		}
	}
	/*
	 * perform opcode specific processing
	 */
	switch (op) {
	case QUERY:
		buflen -= QFIXEDSZ;
		if ((n = dn_comp((const u_char *)dname, cp, buflen, dnptrs, lastdnptr)) < 0)
			return (-1);
		cp += n;
		buflen -= n;
		putshort(type, cp);
		cp += sizeof(u_short);
		putshort(class, cp);
		cp += sizeof(u_short);
		hp->qdcount = htons(1);
		if (op == QUERY || data == NULL)
			break;
		/*
		 * Make an additional record for completion domain.
		 */
		buflen -= RRFIXEDSZ;
		if ((n = dn_comp(data, cp, buflen, dnptrs, lastdnptr)) < 0)
			return (-1);
		cp += n;
		buflen -= n;
		putshort(T_NULL, cp);
		cp += sizeof(u_short);
		putshort(class, cp);
		cp += sizeof(u_short);
		putlong((long)0, cp);
		cp += sizeof(u_long);
		putshort(0, cp);
		cp += sizeof(u_short);
		hp->arcount = htons(1);
		break;

	case IQUERY:
		/*
		 * Initialize answer section
		 */
		if (buflen < 1 + RRFIXEDSZ + datalen)
			return (-1);
		*cp++ = '\0'; /* no domain name */
		putshort(type, cp);
		cp += sizeof(u_short);
		putshort(class, cp);
		cp += sizeof(u_short);
		putlong((long) 0, cp);
		cp += sizeof(u_long);
		putshort(datalen, cp);
		cp += sizeof(u_short);
		if (datalen) {
			bcopy(data, cp, datalen);
			cp += datalen;
		}
		hp->ancount = htons(1);
		break;

#ifdef ALLOW_UPDATES
	/*
	 * For UPDATEM/UPDATEMA, do UPDATED/UPDATEDA followed by UPDATEA
	 * (Record to be modified is followed by its replacement in msg.)
	 */
	case UPDATEM:
	case UPDATEMA:

	case UPDATED:
		/*
		 * The res code for UPDATED and UPDATEDA is the same; user
		 * calls them differently: specifies data for UPDATED; server
		 * ignores data if specified for UPDATEDA.
		 */
	case UPDATEDA:
		buflen -= RRFIXEDSZ + datalen;
		if ((n = dn_comp((const u_char *)dname, cp, buflen, dnptrs, lastdnptr)) < 0)
			return (-1);
		cp += n;
		putshort(type, cp);
		cp += sizeof(u_short);
		putshort(class, cp);
		cp += sizeof(u_short);
		putlong((long) 0, cp);
		cp += sizeof(u_long);
		putshort(datalen, cp);
		cp += sizeof(u_short);
		if (datalen) {
			bcopy(data, cp, datalen);
			cp += datalen;
		}
		if ((op == UPDATED) || (op == UPDATEDA)) {
			hp->ancount = htons(0);
			break;
		}
		/* Else UPDATEM/UPDATEMA, so drop into code for UPDATEA */

	case UPDATEA:	/* Add new resource record */
		buflen -= RRFIXEDSZ + datalen;
		if ((n = dn_comp((const u_char *)dname, cp, buflen, dnptrs, lastdnptr)) < 0)
			return (-1);
		cp += n;
		putshort(newrr->r_type, cp);
		cp += sizeof(u_short);
		putshort(newrr->r_class, cp);
		cp += sizeof(u_short);
		putlong((long) 0, cp);
		cp += sizeof(u_long);
		putshort(newrr->r_size, cp);
	        cp += sizeof(u_short);
		if (newrr->r_size) {
			bcopy(newrr->r_data, cp, newrr->r_size);
			cp += newrr->r_size;
		}
		hp->ancount = htons(0);
		break;

#endif /* ALLOW_UPDATES */
	}
	return (cp - buf);
}
