/*	$NetBSD: tp_events.h,v 1.5 1996/02/13 22:10:58 christos Exp $	*/

#ifndef _NETISO_TP_EVENTS_H_
#define _NETISO_TP_EVENTS_H_

struct tp_event {
	int 			ev_number;
	struct timeval 	e_time;
	union {
		struct {
			SeqNum e_low;
			SeqNum e_high;
			int e_retrans;
		} EV_TM_reference;

		struct {
			SeqNum e_low;
			SeqNum e_high;
			int e_retrans;
		} EV_TM_data_retrans;

		struct {
			unsigned char e_reason;
		} EV_ER_TPDU;

		struct {
			struct mbuf *e_data; 	/* first field */
			int e_datalen; 			/* 2nd field */
			unsigned int e_cdt;
		} EV_CR_TPDU;

		struct {
			struct mbuf *e_data; 	/* first field */
			int e_datalen; 			/* 2nd field */
			unsigned short e_sref;
			unsigned char e_reason;
		} EV_DR_TPDU;

		struct {
			struct mbuf *e_data; 	/* first field */
			int e_datalen; 			/* 2nd field */
			unsigned short e_sref;
			unsigned int e_cdt;
		} EV_CC_TPDU;

		struct {
			unsigned int e_cdt;
			SeqNum e_seq;
			SeqNum e_subseq;
			unsigned char e_fcc_present;
		} EV_AK_TPDU;

		struct {
			struct mbuf *e_data; 	/* first field */
			int e_datalen; 			/* 2nd field */
			unsigned int e_eot;
			SeqNum e_seq;
		} EV_DT_TPDU;

		struct {
			struct mbuf *e_data; 	/* first field */
			int e_datalen; 			/* 2nd field */
			SeqNum e_seq;
		} EV_XPD_TPDU;

		struct {
			SeqNum e_seq;
		} EV_XAK_TPDU;

		struct {
			unsigned char e_reason;
		} EV_T_DISC_req;
	} ev_union;
}; /* end struct event */

/* Macros */
#define EV_ATTR(X)			\
	ev_union.EV_##X

#define EV_GET(X, Y)		\
	ev_union.EV_##X.e_##Y

#define EV_SET(X, Y, Z)		\
	ev_union.EV_##X.e_##Y = Z;

#define TM_inact 		0x0
#define TM_retrans 		0x1
#define TM_sendack 		0x2
#define TM_notused 		0x3
#define TM_reference 	0x4
#define TM_data_retrans 0x5
#define ER_TPDU 		0x6
#define CR_TPDU 		0x7
#define DR_TPDU 		0x8
#define DC_TPDU 		0x9
#define CC_TPDU 		0xa
#define AK_TPDU 		0xb
#define DT_TPDU 		0xc
#define XPD_TPDU 		0xd
#define XAK_TPDU 		0xe
#define T_CONN_req 		0xf
#define T_DISC_req 		0x10
#define T_LISTEN_req 	0x11
#define T_DATA_req 		0x12
#define T_XPD_req 		0x13
#define T_USR_rcvd 		0x14
#define T_USR_Xrcvd 	0x15
#define T_DETACH 		0x16
#define T_NETRESET 		0x17
#define T_ACPT_req 		0x18

#endif /* _NETISO_TP_EVENTS_H_ */
