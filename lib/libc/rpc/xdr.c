/*	$NetBSD: xdr.c,v 1.18 1998/11/15 17:32:46 christos Exp $	*/

/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 * 
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 * 
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 * 
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 * 
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 * 
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char *sccsid = "@(#)xdr.c 1.35 87/08/12";
static char *sccsid = "@(#)xdr.c	2.1 88/07/29 4.0 RPCSRC";
#else
__RCSID("$NetBSD: xdr.c,v 1.18 1998/11/15 17:32:46 christos Exp $");
#endif
#endif

/*
 * xdr.c, Generic XDR routines implementation.
 *
 * Copyright (C) 1986, Sun Microsystems, Inc.
 *
 * These are the "generic" xdr routines used to serialize and de-serialize
 * most common data items.  See xdr.h for more info on the interface to
 * xdr.
 */

#include "namespace.h"

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <rpc/types.h>
#include <rpc/xdr.h>

#ifdef __weak_alias
__weak_alias(xdr_bool,_xdr_bool);
__weak_alias(xdr_bytes,_xdr_bytes);
__weak_alias(xdr_char,_xdr_char);
__weak_alias(xdr_enum,_xdr_enum);
__weak_alias(xdr_free,_xdr_free);
__weak_alias(xdr_int,_xdr_int);
__weak_alias(xdr_int16_t,_xdr_int16_t);
__weak_alias(xdr_int32_t,_xdr_int32_t);
__weak_alias(xdr_int64_t,_xdr_int64_t);
__weak_alias(xdr_long,_xdr_long);
__weak_alias(xdr_netobj,_xdr_netobj);
__weak_alias(xdr_opaque,_xdr_opaque);
__weak_alias(xdr_short,_xdr_short);
__weak_alias(xdr_string,_xdr_string);
__weak_alias(xdr_u_char,_xdr_u_char);
__weak_alias(xdr_u_int,_xdr_u_int);
__weak_alias(xdr_u_int16_t,_xdr_u_int16_t);
__weak_alias(xdr_u_int32_t,_xdr_u_int32_t);
__weak_alias(xdr_u_int64_t,_xdr_u_int64_t);
__weak_alias(xdr_u_long,_xdr_u_long);
__weak_alias(xdr_u_short,_xdr_u_short);
__weak_alias(xdr_union,_xdr_union);
__weak_alias(xdr_void,_xdr_void);
__weak_alias(xdr_wrapstring,_xdr_wrapstring);
#endif

/*
 * constants specific to the xdr "protocol"
 */
#define XDR_FALSE	((long) 0)
#define XDR_TRUE	((long) 1)
#define LASTUNSIGNED	((u_int) 0-1)

/*
 * for unit alignment
 */
static char xdr_zero[BYTES_PER_XDR_UNIT] = { 0, 0, 0, 0 };

/*
 * Free a data structure using XDR
 * Not a filter, but a convenient utility nonetheless
 */
void
xdr_free(proc, objp)
	xdrproc_t proc;
	char *objp;
{
	XDR x;
	
	x.x_op = XDR_FREE;
	(*proc)(&x, objp);
}

/*
 * XDR nothing
 */
bool_t
xdr_void(/* xdrs, addr */)
	/* XDR *xdrs; */
	/* caddr_t addr; */
{

	return (TRUE);
}


/*
 * XDR integers
 */
bool_t
xdr_int(xdrs, ip)
	XDR *xdrs;
	int *ip;
{
	long l;

	switch (xdrs->x_op) {

	case XDR_ENCODE:
		l = (long) *ip;
		return (XDR_PUTLONG(xdrs, &l));

	case XDR_DECODE:
		if (!XDR_GETLONG(xdrs, &l)) {
			return (FALSE);
		}
		*ip = (int) l;
		return (TRUE);

	case XDR_FREE:
		return (TRUE);
	}
	/* NOTREACHED */
	return (FALSE);
}

/*
 * XDR unsigned integers
 */
bool_t
xdr_u_int(xdrs, up)
	XDR *xdrs;
	u_int *up;
{
	u_long l;

	switch (xdrs->x_op) {

	case XDR_ENCODE:
		l = (u_long) *up;
		return (XDR_PUTLONG(xdrs, (long *)&l));

	case XDR_DECODE:
		if (!XDR_GETLONG(xdrs, (long *)&l)) {
			return (FALSE);
		}
		*up = (u_int) l;
		return (TRUE);

	case XDR_FREE:
		return (TRUE);
	}
	/* NOTREACHED */
	return (FALSE);
}

