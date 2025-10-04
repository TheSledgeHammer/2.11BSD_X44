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

/*
 * ARGO Project, Computer Sciences Dept., University of Wisconsin - Madison
 */
/*
 * This ghastly set of macros makes it possible to refer to tpdu structures
 * without going mad.
 */

#ifndef _NETTPI_TPI_TPDU_H_
#define _NETTPI_TPI_TPDU_H_

/* private tpdu structures */

/*
 * This is used when the extended format seqence numbers are
 * being sent and received.
 */
/*
 * the seqeot field is an int that overlays the seq
 * and eot fields, this allows the htonl operation
 * to be applied to the entire 32 bit quantity, and
 * simplifies the structure definitions.
 */
union seq_type {
    struct {
        unsigned int    st_eot:1;   	/* end-of-tsdu */
        unsigned int    st_seq:31;  	/* 31 bit sequence number */
    } st;
    unsigned int        s_seqeot;
#define s_eot	        st.st_eot
#define s_seq	        st.st_seq
};

struct seqeot7 {
    unsigned char 		eot:1;			/* end-of-tsdu */
    unsigned char 		seq:7;			/* 7 bit sequence number */
};

struct seqeot31 {
    unsigned int 		Xeot:1;			/* end-of-tsdu */
    unsigned int 		Xseq:31;		/* 31 bit sequence number */
};

/*
 * This much of a tpdu is the same for all types of tpdus  (except DT tpdus
 * in class 0; their exceptions are handled by the data structure below
 */
struct tpdu_fixed {
    unsigned char       fd_li:8;      	/* length indicator */
    unsigned char       fd_cdt:4;     	/* credit */
    unsigned char       fd_type:4;    	/* type of tpdu (DT, CR, etc.) */
    unsigned short      fd_dref;      	/* destination ref; not in DT in class 0 */
};

struct tpdu_crcc {
    unsigned short      crcc_sref;       /* source reference */
    unsigned short      crcc_opt:4;
    unsigned short      crcc_class:4;
    unsigned short		crcc_xx:8;		/* unused */
};

struct tpdu_ak31 {
    unsigned int        ak31_yrseq0:1;	/* always zero */
    unsigned int        ak31_yrseq:31;	/* [ ISO 8073 13.9.3.d ] */
    unsigned short      ak31_cdt;		/* [ ISO 8073 13.9.3.b ] */
};


/* public tpdu structures */
struct tp0du {
	unsigned char       tp0_li;			/* same as in tpdu_fixed */
	unsigned char       tp0_cdt_type;	/* same as in tpdu_fixed */
	unsigned char		tp0_eot:1;		/* eot */
	unsigned char		tp0_mbz:7;		/* must be zero */
	unsigned char		tp0_notused:8;	/* data begins on this octet */
};

#define tp0du_eot 		tp0_eot
#define tp0du_mbz 		tp0_mbz

struct tpdu_cr {
    struct tpdu_fixed   cr_tpduf;
    struct tpdu_crcc    cr_crcc;
#define cr_li           cr_tpduf.fd_li
#define cr_type         cr_tpduf.fd_type
#define cr_cdt          cr_tpduf.fd_cdt
#define cr_dref_0       cr_tpduf.fd_dref
#define cr_sref         cr_crcc.crcc_sref
#define cr_tpdu_sref    cr_crcc.crcc_sref
#define cr_class        cr_crcc.crcc_class
#define cr_options      cr_crcc.crcc_opt
};

struct tpdu_cc {
    struct tpdu_fixed   cc_tpduf;
    struct tpdu_crcc    cc_crcc;
#define cc_li           cc_tpduf.fd_li
#define cc_type         cc_tpduf.fd_type
#define cc_cdt          cc_tpduf.fd_cdt
#define cc_dref       	cc_tpduf.fd_dref
#define cc_sref         cc_crcc.crcc_sref
#define cc_class        cc_crcc.crcc_class
#define cc_options      cc_crcc.crcc_opt
};

