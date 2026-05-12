/*	$NetBSD: tp_tpdu.h,v 1.11 2005/12/11 00:01:36 elad Exp $	*/

/*-
 * Copyright (c) 1991, 1993
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
 *	@(#)tp_tpdu.h	8.1 (Berkeley) 6/10/93
 */

/***********************************************************
		Copyright IBM Corporation 1987

                      All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of IBM not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

IBM DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
IBM BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

#ifndef _NETISO_TPDU_H_
#define _NETISO_TPDU_H_

#include <nettpi/tp_tpdu_f.h> /* needed for tpdu fixed part */
#include <nettpi/tp_tpdu_v.h> /* needed for tpdu variable part */

/* OPTIONS and ADDL OPTIONS bits */
#define TPO_USE_EFC		0x1
#define TPO_XTD_FMT		0x2
#define TPAO_USE_TXPD 	0x1
#define TPAO_NO_CSUM 	0x2
#define TPAO_USE_RCC 	0x4
#define TPAO_USE_NXPD 	0x8

struct tpdu {
	struct tpdu_fixed   		_tpduf;
	union tpdu_fixed_rest   	_tpdufr;
	struct tpdu_variable   		_tpduv;
	union tpdu_variable_rest   	_tpduvr;
};

/*
 * tpdu info
 * Derived from tpdu_info[][4] in tp_input.c
 */
struct tpdu_info_head;
LIST_HEAD(tpdu_info_head, tpdu_info);
struct tpdu_info {
	unsigned char ti_rf_length;    	/* length: regular format */
	unsigned char ti_xf_length;    	/* length: extended format */
	unsigned char ti_type;         	/* tpdu type (DT, CR, etc.) */
	unsigned char ti_class;        	/* tpdu class (0, 4, etc.) */
    unsigned char ti_max_length;   	/* max length */
    struct tpdu *ti_tpdu;			/* pointer to tpdu */
    LIST_ENTRY(tpdu_info) ti_link; 	/* info list */
};

extern struct tpdu_info_head tp_info_list;

#endif /* _NETISO_TPDU_H_ */