/*
 * XDR long integers
 * same as xdr_u_long - open coded to save a proc call!
 */
bool_t
xdr_long(xdrs, lp)
	XDR *xdrs;
	long *lp;
{
	switch (xdrs->x_op) {
	case XDR_ENCODE:
		return (XDR_PUTLONG(xdrs, lp));
	case XDR_DECODE:
		return (XDR_GETLONG(xdrs, lp));
	case XDR_FREE:
		return (TRUE);
	}
	/* NOTREACHED */
	return (FALSE);
}

/*
 * XDR unsigned long integers
 * same as xdr_long - open coded to save a proc call!
 */
bool_t
xdr_u_long(xdrs, ulp)
	XDR *xdrs;
	u_long *ulp;
{
	switch (xdrs->x_op) {
	case XDR_ENCODE:
		return (XDR_PUTLONG(xdrs, (long *)ulp));
	case XDR_DECODE:
		return (XDR_GETLONG(xdrs, (long *)ulp));
	case XDR_FREE:
		return (TRUE);
	}
	/* NOTREACHED */
	return (FALSE);
}


/*
 * XDR 32-bit integers
 * same as xdr_u_int32_t - open coded to save a proc call!
 */
bool_t
xdr_int32_t(xdrs, int32_p)
	XDR *xdrs;
	int32_t *int32_p;
{
	long l;

	switch (xdrs->x_op) {

	case XDR_ENCODE:
		l = (long) *int32_p;
		return (XDR_PUTLONG(xdrs, &l));

	case XDR_DECODE:
		if (!XDR_GETLONG(xdrs, &l)) {
			return (FALSE);
		}
		*int32_p = (int32_t) l;
		return (TRUE);

	case XDR_FREE:
		return (TRUE);
	}
	/* NOTREACHED */
	return (FALSE);
}

/*
 * XDR unsigned 32-bit integers
 * same as xdr_int32_t - open coded to save a proc call!
 */
bool_t
xdr_u_int32_t(xdrs, u_int32_p)
	XDR *xdrs;
	u_int32_t *u_int32_p;
{
	u_long l;

	switch (xdrs->x_op) {

	case XDR_ENCODE:
		l = (u_long) *u_int32_p;
		return (XDR_PUTLONG(xdrs, (long *)&l));

	case XDR_DECODE:
		if (!XDR_GETLONG(xdrs, (long *)&l)) {
			return (FALSE);
		}
		*u_int32_p = (u_int32_t) l;
		return (TRUE);

	case XDR_FREE:
		return (TRUE);
	}
	/* NOTREACHED */
	return (FALSE);
}

/*
 * XDR short integers
 */
bool_t
xdr_short(xdrs, sp)
	XDR *xdrs;
	short *sp;
{
	long l;

	switch (xdrs->x_op) {

	case XDR_ENCODE:
		l = (long) *sp;
		return (XDR_PUTLONG(xdrs, &l));

	case XDR_DECODE:
		if (!XDR_GETLONG(xdrs, &l)) {
			return (FALSE);
		}
		*sp = (short) l;
		return (TRUE);

	case XDR_FREE:
		return (TRUE);
	}
	/* NOTREACHED */
	return (FALSE);
}

/*
 * XDR unsigned short integers
 */
bool_t
xdr_u_short(xdrs, usp)
	XDR *xdrs;
	u_short *usp;
{
	u_long l;

	switch (xdrs->x_op) {

	case XDR_ENCODE:
		l = (u_long) *usp;
		return (XDR_PUTLONG(xdrs, (long *)&l));

	case XDR_DECODE:
		if (!XDR_GETLONG(xdrs, (long *)&l)) {
			return (FALSE);
		}
		*usp = (u_short) l;
		return (TRUE);

	case XDR_FREE:
		return (TRUE);
	}
	/* NOTREACHED */
	return (FALSE);
}


/*
 * XDR 16-bit integers
 */
