/*
 * The 3-Clause BSD License:
 * Copyright (c) 2026 Martin Kelly
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Below code is all referenced from,
 * RFC905: "ISO Transport Protocol Specification ISO DP 8073"
 */

/*
 * Notes:
 * - Some of these are already present within netiso's TP implementation.
 * - Includes all possible tpdu variables, with the notion of modularizing it
 * 		to tailor to different needs.
 */

/*
 * TPDU Variable Part
 */

#ifndef _NETISO_TPDU_V_H_
#define _NETISO_TPDU_V_H_

/* private tpdu structures */

/* sizes in octets */
unsigned char tpdu_sizes = {
		8192, /*  (not allowed in Class 0) */
		4097, /*  (not allowed in Class 0) */
		2048,
		1024,
		512,
		256,
		128
};

struct cksum16 {
	unsigned short ck_cksum; 		/* checksum (Uses Fletcher16) */
};

struct error_rate3 { 				/* 3 octets in length */
	unsigned char er_val;  			/* target value, power of 10 */
	unsigned char er_min; 			/* min accetable, power of 10 */
	unsigned char er_size;			/* TSDU size, power of 2 */
};

struct error_rate12 { 				/* 12 octets in length */
	unsigned int er_caller_val;		/* caller value */
	unsigned int er_caller_min;		/* caller min */
	unsigned int er_calling_min;	/* calling value */
	unsigned int er_calling_val;	/* calling min */
};

struct throughput12 { 				/* 12 octets in length */
	unsigned int tp_caller_val;		/* caller value */
	unsigned int tp_caller_min;		/* caller min */
	unsigned int tp_calling_min;	/* calling value */
	unsigned int tp_calling_val;	/* calling min */
};

struct throughput24 { 				/* 24 octets in length */
	unsigned long tp_max;			/* max throughput */
	unsigned long tp_avg;			/* average throughput */
};

struct transit_delay {				/* 8 octets in length */
	unsigned short td_caller_val;	/* caller value */
	unsigned short td_caller_min;	/* caller min */
	unsigned short td_calling_val;	/* calling value */
	unsigned short td_calling_min;	/* calling min */
};

/* these not correct */
struct flow_control {				/* 8 octets in length */
	unsigned short fc_window_edge;	/* lower window edge (16-bits) */
	unsigned short fc_window_edge0:8;	/* lower window edge: Bit 8 of Octet 4 set to zero */
	unsigned short fc_subseq;		/* sub-sequence of received AK TPDU (16-bits) */
	unsigned short fc_cdt;			/* cdt field of received AK TPDU (16-bits) */
};

/*
 * Maximum number of parameters that may be contained within
 * the variable portion is:
 * tpdu length indicator - tpdu fixed part.
 */
#define TPDUV_NUM_MAX(li) ((li) - sizeof(struct tpdu_fixed))

struct tpdu_variable {
	unsigned char fd_pcode;			/* parameter code: no codes use bits 7 and 8  */
	unsigned char fd_pli;			/* parameter length indicator (in octects): limit: 247, max: 255 */
	unsigned char fd_pvalue;		/* parameter variable value */
};

struct tpduv_crcc {
	unsigned int  crcc_caller_id;	/* TSAP_ID of caller */
	unsigned int  crcc_calling_id;	/* TSAP_ID of calling */
	unsigned char crcc_size;		/* See tpdu_sizes above (RFC default: 128) */
	unsigned char crcc_version; 	/* version number */
	unsigned int  crcc_security;
	unsigned char crcc_opt_select;	/* options select */
	unsigned int  crcc_alt_class;	/* alternate class */
	unsigned short crcc_ack_time;	/* acknowledgment time */
	unsigned short crcc_priority;	/* priority */
	unsigned short crcc_assignment_time; /* the TTR value in seconds */
};

/* public tpdu variable structures */

