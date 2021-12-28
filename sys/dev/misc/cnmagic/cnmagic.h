/*	$NetBSD: systm.h,v 1.170 2004/01/23 05:01:19 simonb Exp $	*/
/*-
 * Copyright (c) 1982, 1988, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *
 *	@(#)systm.h	8.7 (Berkeley) 3/29/95
 */

#ifndef _DEV_CNMAGIC_H_
#define _DEV_CNMAGIC_H_

#include <dev/misc/cons/cons.h>

/*
 * Stuff to handle debugger magic key sequences.
 */
#define CNS_LEN				128
#define CNS_MAGIC_VAL(x)	((x)&0x1ff)
#define CNS_MAGIC_NEXT(x)	(((x)>>9)&0x7f)
#define CNS_TERM			0x7f	/* End of sequence */

typedef struct cnm_state {
	int		cnm_state;
	u_short	*cnm_magic;
} cnm_state_t;

/* Override db_console() in MD headers */
#define cn_trap() 				\
	do {						\
		cn_trapped = 1;			\
	} while (/* CONSTCOND */ 0)
#ifndef cn_isconsole
#define cn_isconsole(d)	(cn_tab != NULL && (d) == cn_tab->cn_dev)
#endif

void 	cn_init_magic(cnm_state_t *);
void	cn_destroy_magic(cnm_state_t *);
int 	cn_set_magic(const char *);
int 	cn_get_magic(char *, size_t);

/* This should be called for each byte read */
#ifndef cn_check_magic
#define cn_check_magic(d, k, s)						\
	do {											\
		if (cn_isconsole(d)) {						\
			int _v = (s).cnm_magic[(s).cnm_state];	\
			if ((k) == CNS_MAGIC_VAL(_v)) {			\
				(s).cnm_state = CNS_MAGIC_NEXT(_v);	\
				if ((s).cnm_state == CNS_TERM) {	\
					cn_trap();						\
					(s).cnm_state = 0;				\
				}									\
			} else {								\
				(s).cnm_state = 0;					\
			}										\
		}											\
	} while (/* CONSTCOND */ 0)
#endif

/* Encode out-of-band events this way when passing to cn_check_magic() */
#define	CNC_BREAK		0x100

#endif /* _DEV_CNMAGIC_H_ */