bool_t
xdr_int16_t(xdrs, int16_p)
	XDR *xdrs;
	int16_t *int16_p;
{
	long l;

	switch (xdrs->x_op) {

	case XDR_ENCODE:
		l = (long) *int16_p;
		return (XDR_PUTLONG(xdrs, &l));

	case XDR_DECODE:
		if (!XDR_GETLONG(xdrs, &l)) {
			return (FALSE);
		}
		*int16_p = (int16_t) l;
		return (TRUE);

	case XDR_FREE:
		return (TRUE);
	}
	/* NOTREACHED */
	return (FALSE);
}

/*
 * XDR unsigned 16-bit integers
 */
bool_t
xdr_u_int16_t(xdrs, u_int16_p)
	XDR *xdrs;
	u_int16_t *u_int16_p;
{
	u_long l;

	switch (xdrs->x_op) {

	case XDR_ENCODE:
		l = (u_long) *u_int16_p;
		return (XDR_PUTLONG(xdrs, (long *)&l));

	case XDR_DECODE:
		if (!XDR_GETLONG(xdrs, (long *)&l)) {
			return (FALSE);
		}
		*u_int16_p = (u_int16_t) l;
		return (TRUE);

	case XDR_FREE:
		return (TRUE);
	}
	/* NOTREACHED */
	return (FALSE);
}


/*
 * XDR a char
 */
bool_t
xdr_char(xdrs, cp)
	XDR *xdrs;
	char *cp;
{
	int i;

	i = (*cp);
	if (!xdr_int(xdrs, &i)) {
		return (FALSE);
	}
	*cp = i;
	return (TRUE);
}

/*
 * XDR an unsigned char
 */
bool_t
xdr_u_char(xdrs, cp)
	XDR *xdrs;
	u_char *cp;
{
	u_int u;

	u = (*cp);
	if (!xdr_u_int(xdrs, &u)) {
		return (FALSE);
	}
	*cp = u;
	return (TRUE);
}

/*
 * XDR booleans
 */
bool_t
xdr_bool(xdrs, bp)
	XDR *xdrs;
	bool_t *bp;
{
	long lb;

	switch (xdrs->x_op) {

	case XDR_ENCODE:
		lb = *bp ? XDR_TRUE : XDR_FALSE;
		return (XDR_PUTLONG(xdrs, &lb));

	case XDR_DECODE:
		if (!XDR_GETLONG(xdrs, &lb)) {
			return (FALSE);
		}
		*bp = (lb == XDR_FALSE) ? FALSE : TRUE;
		return (TRUE);

	case XDR_FREE:
		return (TRUE);
	}
	/* NOTREACHED */
	return (FALSE);
}

/*
 * XDR enumerations
 */
bool_t
xdr_enum(xdrs, ep)
	XDR *xdrs;
	enum_t *ep;
{
	enum sizecheck { SIZEVAL };	/* used to find the size of an enum */

	/*
	 * enums are treated as ints
	 */
	/* LINTED */ if (sizeof (enum sizecheck) == sizeof (long)) {
		return (xdr_long(xdrs, (long *)ep));
	} else /* LINTED */ if (sizeof (enum sizecheck) == sizeof (int)) {
		return (xdr_int(xdrs, (int *)ep));
	} else /* LINTED */ if (sizeof (enum sizecheck) == sizeof (short)) {
		return (xdr_short(xdrs, (short *)ep));
	} else {
		return (FALSE);
	}
}

/*
 * XDR opaque data
 * Allows the specification of a fixed size sequence of opaque bytes.
 * cp points to the opaque object and cnt gives the byte length.
 */
bool_t
xdr_opaque(xdrs, cp, cnt)
	XDR *xdrs;
	caddr_t cp;
	u_int cnt;
{
	u_int rndup;
	static int crud[BYTES_PER_XDR_UNIT];

	/*
	 * if no data we are done
	 */
	if (cnt == 0)
		return (TRUE);

	/*
	 * round byte count to full xdr units
	 */
	rndup = cnt % BYTES_PER_XDR_UNIT;
	if (rndup > 0)
		rndup = BYTES_PER_XDR_UNIT - rndup;

	if (xdrs->x_op == XDR_DECODE) {
		if (!XDR_GETBYTES(xdrs, cp, cnt)) {
			return (FALSE);
		}
		if (rndup == 0)
			return (TRUE);
		return (XDR_GETBYTES(xdrs, (caddr_t)crud, rndup));
	}

	if (xdrs->x_op == XDR_ENCODE) {
		if (!XDR_PUTBYTES(xdrs, cp, cnt)) {
			return (FALSE);
		}
		if (rndup == 0)
			return (TRUE);
		return (XDR_PUTBYTES(xdrs, xdr_zero, rndup));
	}

	if (xdrs->x_op == XDR_FREE) {
		return (TRUE);
	}

	return (FALSE);
}