/* TPDU connect request */
struct tpduv_cr {
	struct tpdu_variable 		cr_tpduv;
	struct tpduv_crcc 			cr_crcc;
	struct cksum16 				cr_cksum16;
	struct throughput12 		cr_throughput12;
	struct throughput24 		cr_throughput24;
	struct error_rate3			cr_erate3;
	struct error_rate12 		cr_erate12;
	struct transit_delay 		cr_trans_delay;
#define cr_pcode				cr_tpduv.fd_pcode
#define cr_pli					cr_tpduv.fd_pli
#define cr_pvalue				cr_tpduv.fd_pvalue
#define cr_caller_id			cr_crcc.crcc_caller_id
#define cr_calling_id			cr_crcc.crcc_calling_id
#define cr_size					cr_crcc.crcc_size
#define cr_version				cr_crcc.crcc_version
#define cr_security				cr_crcc.crcc_security
#define cr_opt_select			cr_crcc.crcc_opt_select
#define cr_alt_class			cr_crcc.crcc_alt_class
#define cr_ack_time				cr_crcc.crcc_ack_time
#define cr_priority  			cr_crcc.crcc_priority
#define cr_ttr					cr_crcc.crcc_assignment_time
#define cr_cksum  				cr_cksum16.ck_cksum
#define cr_tput_caller_val		cr_throughput12.tp_caller_val
#define cr_tput_caller_min		cr_throughput12.tp_caller_min
#define cr_tput_calling_val		cr_throughput12.tp_calling_val
#define cr_tput_calling_min		cr_throughput12.tp_calling_min
#define cr_tput_max				cr_throughput24.tp_max
#define cr_tput_avg				cr_throughput24.tp_avg
#define cr_erate_val			cr_erate3.er_val
#define cr_erate_min			cr_erate3.er_min
#define cr_erate_size			cr_erate3.er_size
#define cr_erate_caller_val		cr_erate12.er_caller_val
#define cr_erate_caller_min		cr_erate12.er_caller_min
#define cr_erate_calling_val	cr_erate12.er_calling_val
#define cr_erate_calling_min	cr_erate12.er_calling_min
#define cr_tdelay_caller_val	cr_trans_delay.td_caller_val
#define cr_tdelay_caller_min	cr_trans_delay.td_caller_min
#define cr_tdelay_calling_val	cr_trans_delay.td_calling_val
#define cr_tdelay_calling_min	cr_trans_delay.td_calling_min
};

/* TPDU connect confirm */
struct tpduv_cc {
	struct tpdu_variable	 	cc_tpduv;
	struct tpduv_crcc 			cc_crcc;
	struct throughput12 		cc_throughput12;
	struct throughput24 		cc_throughput24;
	struct error_rate3			cc_erate3;
	struct error_rate12 		cc_erate12;
	struct transit_delay 		cc_trans_delay;
#define cc_pcode				cc_tpduv.fd_pcode
#define cc_pli					cc_tpduv.fd_pli
#define cc_pvalue				cc_tpduv.fd_pvalue
#define cc_caller_id			cc_crcc.crcc_caller_id
#define cc_calling_id			cc_crcc.crcc_calling_id
#define cc_size					cc_crcc.crcc_size
#define cc_version				cc_crcc.crcc_version
#define cc_security				cc_crcc.crcc_security
#define cc_ack_time				cc_crcc.crcc_ack_time
#define cc_priority  			cc_crcc.crcc_priority
#define cc_ttr					cc_crcc.crcc_assignment_time
#define cc_cksum  				cc_cksum16.ck_cksum
#define cc_tput_caller_val		cc_throughput12.tp_caller_val
#define cc_tput_caller_min		cc_throughput12.tp_caller_min
#define cc_tput_calling_val		cc_throughput12.tp_calling_val
#define cc_tput_calling_min		cc_throughput12.tp_calling_min
#define cc_tput_max				cc_throughput24.tp_max
#define cc_tput_avg				cc_throughput24.tp_avg
#define cc_erate_val			cc_erate3.er_val
#define cc_erate_min			cc_erate3.er_min
#define cc_erate_size			cc_erate3.er_size
#define cc_erate_caller_val		cc_erate12.er_caller_val
#define cc_erate_caller_min		cc_erate12.er_caller_min
#define cc_erate_calling_val	cc_erate12.er_calling_val
#define cc_erate_calling_min	cc_erate12.er_calling_min
#define cc_tdelay_caller_val	cc_trans_delay.td_caller_val
#define cc_tdelay_caller_min	cc_trans_delay.td_caller_min
#define cc_tdelay_calling_val	cc_trans_delay.td_calling_val
#define cc_tdelay_calling_min	cc_trans_delay.td_calling_min
};

