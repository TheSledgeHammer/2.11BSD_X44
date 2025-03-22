/*	$NetBSD: pw_private.h,v 1.2 2003/07/26 19:24:43 salo Exp $	*/

/*
 * Written by Jason R. Thorpe <thorpej@NetBSD.org>, June 26, 1998.
 * Public domain.
 */

#ifndef _PW_PRIVATE_H_
#define _PW_PRIVATE_H_

int		__pw_scan(char *, struct passwd *, int *);
#if defined(RUN_NDBM) && (RUN_NDBM == 0)
/* NDBM */
int 	_pw_start(DBM *, FILE *, int *, int *, int *);
int 	_pw_end(DBM *, FILE *, int *);
int	_pw_getkey(DBM *, datum *, struct passwd *, char *, size_t, int *, int *, int);
void 	_pw_setkey(datum *, char *, size_t);
int	_pw_scanfp(FILE *, struct passwd *, char *);
void	_pw_readfp(struct passwd *);
#else
/* DB */
int 	_pw_start(DB *, int *, int *);
int 	_pw_end(DB *, int *);
int	_pw_getkey(DB *, DBT *, struct passwd *, char *, size_t, int *, int);
void 	_pw_setkey(DBT *, char *, size_t);
#endif
#endif /* _PW_PRIVATE_H_ */
