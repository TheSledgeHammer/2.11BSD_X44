/*
 * The 3-Clause BSD License:
 * Copyright (c) 2025 Martin Kelly
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

/* TPKT: RFC1006 [ISO8072] & ITOT: RFC2126 */

#ifndef _NETTPI_TPI_TPKT_H_
#define _NETTPI_TPI_TPKT_H_

/* TPKT */
#define TPKT_VERSION 	3
#define TPKT_RESERVED	0
#define TPKT_MINLEN 	7
#define TPKT_MAXLEN 	65635

struct tpkt {
    unsigned short	pkt_vers:8;     /* version (8-bits)(vers:3) */
    unsigned int	pkt_len:16;     /* length in octets including packet header (16-bits)(min: 7 max: 65635)  */
    unsigned short	pkt_reserved:8; /* reserved (8-bits)(val:0) */
    struct tpdu_xpd  	*pkt_tpdu;  	/* TPDU Expedited Data Only */
};

/* Primitives */
/* connection establishment */
#define TPKT_CONNECT_REQUEST		0x01	/* open completes */
#define TPKT_CONNECT_INDICATION		0x02	/* listen finishes (passive) */
#define TPKT_CONNECT_RESPONSE		0x04	/* listen completes */
#define TPKT_CONNECT_CONFIRMATION	0x06	/* open finishes (active) */
/* data transfer */
#define TPKT_DATA_REQUEST			0x01	/* send data */
#define TPKT_DATA_INDICATION		0x02	/* data ready followed by read data */
/* connection release (disconnect) */
#define TPKT_DISCONNECT_REQUEST		0x01	/* close */
#define TPKT_DISCONNECT_INDICATION 	0x02	/* connection closes or errors */

/* Actions */
#define T_CONNECT_REQUEST
#define T_CONNECT_RESPONSE
#define T_DISCONNECT_REQUEST
#define T_DATA_REQUEST
#define T_XPD_DATA_REQUEST

/* Events */
#define N_CONNECT_INDICATION
#define N_CONNECT_CONFIRMATION
#define N_DISCONNECT_INDICATION
#define N_DATA_INDICATION
#define N_XPD_DATA_INDICATION


#endif /* _NETTPI_TPI_TPKT_H_ */