/* TPDU disconnect request */
struct tpduv_dr {
	struct tpdu_variable dr_tpduv;
	unsigned char 		dr_add_info:31;
	struct cksum16 		dr_cksum16;
#define dr_pcode		dr_tpduv.fd_pcode
#define dr_pli			dr_tpduv.fd_pli
#define dr_pvalue		dr_tpduv.fd_pvalue
#define dr_cksum  		dr_cksum16.ck_cksum
};

/* TPDU disconnect confirm */
struct tpduv_dc {
	struct tpdu_variable dc_tpduv;
	struct cksum16 		dc_cksum16;
#define dc_pcode		dc_tpduv.fd_pcode
#define dc_pli			dc_tpduv.fd_pli
#define dc_pvalue		dc_tpduv.fd_pvalue
#define dc_cksum  		dc_cksum16.ck_cksum
};

/* TPDU data */
struct tpduv_dt {
	struct tpdu_variable dt_tpduv;
	struct cksum16 		dt_cksum16;
#define dt_pcode		dt_tpduv.fd_pcode
#define dt_pli			dt_tpduv.fd_pli
#define dt_pvalue		dt_tpduv.fd_pvalue
#define dt_cksum  		dt_cksum16.ck_cksum
};

/* TPDU expedited data */
struct tpduv_xpd {
	struct tpdu_variable xpd_tpduv;
	struct cksum16 		xpd_cksum16;
#define xpd_pcode		xpd_tpduv.fd_pcode
#define xpd_pli			xpd_tpduv.fd_pli
#define xpd_pvalue		xpd_tpduv.fd_pvalue
#define xpd_cksum  		xpd_cksum16.ck_cksum
};

/* TPDU data acknowledge */
struct tpduv_ak {
	struct tpdu_variable ak_tpduv;
	struct cksum16 		ak_cksum16;
	struct flow_control ak_flow;
#define ak_pcode		ak_tpduv.fd_pcode
#define ak_pli			ak_tpduv.fd_pli
#define ak_pvalue		ak_tpduv.fd_pvalue
#define ak_cksum  		ak_cksum16.ck_cksum
#define	ak_winedge0 	ak_flow.fc_window_edge0
#define ak_winedge		ak_flow.fc_window_edge
#define ak_subseq		ak_flow.fc_subseq
#define	ak_cdt			ak_flow.fc_cdt
};

/* TPDU expedited data acknowledge */
struct tpduv_xak {
	struct tpdu_variable xak_tpduv;
	struct cksum16 		xak_cksum16;
#define xak_pcode		xak_tpduv.fd_pcode
#define xak_pli			xak_tpduv.fd_pli
#define xak_pvalue		xak_tpduv.fd_pvalue
#define xak_cksum  		xak_cksum16.ck_cksum
};

/* TPDU error */
struct tpduv_er {
	struct tpdu_variable er_tpduv;
	unsigned char 		er_invalid:31;	/* invalid TPDU */
	struct cksum16 		er_cksum16;
#define er_pcode		er_tpduv.fd_pcode
#define er_pli			er_tpduv.fd_pli
#define er_pvalue		er_tpduv.fd_pvalue
#define er_cksum  		er_cksum16.ck_cksum
};