/*
 * XDR counted bytes
 * *cpp is a pointer to the bytes, *sizep is the count.
 * If *cpp is NULL maxsize bytes are allocated
 */
bool_t
xdr_bytes(xdrs, cpp, sizep, maxsize)
	XDR *xdrs;
	char **cpp;
	u_int *sizep;
	u_int maxsize;
{
	char *sp = *cpp;  /* sp is the actual string pointer */
	u_int nodesize;

	/*
	 * first deal with the length since xdr bytes are counted
	 */
	if (! xdr_u_int(xdrs, sizep)) {
		return (FALSE);
	}
	nodesize = *sizep;
	if ((nodesize > maxsize) && (xdrs->x_op != XDR_FREE)) {
		return (FALSE);
	}

	/*
	 * now deal with the actual bytes
	 */
	switch (xdrs->x_op) {

	case XDR_DECODE:
		if (nodesize == 0) {
			return (TRUE);
		}
		if (sp == NULL) {
			*cpp = sp = (char *)mem_alloc(nodesize);
		}
		if (sp == NULL) {
			warnx("xdr_bytes: out of memory");
			return (FALSE);
		}
		/* FALLTHROUGH */

	case XDR_ENCODE:
		return (xdr_opaque(xdrs, sp, nodesize));

	case XDR_FREE:
		if (sp != NULL) {
			mem_free(sp, nodesize);
			*cpp = NULL;
		}
		return (TRUE);
	}
	/* NOTREACHED */
	return (FALSE);
}

/*
 * Implemented here due to commonality of the object.
 */
bool_t
xdr_netobj(xdrs, np)
	XDR *xdrs;
	struct netobj *np;
{

	return (xdr_bytes(xdrs, &np->n_bytes, &np->n_len, MAX_NETOBJ_SZ));
}

/*
 * XDR a descriminated union
 * Support routine for discriminated unions.
 * You create an array of xdrdiscrim structures, terminated with
 * an entry with a null procedure pointer.  The routine gets
 * the discriminant value and then searches the array of xdrdiscrims
 * looking for that value.  It calls the procedure given in the xdrdiscrim
 * to handle the discriminant.  If there is no specific routine a default
 * routine may be called.
 * If there is no specific or default routine an error is returned.
 */
bool_t
xdr_union(xdrs, dscmp, unp, choices, dfault)
	XDR *xdrs;
	enum_t *dscmp;		/* enum to decide which arm to work on */
	char *unp;		/* the union itself */
	struct xdr_discrim *choices;	/* [value, xdr proc] for each arm */
	xdrproc_t dfault;	/* default xdr routine */
{
	enum_t dscm;

	/*
	 * we deal with the discriminator;  it's an enum
	 */
	if (! xdr_enum(xdrs, dscmp)) {
		return (FALSE);
	}
	dscm = *dscmp;

	/*
	 * search choices for a value that matches the discriminator.
	 * if we find one, execute the xdr routine for that value.
	 */
	for (; choices->proc != NULL_xdrproc_t; choices++) {
		if (choices->value == dscm)
			return ((*(choices->proc))(xdrs, unp));
	}

	/*
	 * no match - execute the default xdr routine if there is one
	 */
	return ((dfault == NULL_xdrproc_t) ? FALSE :
	    (*dfault)(xdrs, unp));
}


/*
 * Non-portable xdr primitives.
 * Care should be taken when moving these routines to new architectures.
 */


/*
 * XDR null terminated ASCII strings
 * xdr_string deals with "C strings" - arrays of bytes that are
 * terminated by a NULL character.  The parameter cpp references a
 * pointer to storage; If the pointer is null, then the necessary
 * storage is allocated.  The last parameter is the max allowed length
 * of the string as specified by a protocol.
 */
