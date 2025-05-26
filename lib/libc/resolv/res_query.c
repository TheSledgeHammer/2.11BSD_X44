/*-
 * Copyright (c) 1988, 1993
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
 * Copyright (c) 1988 Regents of the University of California.
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
static char sccsid[] = "@(#)res_query.c	5.3 (Berkeley) 4/5/88";
#endif
#endif /* LIBC_SCCS and not lint */

#include <sys/param.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <netinet/in.h>

#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <resolv.h>

#include "res_private.h"


#if 0
#ifdef __weak_alias
__weak_alias(res_nquery,_res_nquery)
__weak_alias(res_nsearch,_res_nsearch)
__weak_alias(res_nquerydomain,__res_nquerydomain)
__weak_alias(res_hostalias,__res_hostalias)
#endif
#endif

#if PACKETSZ > 1024
#define MAXPACKET	PACKETSZ
#else
#define MAXPACKET	1024
#endif

int h_errno;

/*
 * Formulate a normal query, send, and await answer.
 * Returned answer is placed in supplied buffer "answer".
 * Perform preliminary check of answer, returning success only
 * if no error is indicated and the answer count is nonzero.
 * Return the size of the response on success, -1 on error.
 * Error number is left in h_errno.
 * Caller must parse answer and determine whether it answers the question.
 */
int
res_nquery(statp, name, class, type, answer, anslen)
	res_state statp;
	char *name;		/* domain name */
	int class, type;	/* class and type of query */
	u_char *answer;		/* buffer to put answer */
	int anslen;		/* size of answer buffer */
{
	u_char buf[MAXPACKET];
	HEADER *hp;
	int n;

	if ((statp->options & RES_INIT) == 0 && res_ninit(statp) == -1)
		return (-1);
#ifdef DEBUG
	if (statp->options & RES_DEBUG)
		printf("res_query(%s, %d, %d)\n", name, class, type);
#endif
	n = res_nmkquery(statp, QUERY, name, class, type, NULL, 0, NULL, buf, sizeof(buf));

	if (n <= 0) {
#ifdef DEBUG
		if (statp->options & RES_DEBUG)
			printf("res_query: mkquery failed\n");
#endif
		h_errno = NO_RECOVERY;
		return (n);
	}
	n = res_nsend(statp, buf, n, answer, anslen);
	if (n < 0) {
#ifdef DEBUG
		if (statp->options & RES_DEBUG)
			printf("res_query: send error\n");
#endif
		h_errno = TRY_AGAIN;
		return(n);
	}

	hp = (HEADER *) answer;
	if (hp->rcode != NOERROR || ntohs(hp->ancount) == 0) {
#ifdef DEBUG
		if (statp->options & RES_DEBUG)
			printf("rcode = %d, ancount=%d\n", hp->rcode,
			    ntohs(hp->ancount));
#endif
		switch (hp->rcode) {
			case NXDOMAIN:
				h_errno = HOST_NOT_FOUND;
				break;
			case SERVFAIL:
				h_errno = TRY_AGAIN;
				break;
			case NOERROR:
				h_errno = NO_DATA;
				break;
			case FORMERR:
			case NOTIMP:
			case REFUSED:
			default:
				h_errno = NO_RECOVERY;
				break;
		}
		return (-1);
	}
	return (n);
}

/*
 * Formulate a normal query, send, and retrieve answer in supplied buffer.
 * Return the size of the response on success, -1 on error.
 * If enabled, implement search rules until answer or unrecoverable failure
 * is detected.  Error number is left in h_errno.
 * Only useful for queries in the same name hierarchy as the local host
 * (not, for example, for host address-to-name lookups in domain in-addr.arpa).
 */