union tpdu_variable_rest {
	struct tpduv_cr 				_tpduvr_cr;
#define tpdu_CRpcode 				_tpduvr_cr.cr_pcode
#define tpdu_CRpli 					_tpduvr_cr.cr_pli
#define tpdu_CRpvalue 				_tpduvr_cr.cr_pvalue
#define tpdu_CRcaller_id 			_tpduvr_cr.cr_caller_id
#define tpdu_CRcalling_id 			_tpduvr_cr.cr_calling_id
#define tpdu_CRsize 				_tpduvr_cr.cr_size
#define tpdu_CRversion 				_tpduvr_cr.cr_version
#define tpdu_CRsecurity 			_tpduvr_cr.cr_security
#define tpdu_CRopt_select 			_tpduvr_cr.cr_opt_select
#define tpdu_CRalt_class 			_tpduvr_cr.cr_alt_class
#define tpdu_CRack_time 			_tpduvr_cr.cr_ack_time
#define tpdu_CRpriority 			_tpduvr_cr.cr_priority
#define tpdu_CRttr 					_tpduvr_cr.cr_ttr
#define tpdu_CRcksum 				_tpduvr_cr.cr_cksum
#define tpdu_CRtput_caller_val 		_tpduvr_cr.cr_tput_caller_val
#define tpdu_CRtput_caller_min 		_tpduvr_cr.cr_tput_caller_min
#define tpdu_CRtput_calling_val 	_tpduvr_cr.cr_tput_calling_val
#define tpdu_CRtput_calling_min 	_tpduvr_cr.cr_tput_calling_min
#define tpdu_CRtput_max 			_tpduvr_cr.cr_tput_max
#define tpdu_CRtput_avg 			_tpduvr_cr.cr_tput_avg
#define tpdu_CRerate_val 			_tpduvr_cr.cr_erate_val
#define tpdu_CRerate_min 			_tpduvr_cr.cr_erate_min
#define tpdu_CRerate_size 			_tpduvr_cr.cr_erate_size
#define tpdu_CRerate_caller_val 	_tpduvr_cr.cr_erate_caller_val
#define tpdu_CRerate_caller_min 	_tpduvr_cr.cr_erate_caller_min
#define tpdu_CRerate_calling_val 	_tpduvr_cr.cr_erate_calling_val
#define tpdu_CRerate_calling_min 	_tpduvr_cr.cr_erate_calling_min
#define tpdu_CRtdelay_caller_val 	_tpduvr_cr.cr_tdelay_caller_val
#define tpdu_CRtdelay_caller_min 	_tpduvr_cr.cr_tdelay_caller_min
#define tpdu_CRtdelay_calling_val 	_tpduvr_cr.cr_tdelay_calling_val
#define tpdu_CRtdelay_calling_min 	_tpduvr_cr.cr_tdelay_calling_min

	struct tpduv_cc					_tpduvr_cc;
#define tpdu_CCpcode 				_tpduvr_cc.cc_pcode
#define tpdu_CCpli 					_tpduvr_cc.cc_pli
#define tpdu_CCpvalue 				_tpduvr_cc.cc_pvalue
#define tpdu_CCcaller_id 			_tpduvr_cc.cc_caller_id
#define tpdu_CCcalling_id 			_tpduvr_cc.cc_calling_id
#define tpdu_CCsize 				_tpduvr_cc.cc_size
#define tpdu_CCversion	 			_tpduvr_cc.cc_version
#define tpdu_CCsecurity 			_tpduvr_cc.cc_security
#define tpdu_CCack_time 			_tpduvr_cc.cc_ack_time
#define tpdu_CCpriority 			_tpduvr_cc.cc_priority
#define tpdu_CCttr 					_tpduvr_cc.cc_ttr
#define tpdu_CCcksum 				_tpduvr_cc.cc_cksum
#define tpdu_CCtput_caller_val 		_tpduvr_cc.cc_tput_caller_val
#define tpdu_CCtput_caller_min 		_tpduvr_cc.cc_tput_caller_min
#define tpdu_CCtput_calling_val 	_tpduvr_cc.cc_tput_calling_val
#define tpdu_CCtput_calling_min 	_tpduvr_cc.cc_tput_calling_min
#define tpdu_CCtput_max 			_tpduvr_cc.cc_tput_max
#define tpdu_CCtput_avg 			_tpduvr_cc.cc_tput_avg
#define tpdu_CCerate_val 			_tpduvr_cc.cc_erate_val
#define tpdu_CCerate_min 			_tpduvr_cc.cc_erate_min
#define tpdu_CCerate_size 			_tpduvr_cc.cc_erate_size
#define tpdu_CCerate_caller_val 	_tpduvr_cc.cc_erate_caller_val
#define tpdu_CCerate_caller_min 	_tpduvr_cc.cc_erate_caller_min
#define tpdu_CCerate_calling_val 	_tpduvr_cc.cc_erate_calling_val
#define tpdu_CCerate_calling_min 	_tpduvr_cc.cc_erate_calling_min
#define tpdu_CCtdelay_caller_val 	_tpduvr_cc.cc_tdelay_caller_val
#define tpdu_CCtdelay_caller_min 	_tpduvr_cc.cc_tdelay_caller_min
#define tpdu_CCtdelay_calling_val 	_tpduvr_cc.cc_tdelay_calling_val
#define tpdu_CCtdelay_calling_min 	_tpduvr_cc.cc_tdelay_calling_min

