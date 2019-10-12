/*
 * types2.h
 *
 *  Created on: 12 Oct 2019
 *      Author: marti
 */

/* Temporary: Extends types.h with FreeBSD related headers for imgact.h. May contain duplicates.*/

#ifndef _TYPES2_
#define	_TYPES2_

#include <sys/cdefs.h>
#include <sys/types.h>

/* Machine type dependent parameters. */
#include <machine/endian.h>

#ifndef _POSIX_SOURCE
typedef	unsigned char	u_char;
typedef	unsigned short	u_short;
typedef	unsigned int	u_int;
typedef	unsigned long	u_long;
typedef	unsigned short	ushort;		/* Sys V compatibility */
typedef	unsigned int	uint;		/* Sys V compatibility */
#endif

typedef	char *			caddr_t;	/* core address */

#endif