struct tpdu_dr {
    struct tpdu_fixed   dr_tpduf;
    unsigned short      dr_sref;
    unsigned char       dr_reason;
#define dr_li           dr_tpduf.fd_li
#define dr_type         dr_tpduf.fd_type
#define dr_dref         dr_tpduf.fd_dref
};

struct tpdu_dc {
    struct tpdu_fixed   dc_tpduf;
    unsigned short      dc_sref;
#define dc_li           dc_tpduf.fd_li
#define dc_type         dc_tpduf.fd_type
#define dc_dref         dc_tpduf.fd_dref
};

struct tpdu_dt {
    struct tpdu_fixed   dt_tpduf;
    struct seqeot7      dt_seq7;
    struct seqeot31     dt_seq31;
#define dt_li           dt_tpduf.fd_li
#define dt_type         dt_tpduf.fd_type
#define dt_dref         dt_tpduf.fd_dref
#define dt_seq          dt_seq7.seq
#define dt_eot          dt_seq7.eot
#define dt_seqX         dt_seq31.Xseq
#define dt_eotX         dt_seq31.Xeot
};

struct tpdu_xpd {
    struct tpdu_fixed   xpd_tpduf;
    struct seqeot7      xpd_seq7;
    struct seqeot31     xpd_seq31;
#define xpd_li          xpd_tpduf.fd_li
#define xpd_type        xpd_tpduf.fd_type
#define xpd_dref        xpd_tpduf.fd_dref
#define xpd_seq         xpd_seq7.seq
#define xpd_eot         xpd_seq7.eot
#define xpd_seqX        xpd_seq31.Xseq
#define xpd_eotX        xpd_seq31.Xeot
};

struct tpdu_ak {
    struct tpdu_fixed   ak_tpduf;
    struct tpdu_ak31    ak_ak31;
    struct seqeot7      ak_seq7;
    struct seqeot31     ak_seq31;
#define ak_li           ak_tpduf.fd_li
#define ak_type         ak_tpduf.fd_type
#define ak_dref         ak_tpduf.fd_dref
#define ak_seq          ak_seq7.seq
#define ak_seqX         ak_ak31.ak31_yrseq
	/* location of cdt depends on size of seq. numbers */
#define ak_cdt          ak_tpduf.fd_cdt
#define ak_cdtX         ak_ak31.ak31_cdt
};

struct tpdu_xak {
    struct tpdu_fixed   xak_tpduf;
    struct seqeot7      xak_seq7;
    struct seqeot31     xak_seq31;
#define xak_li          xak_tpduf.fd_li
#define xak_type        xak_tpduf.fd_type
#define xak_dref        xak_tpduf.fd_dref
#define xak_seq         xak_seq7.seq
#define xak_seqX        xak_seq31.seq
};

struct tpdu_er {
    struct tpdu_fixed   er_tpduf;
    unsigned char       er_reason;			/* [ ISO 8073 13.12.3.c ] */
#define er_li           er_tpduf.fd_li
#define er_type         er_tpduf.fd_type
#define er_dref         er_tpduf.fd_dref
};

/* Compatability with tp_tpdu.h */

/*
 * Then most tpdu types have a portion that is always present but differs
 * among the tpdu types :
 */
union tpdu_fixed_rest {
    struct tpdu_cr      _tpdufr_cr;
#define tpdu_CRli       _tpdufr_cr.cr_li
#define tpdu_CRtype     _tpdufr_cr.cr_type
#define tpdu_CRcdt      _tpdufr_cr.cr_cdt
#define tpdu_CRdref_0   _tpdufr_cr.cr_dref_0
#define tpdu_CRsref     _tpdufr_cr.cr_sref
#define tpdu_sref       _tpdufr_cr.cr_tpdu_sref
#define tpdu_CRclass    _tpdufr_cr.cr_class
#define tpdu_CRoptions  _tpdufr_cr.cr_opt

    struct tpdu_cc      _tpdufr_cc;
#define tpdu_CCli       _tpdufr_cc.cc_li
#define tpdu_CCtype     _tpdufr_cc.cc_type
#define tpdu_CCcdt      _tpdufr_cc.cc_cdt
#define tpdu_CCdref     _tpdufr_cc.cc_dref
#define tpdu_CCsref     _tpdufr_cc.cc_sref
#define tpdu_CCclass    _tpdufr_cc.cc_class
#define tpdu_CCoptions  _tpdufr_cc.cc_opt