bool_t
xdr_string(xdrs, cpp, maxsize)
	XDR *xdrs;
	char **cpp;
	u_int maxsize;
{
	char *sp = *cpp;  /* sp is the actual string pointer */
	u_int size;
	u_int nodesize;

	/*
	 * first deal with the length since xdr strings are counted-strings
	 */
	switch (xdrs->x_op) {
	case XDR_FREE:
		if (sp == NULL) {
			return(TRUE);	/* already free */
		}
		/* FALLTHROUGH */
	case XDR_ENCODE:
		size = strlen(sp);
		break;
	case XDR_DECODE:
		break;
	}
	if (! xdr_u_int(xdrs, &size)) {
		return (FALSE);
	}
	if (size > maxsize) {
		return (FALSE);
	}
	nodesize = size + 1;

	/*
	 * now deal with the actual bytes
	 */
	switch (xdrs->x_op) {

	case XDR_DECODE:
		if (nodesize == 0) {
			return (TRUE);
		}
		if (sp == NULL)
			*cpp = sp = (char *)mem_alloc(nodesize);
		if (sp == NULL) {
			warnx("xdr_string: out of memory");
			return (FALSE);
		}
		sp[size] = 0;
		/* FALLTHROUGH */

	case XDR_ENCODE:
		return (xdr_opaque(xdrs, sp, size));

	case XDR_FREE:
		mem_free(sp, nodesize);
		*cpp = NULL;
		return (TRUE);
	}
	/* NOTREACHED */
	return (FALSE);
}

/* 
 * Wrapper for xdr_string that can be called directly from 
 * routines like clnt_call
 */
bool_t
xdr_wrapstring(xdrs, cpp)
	XDR *xdrs;
	char **cpp;
{
	return xdr_string(xdrs, cpp, LASTUNSIGNED);
}

/*
 * XDR 64-bit integers
 */
bool_t
xdr_int64_t(xdrs, int64_p)
	XDR *xdrs;
	int64_t *int64_p;
{
	u_long ul[2];

	switch (xdrs->x_op) {
	case XDR_ENCODE:
		ul[0] = (u_long) (((uint64_t) * int64_p >> 32)
				& (uint64_t) 0xffffffffULL);
		ul[1] = (u_long) (((uint64_t) * int64_p) & (uint64_t) 0xffffffffULL);
		if (XDR_PUTLONG(xdrs, (long *)&ul[0]) == FALSE)
			return (FALSE);
		return (XDR_PUTLONG(xdrs, (long* )&ul[1]));
	case XDR_DECODE:
		if (XDR_GETLONG(xdrs, (long *)&ul[0]) == FALSE)
			return (FALSE);
		if (XDR_GETLONG(xdrs, (long *)&ul[1]) == FALSE)
			return (FALSE);
		*int64_p = (int64_t)(((u_int64_t) ul[0] << 32) | ((u_int64_t) ul[1]));
		return (TRUE);
	case XDR_FREE:
		return (TRUE);
	}
	/* NOTREACHED */
	return (FALSE);
}

/*
 * XDR unsigned 64-bit integers
 */
bool_t
xdr_u_int64_t(xdrs, u_int64_p)
	XDR *xdrs;
	u_int64_t *u_int64_p;
{
	u_long ul[2];

	switch (xdrs->x_op) {
	case XDR_ENCODE:
		ul[0] = (u_long) (*u_int64_p >> 32) & 0xffffffffUL;
		ul[1] = (u_long) (*u_int64_p) & 0xffffffffUL;
		if (XDR_PUTLONG(xdrs, (long *)&ul[0]) == FALSE)
			return (FALSE);
		return (XDR_PUTLONG(xdrs, (long* )&ul[1]));
	case XDR_DECODE:
		if (XDR_GETLONG(xdrs, (long *)&ul[0]) == FALSE)
			return (FALSE);
		if (XDR_GETLONG(xdrs, (long *)&ul[1]) == FALSE)
			return (FALSE);
		*u_int64_p = (u_int64_t)(
				((u_int64_t) ul[0] << 32) | ((u_int64_t) ul[1]));
		return (TRUE);
	case XDR_FREE:
		return (TRUE);
	}
	/* NOTREACHED */
	return (FALSE);
}
