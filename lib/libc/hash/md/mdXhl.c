/*	$NetBSD: mdXhl.c,v 1.13 2014/09/24 13:18:52 christos Exp $	*/

/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <phk@login.dkuug.dk> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.   Poul-Henning Kamp
 * ----------------------------------------------------------------------------
 *
 * from FreeBSD Id: mdXhl.c,v 1.8 1996/10/25 06:48:12 bde Exp
 */

/*
 * Modified April 29, 1997 by Jason R. Thorpe <thorpej@NetBSD.org>
 */

#ifdef MDALGORITHM

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

/*
 * Do all the name mangling before we include "namespace.h"
 */
#define	CONCAT(x,y)	__CONCAT(x,y)

#ifndef MD_FNPREFIX
#define MD_FNPREFIX     MDALGORITHM
#endif /* !MD_FNPREFIX */

#define	FNPREFIX(x)	    CONCAT(MD_FNPREFIX,x)
#define	MD_CTX	        CONCAT(MDALGORITHM,_CTX)
#define	MD_LEN	        CONCAT(MDALGORITHM,_DIGEST_LENGTH)
#define	MD_STRLEN	    CONCAT(MDALGORITHM,_DIGEST_STRING_LENGTH)

#define	MDNAME(x)	    CONCAT(MDALGORITHM,x)

#if !defined(_KERNEL) && defined(__weak_alias) && !defined(HAVE_NBTOOL_CONFIG_H)
#define	WA(a,b)	__weak_alias(a,b)
WA(FNPREFIX(End),CONCAT(_,FNPREFIX(End)))
WA(FNPREFIX(File),CONCAT(_,FNPREFIX(File)))
WA(FNPREFIX(Data),CONCAT(_,FNPREFIX(Data)))
#undef WA
#endif

#include "namespace.h"

#include <sys/types.h>
#include MDINCLUDE

#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

char *
FNPREFIX(End)(MD_CTX *ctx, char *buf)
{
	int i;
	unsigned char digest[MD_LEN];
	static const char hex[]="0123456789abcdef";

	_DIAGASSERT(ctx != 0);

	if (buf == NULL)
		buf = malloc((size_t)MD_STRLEN);
	if (buf == NULL)
		return (NULL);

	FNPREFIX(Final)(digest, ctx);

	for (i = 0; i < MD_LEN; i++) {
		buf[i+i] = hex[(u_int32_t)digest[i] >> 4];
		buf[i+i+1] = hex[digest[i] & 0x0f];
	}

	buf[i+i] = '\0';
	return (buf);
}

char *
FNPREFIX(File)(const char *filename, char *buf)
{
	unsigned char buffer[BUFSIZ];
	MD_CTX ctx;
	int f, j;
	ssize_t i;

	_DIAGASSERT(filename != 0);
	/* buf may be NULL */

	FNPREFIX(Init)(&ctx);
	f = open(filename, O_RDONLY | O_CLOEXEC, 0666);
	if (f < 0)
		return NULL;

	while ((i = read(f, buffer, sizeof(buffer))) > 0)
		FNPREFIX(Update)(&ctx, buffer, (unsigned int)i);

	j = errno;
	close(f);
	errno = j;

	if (i < 0)
		return NULL;

	return (FNPREFIX(End)(&ctx, buf));
}

char *
FNPREFIX(Data)(const unsigned char *data, unsigned int len, char *buf)
{
	MD_CTX ctx;

	_DIAGASSERT(data != 0);

	FNPREFIX(Init)(&ctx);
	FNPREFIX(Update)(&ctx, data, len);
	return (FNPREFIX(End)(&ctx, buf));
}

#endif /* MDALGORITHM */