	struct tpduv_dr					_tpduvr_dr;
#define tpdu_DRpcode 				_tpduvr_dr.dr_pcode
#define tpdu_DRpli 					_tpduvr_dr.dr_pli
#define tpdu_DRpvalue 				_tpduvr_dr.dr_pvalue
#define tpdu_DRadd_info				_tpduvr_dr.dr_add_info
#define tpdu_DRcksum 				_tpduvr_dr.dr_cksum

	struct tpduv_dc					_tpduvr_dc;
#define tpdu_DCpcode 				_tpduvr_dc.dc_pcode
#define tpdu_DCpli 					_tpduvr_dc.dc_pli
#define tpdu_DCpvalue 				_tpduvr_dc.dc_pvalue
#define tpdu_DCcksum 				_tpduvr_dc.dc_cksum

	struct tpduv_dt					_tpduvr_dt;
#define tpdu_DTpcode 				_tpduvr_dt.dt_pcode
#define tpdu_DTpli 					_tpduvr_dt.dt_pli
#define tpdu_DTpvalue 				_tpduvr_dt.dt_pvalue
#define tpdu_DTcksum 				_tpduvr_dt.dt_cksum

	struct tpduv_xpd				_tpduvr_xpd;
#define tpdu_XPDpcode 				_tpduvr_xpd.pcode
#define tpdu_XPDpli 				_tpduvr_xpd.pli
#define tpdu_XPDpvalue 				_tpduvr_xpd.pvalue
#define tpdu_XPDcksum 				_tpduvr_xpd.cksum

	struct tpduv_ak					_tpduvr_ak;
#define tpdu_AKpcode 				_tpduvr_ak.ak_pcode
#define tpdu_AKpli 					_tpduvr_ak.ak_pli
#define tpdu_AKpvalue 				_tpduvr_ak.ak_pvalue
#define tpdu_AKcksum 				_tpduvr_ak.ak_cksum
#define tpdu_AKwinedge0 			_tpduvr_ak.ak_winedge0
#define tpdu_AKwinedge 				_tpduvr_ak.ak_winedge
#define tpdu_AKsubseq 				_tpduvr_ak.ak_subseq
#define tpdu_AKcdt 					_tpduvr_ak.ak_cdt

	struct tpduv_xak				_tpduvr_xak;
#define tpdu_XAKpcode 				_tpduvr_xak.xak_pcode
#define tpdu_XAKpli 				_tpduvr_xak.xak_pli
#define tpdu_XAKpvalue 				_tpduvr_xak.xak_pvalue
#define tpdu_XAKcksum 				_tpduvr_xak.xak_cksum

	struct tpduv_er					_tpduvr_er;
#define tpdu_ERpcode 				_tpduvr_er.er_pcode
#define tpdu_ERpli 					_tpduvr_er.er_pli
#define tpdu_ERpvalue 				_tpduvr_er.er_pvalue
#define tpdu_ERinvalid				_tpduvr_er.er_invalid
#define tpdu_ERcksum 				_tpduvr_er.er_ckum
};

#endif /* _NETISO_TPDU_V_H_ */