int
res_nsearch(statp, name, class, type, answer, anslen)
	res_state statp;	/* res state */
	char *name;			/* domain name */
	int class, type;	/* class and type of query */
	u_char *answer;		/* buffer to put answer */
	int anslen;		/* size of answer */
{
	register char *cp, **domain;
	int n, ret;

	if ((statp->options & RES_INIT) == 0 && res_ninit(statp) == -1)
		return (-1);

	errno = 0;
	h_errno = HOST_NOT_FOUND;		/* default, if we never query */
	for (cp = name, n = 0; *cp; cp++) {
		if (*cp == '.') {
			n++;
		}
	}
	if (n == 0 && (cp = hostalias(name)))
		return (res_nquery(statp, cp, class, type, answer, anslen));

	if ((n == 0 || *--cp != '.') && (statp->options & RES_DEFNAMES)) {
		for (domain = statp->dnsrch; *domain; domain++) {
			h_errno = 0;
			ret = res_nquerydomain(statp, name, *domain, class, type, answer,
					anslen);
			if (ret > 0)
				return (ret);
			/*
			 * If no server present, give up.
			 * If name isn't found in this domain,
			 * keep trying higher domains in the search list
			 * (if that's enabled).
			 * On a NO_DATA error, keep trying, otherwise
			 * a wildcard entry of another type could keep us
			 * from finding this entry higher in the domain.
			 * If we get some other error (non-authoritative negative
			 * answer or server failure), then stop searching up,
			 * but try the input name below in case it's fully-qualified.
			 */
			if (errno == ECONNREFUSED) {
				h_errno = TRY_AGAIN;
				return (-1);
			}
			if ((h_errno != HOST_NOT_FOUND && h_errno != NO_DATA)
					|| (statp->options & RES_DNSRCH) == 0)
				break;
		}
	}
	/*
	 * If the search/default failed, try the name as fully-qualified,
	 * but only if it contained at least one dot (even trailing).
	 */
	if (n) {
		return (res_nquerydomain(statp, name, (char*) NULL, class, type, answer,
				anslen));
	}
	return (-1);
}

/*
 * Perform a call on res_query on the concatenation of name and domain,
 * removing a trailing dot from name if domain is NULL.
 */
int
res_nquerydomain(statp, name, domain, class, type, answer, anslen)
	res_state statp;	/* res state */
	char *name, *domain;
	int class, type;	/* class and type of query */
	u_char *answer;		/* buffer to put answer */
	int anslen;			/* size of answer */
{
	char nbuf[2*MAXDNAME+2];
	char *longname = nbuf;
	int n;

#ifdef DEBUG
	if (statp->options & RES_DEBUG)
		printf("res_querydomain(%s, %s, %d, %d)\n", name, domain, class, type);
#endif
	if (domain == NULL) {
		/*
		 * Check for trailing '.';
		 * copy without '.' if present.
		 */
		n = strlen(name) - 1;
		if (name[n] == '.' && n < sizeof(nbuf) - 1) {
			bcopy(name, nbuf, n);
			nbuf[n] = '\0';
		} else
			longname = name;
	} else
		(void)sprintf(nbuf, "%.*s.%.*s",
		    MAXDNAME, name, MAXDNAME, domain);

	return (res_nquery(statp, longname, class, type, answer, anslen));
}

const char *
res_hostalias(statp, name, dst, size)
	res_state statp;
	const char *name;
	char *dst;
	size_t size;
{
	register char *C1, *C2;
	FILE *fp;
	char *file;
	char buf[BUFSIZ];

	if (statp->options & RES_NOALIASES) {
		return (NULL);
	}

	/*
	 * forbid hostaliases for setuid binary, due to possible security
	 * breach.
	 */
	/*
	if (issetugid()) {
		return (NULL);
	}
	*/
	file = getenv("HOSTALIASES");
	if (file == NULL || (fp = fopen(file, "r")) == NULL)
		return (NULL);
	buf[sizeof(buf) - 1] = '\0';
	while (fgets(buf, sizeof(buf), fp)) {
		for (C1 = buf; *C1 && !isspace(*C1); ++C1)
			;
		if (!*C1)
			break;
		*C1 = '\0';
		if (!strcasecmp(buf, name)) {
			while (isspace(*++C1))
				;
			if (!*C1)
				break;
			for (C2 = C1 + 1; *C2 && !isspace(*C2); ++C2)
				;
			*C2 = '\0';
			strncpy(dst, C1, size - 1);
			dst[size -1] = '\0';
			fclose(fp);
			return (dst);
		}
	}
	fclose(fp);
	return (NULL);
}
