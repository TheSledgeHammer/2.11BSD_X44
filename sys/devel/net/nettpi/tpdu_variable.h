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
 * Notes:
 * - Some of these are already present within netiso's TP implementation.
 */

/*
 * TODO:
 * - define error_rate and transmit_delay for CC and CR.
 * - fill out tpdu_variable_rest
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
	unsigned char cksum:2; 			/* checksum (Uses Fletcher16/32) */
};

struct error_rate3 { 				/* 3 octets in length */
	unsigned char val;  			/* target value, power of 10 */
	unsigned char min; 				/* min accetable, power of 10 */
	unsigned char size;				/* TSDU size, power of 2 */
};

struct error_rate12 { 				/* 12 octets in length */
	unsigned char caller_val:3;		/* caller value */
	unsigned char caller_min:3;		/* caller min */
	unsigned char calling_min:3;	/* calling value */
	unsigned char calling_val:3;	/* calling min */
};

struct throughput12 { 				/* 12 octets in length */
	unsigned char caller_val:3;		/* caller value */
	unsigned char caller_min:3;		/* caller min */
	unsigned char calling_min:3;	/* calling value */
	unsigned char calling_val:3;	/* calling min */
};

struct throughput24 { 				/* 24 octets in length */
	unsigned char max:12;			/* max throughput */
	unsigned char avg:12;			/* average throughput */
	unsigned char caller_val:3;
	unsigned char caller_min:3;
	unsigned char calling_min:3;
	unsigned char calling_val:3;
};

struct transit_delay {				/* 8 octets in length */
	unsigned char caller_val:2;		/* caller value */
	unsigned char caller_min:2;		/* caller min */
	unsigned char calling_min:2;	/* calling value */
	unsigned char calling_val:2;	/* calling min */
};

/* these not correct */
struct flow_control {				/* 8 octets in length */
	unsigned char window_edge0:1;	/* lower window edge: Bit 8 of Octet 4 set to zero */
	unsigned char window_edge:2;	/* lower window edge (16-bits) */
	unsigned char subseq:2;			/* sub-sequence of received AK TPDU (16-bits) */
	unsigned char cdt:4;			/* cdt field of received AK TPDU (16-bits) */
};

struct tpduv_crcc {
	unsigned int  crcc_caller_id;		/* TSAP_ID of caller */
	unsigned int  crcc_calling_id;		/* TSAP_ID of calling */
	unsigned char crcc_size;
	unsigned char crcc_version; 		/* version number */
	unsigned int  crcc_security;
	unsigned char crcc_opt_select;
	SeqNum 		  crcc_alt_class;
	unsigned char crcc_ack_time:2;
	unsigned char crcc_priority:2;
	unsigned char crcc_assignment_time:2; /* the TTR value in seconds */
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
#define cr_caller_id	cr_tpduv.crcc_caller_id
#define cr_calling_id	cr_tpduv.crcc_calling_id
#define cr_size			cr_tpduv.crcc_size
#define cr_version		cr_tpduv.crcc_version
#define cr_security		cr_tpduv.crcc_security
#define cr_opt_select	cr_tpduv.crcc_opt_select
#define cr_alt_class	cr_tpduv.crcc_alt_class
#define cr_ack_time		cr_tpduv.crcc_ack_time
#define cr_priority  	cr_tpduv.crcc_priority
#define cr_ttr			cr_tpduv.crcc_assignment_time
#define cr_cksum  		cr_cksum16.cksum
#define cr_caller_val	cr_throughput12.caller_val
#define cr_caller_min	cr_throughput12.caller_min
#define cr_calling_val	cr_throughput12.calling_val
#define cr_calling_min	cr_throughput12.calling_min
#define cr_tpmax		cr_throughput24.max
#define cr_tpavg		cr_throughput24.avg
};

struct tpduv_cc {
	struct tpduv_crcc 	cc_tpduv;
	struct throughput12 cc_throughput12;
	struct throughput24 cc_throughput24;
	struct error_rate3	cc_erate3;
	struct error_rate12 cc_erate12;
	struct transit_delay cc_trans_delay;
#define cc_caller_id	cc_tpduv.crcc_caller_id
#define cc_calling_id	cc_tpduv.crcc_calling_id
#define cc_size			cc_tpduv.crcc_size
#define cc_version		cc_tpduv.crcc_version
#define cc_security		cc_tpduv.crcc_security
#define cc_ack_time		cc_tpduv.crcc_ack_time
#define cc_priority  	cc_tpduv.crcc_priority
#define cc_ttr			cc_tpduv.crcc_assignment_time
#define cc_cksum  		cc_cksum16.cksum
#define cc_caller_val	cc_throughput12.caller_val
#define cc_caller_min	cc_throughput12.caller_min
#define cc_calling_val	cc_throughput12.calling_val
#define cc_calling_min	cc_throughput12.calling_min
#define cc_tpmax		cc_throughput24.max
#define cc_tpavg		cc_throughput24.avg

};

struct tpduv_dr {
	unsigned char 		dr_add_info:31;
	struct cksum16 		dr_cksum16;
#define dr_cksum  		dr_cksum16.cksum
};

struct tpduv_dc {
	struct cksum16 		dc_cksum16;
#define dc_cksum  		dc_cksum16.cksum
};

struct tpduv_dt {
	struct cksum16 		dt_cksum16;
#define dt_cksum  		dt_cksum16.cksum
};

struct tpduv_ak {
	struct cksum16 		ak_cksum16;
	struct flow_control ak_flow;
#define ak_cksum  		ak_cksum16.cksum
#define	ak_winedge0 	ak_flow.window_edge0
#define ak_winedge		ak_flow.window_edge
#define ak_subseq		ak_flow.subseq
#define	ak_cdt			ak_flow.cdt
};

struct tpduv_xak {
	struct cksum16 		xak_cksum16;
#define xak_cksum  		xak_cksum16.cksum
};

struct tpduv_er {
	unsigned char 		er_invalid:31;	/* invalid TPDU */
	struct cksum16 		er_cksum16;
#define er_cksum  		er_cksum16.cksum
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

#endif /* _NETTPI_TPI_TPDU_VARIABLE_H_ */
