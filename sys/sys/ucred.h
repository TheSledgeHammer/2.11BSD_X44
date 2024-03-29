/*
 * Copyright (c) 1989, 1993
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
 *
 *	@(#)ucred.h	8.2 (Berkeley) 1/4/94
 * $Id: ucred.h,v 1.3 1994/08/18 22:35:51 wollman Exp $
 */

#ifndef _SYS_UCRED_H_
#define	_SYS_UCRED_H_

/*
 * Credentials.
 */
struct ucred {
	u_short		cr_ref;			/* reference count */
	uid_t		cr_uid;			/* effective user id */
	short		cr_ngroups;		/* number of groups */
	gid_t		cr_groups[NGROUPS];	/* groups */
};
#define cr_gid 		cr_groups[0]
#define NOCRED 		((struct ucred *)-1)	/* no credential available */
#define FSCRED 		((struct ucred *)-2)	/* filesystem credential */

/* for priv_check_cred() */
#define	NULL_CRED_OKAY	0x2

/*
 * Track beginning of privilege list.
 */
#define	_PRIV_LOWEST	1

/*
 * Kernel environment priveleges.
 */
#define	PRIV_KENV_SET	2	/* 120: Set kernel env. variables. */
#define	PRIV_KENV_UNSET	3	/* 121: Unset kernel env. variables. */

/*
 * Track end of privilege list.
 */
#define	_PRIV_HIGHEST	4 /* 682: FreeBSD's Highest  */

/*
 * Validate that a named privilege is known by the privilege system.  Invalid
 * privileges presented to the privilege system by a priv_check interface
 * will result in a panic.  This is only approximate due to sparse allocation
 * of the privilege space.
 */
#define	PRIV_VALID(x)	((x) > _PRIV_LOWEST && (x) < _PRIV_HIGHEST)

#ifdef _KERNEL
struct ucred 	*crget(void);
struct ucred 	*crcopy(struct ucred *);
struct ucred 	*crdup(struct ucred *);
struct ucred 	*crhold(struct ucred *);
extern void 	crfree(struct ucred *);
int 			suser();
int 			_suser(struct ucred *, short *);
int				groupmember(gid_t);
int				_groupmember(gid_t, struct ucred *);
int				priv_check(int);
int				priv_check_cred(struct ucred *, int, int);

/* 4.4BSD compat */
#define suser1(cred, acflag) 	\
	_suser(cred, acflag)

#define groupmember1(gid, cred) \
	_groupmember(gid, cred)
#endif /* KERNEL */
#endif /* !_SYS_UCRED_H_ */