    struct tpdu_dr      _tpdufr_dr;
#define tpdu_DRli       _tpdufr_dr.dr_li
#define tpdu_DRtype     _tpdufr_dr.dr_type
#define tpdu_DRdref     _tpdufr_dr.dr_dref
#define tpdu_DRsref     _tpdufr_dr.dr_sref
#define tpdu_DRreason   _tpdufr_dr.dr_reason

    struct tpdu_dc      _tpdufr_dc;
#define tpdu_DCli       _tpdufr_dc.dc_li
#define tpdu_DCtype     _tpdufr_dc.dc_type
#define tpdu_DCdref     _tpdufr_dc.dc_dref
#define tpdu_DCsref     _tpdufr_dc.dc_sref

    unsigned int        _tpdufr_Xseqeot;
#define tpdu_seqeotX    _tpdufr._tpdufr_Xseqeot

    struct tpdu_dt      _tpdufr_dt;
#define tpdu_DTli       _tpdufr_dt.dt_li
#define tpdu_DTtype     _tpdufr_dt.dt_type
#define tpdu_DTdref     _tpdufr_dt.dt_dref
#define tpdu_DTseq      _tpdufr_dt.dt_seq
#define tpdu_DTeot      _tpdufr_dt.dt_eot
#define tpdu_DTseqX     _tpdufr_dt.dt_Xseq
#define tpdu_DTeotX     _tpdufr_dt.dt_Xeot

    struct tpdu_xpd     _tpdufr_xpd;
#define tpdu_XPDli      _tpdufr_xpd.xpd_li
#define tpdu_XPDtype    _tpdufr_xpd.xpd_type
#define tpdu_XPDdref    _tpdufr_xpd.xpd_dref
#define tpdu_XPDseq     _tpdufr_xpd.xpd_seq
#define tpdu_XPDeot     _tpdufr_xpd.xpd_eot
#define tpdu_XPDseqX    _tpdufr_xpd.xpd_Xseq
#define tpdu_XPDeotX    _tpdufr_xpd.xpd_Xeot

    struct tpdu_ak      _tpdufr_ak;
#define tpdu_AKli       _tpdufr_ak.ak_li
#define tpdu_AKtype     _tpdufr_ak.ak_type
#define tpdu_AKdref     _tpdufr_ak.ak_dref
#define tpdu_AKseq      _tpdufr_ak.ak_seq
#define tpdu_AKseqX     _tpdufr_ak.ak_yrseq
	/* location of cdt depends on size of seq. numbers */
#define tpdu_AKcdt      _tpdufr_ak.ak_cdt
#define tpdu_AKcdtX     _tpdufr_ak.ak_cdtX

    struct tpdu_xak     _tpdufr_xak;
#define tpdu_XAKli      _tpdufr_xak.xak_li
#define tpdu_XAKtype    _tpdufr_xak.xak_type
#define tpdu_XAKdref    _tpdufr_xak.xak_dref
#define tpdu_XAKseq     _tpdufr_xak.xak_seq
#define tpdu_XAKseqX    _tpdufr_xak.xak_Xseq

    struct tpdu_er      _tpdufr_er;
#define tpdu_ERli       _tpdufr_er.er_li
#define tpdu_ERtype     _tpdufr_er.er_type
#define tpdu_ERdref     _tpdufr_er.er_dref
#define tpdu_ERreason   _tpdufr_er.er_reason
};

/* OPTIONS and ADDL OPTIONS bits */
#define TPO_USE_EFC	 			0x1
#define TPO_XTD_FMT	 			0x2
#define TPAO_USE_TXPD 			0x1
#define TPAO_NO_CSUM 			0x2
#define TPAO_USE_RCC 			0x4
#define TPAO_USE_NXPD 			0x8

struct tpdu {
	struct tpdu_fixed       _tpduf;
	union tpdu_fixed_rest   _tpdufr;
};

#endif /* _NETTPI_TPI_TPDU_H_ */
