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

#ifndef _NETTPI_TPI_TPDU_VARIABLE_H_
#define _NETTPI_TPI_TPDU_VARIABLE_H_

typedef unsigned int SeqNum;

/* private tpdu variable structures */

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
	unsigned char fd_code; 			/* parameter code: no codes use bits 7 and 8  */
	unsigned char fd_length;		/* length indication (in octects): limit: 247, max: 255 */
	unsigned char fd_value;			/* variable value */
};

struct tpduv_crcc {
	unsigned int  crcc_caller_id;	/* TSAP_ID of caller */
	unsigned int  crcc_calling_id;	/* TSAP_ID of calling */
	unsigned char crcc_size;		/* See tpdu_sizes above (RFC default: 128) */
	unsigned char crcc_version; 	/* version number */
	unsigned int  crcc_security;
	unsigned char crcc_opt_select;
	SeqNum 		  crcc_alt_class;
	unsigned short crcc_ack_time;
	unsigned short crcc_priority;
	unsigned short crcc_assignment_time; /* the TTR value in seconds */
};

/* public tpdu variable structures */

struct tpduv_cr {
	struct tpduv_crcc 	cr_tpduv;
	struct cksum16 		cr_cksum16;
	struct throughput12 cr_throughput12;
	struct throughput24 cr_throughput24;
	struct error_rate3	cr_erate3;
	struct error_rate12 cr_erate12;
	struct transit_delay cr_trans_delay;
#define cr_caller_id			cr_tpduv.crcc_caller_id
#define cr_calling_id			cr_tpduv.crcc_calling_id
#define cr_size					cr_tpduv.crcc_size
#define cr_version				cr_tpduv.crcc_version
#define cr_security				cr_tpduv.crcc_security
#define cr_opt_select			cr_tpduv.crcc_opt_select
#define cr_alt_class			cr_tpduv.crcc_alt_class
#define cr_ack_time				cr_tpduv.crcc_ack_time
#define cr_priority  			cr_tpduv.crcc_priority
#define cr_ttr					cr_tpduv.crcc_assignment_time
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

struct tpduv_cc {
	struct tpduv_crcc 	cc_tpduv;
	struct throughput12 cc_throughput12;
	struct throughput24 cc_throughput24;
	struct error_rate3	cc_erate3;
	struct error_rate12 cc_erate12;
	struct transit_delay cc_trans_delay;
#define cc_caller_id			cc_tpduv.crcc_caller_id
#define cc_calling_id			cc_tpduv.crcc_calling_id
#define cc_size					cc_tpduv.crcc_size
#define cc_version				cc_tpduv.crcc_version
#define cc_security				cc_tpduv.crcc_security
#define cc_ack_time				cc_tpduv.crcc_ack_time
#define cc_priority  			cc_tpduv.crcc_priority
#define cc_ttr					cc_tpduv.crcc_assignment_time
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

struct tpduv_dr {
	unsigned char 		dr_add_info:31;
	struct cksum16 		dr_cksum16;
#define dr_cksum  		dr_cksum16.ck_cksum
};

struct tpduv_dc {
	struct cksum16 		dc_cksum16;
#define dc_cksum  		dc_cksum16.ck_cksum
};

struct tpduv_dt {
	struct cksum16 		dt_cksum16;
#define dt_cksum  		dt_cksum16.ck_cksum
};

struct tpduv_ak {
	struct cksum16 		ak_cksum16;
	struct flow_control ak_flow;
#define ak_cksum  		ak_cksum16.ck_cksum
#define	ak_winedge0 	ak_flow.fc_window_edge0
#define ak_winedge		ak_flow.fc_window_edge
#define ak_subseq		ak_flow.fc_subseq
#define	ak_cdt			ak_flow.fc_cdt
};

struct tpduv_xak {
	struct cksum16 		xak_cksum16;
#define xak_cksum  		xak_cksum16.ck_cksum
};

struct tpduv_er {
	unsigned char 		er_invalid:31;	/* invalid TPDU */
	struct cksum16 		er_cksum16;
#define er_cksum  		er_cksum16.ck_cksum
};

union tpdu_variable_rest {
	struct tpduv_cr 	_tpduvr_cr;

	struct tpduv_cc		_tpduvr_cc;

	struct tpduv_dr		_tpduvr_dr;

	struct tpduv_dc		_tpduvr_dc;

	struct tpduv_dt		_tpduvr_dt;

	struct tpduv_ak		_tpduvr_ak;

	struct tpduv_xak	_tpduvr_xak;

	struct tpduv_er		_tpduvr_er;
};

struct tpdu {
	struct tpdu_fixed   		_tpduf;
	union tpdu_fixed_rest   	_tpduvf;
	struct tpdu_variable   		_tpduv;
	union tpdu_variable_rest   	_tpduvr;
};
#endif /* _NETTPI_TPI_TPDU_VARIABLE_H_ */
