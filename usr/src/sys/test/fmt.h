/*
 * fmt.h
 *
 *  Created on: 17 Oct 2019
 *      Author: marti
 */
/* Not sure how this fits into the Unix BSD world, just yet...! */
#ifndef _FMT_H_
#define _FMT_H_

#include <types.h>

/*
 * print routines
 */
typedef struct Fmt	Fmt;
typedef int (*Fmts)(Fmt*);

struct Fmt{
	u_char	runes;			/* output buffer is runes or chars? */
	void	*start;			/* of buffer */
	void	*to;			/* current place in the buffer */
	void	*stop;			/* end of the buffer; overwritten if flush fails */
	int	    (*flush)(Fmt *);/* called when to == stop */
	void	*farg;			/* to make flush a closure */
	int	    nfmt;			/* num chars formatted so far */
	va_list	args;			/* args passed to dofmt */
	int	    r;			    /* % format Rune */
	int	    width;
	int	    prec;
	u_long	flags;
};

#endif /* _FMT_H_ */
