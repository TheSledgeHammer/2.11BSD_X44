/*
 * Copyright (c) University of British Columbia, 1984
 * Copyright (c) 1990, 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 * 		 University of Erlangen-Nuremberg, Germany, 1992
 *
 * This code is derived from software contributed to Berkeley by the
 * Laboratory for Computation Vision and the Computer Science Department
 * of the the University of British Columbia and the Computer Science
 * Department (IV) of the University of Erlangen-Nuremberg, Germany.
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
 *	@(#)x25.h	8.1 (Berkeley) 6/10/93
 */

#ifndef _NETCCITT_X25_H_
#define _NETCCITT_X25_H_

#include <sys/ansi.h>

struct x25opts {
	char	op_flags;	/* miscellaneous options */
	char	op_psize;	/* requested packet size */
	char	op_wsize;	/* window size (1 .. 7) */
	char	op_speed;	/* throughput class */
};
/* pk_var.h defines other lcd_flags */
#define X25_REVERSE_CHARGE 0x01	/* remote DTE pays for call */
#define X25_DBIT	0x02	/* not yet supported */
#define X25_MQBIT	0x04	/* prepend M&Q bit status byte to packet data */
#define X25_OLDSOCKADDR	0x08	/* uses old sockaddr structure */
#define X25_DG_CIRCUIT	0x10	/* lcd_flag: used for datagrams */
#define X25_DG_ROUTING	0x20	/* lcd_flag: peer addr not yet known */
#define X25_MBS_HOLD	0x40	/* lcd_flag: collect m-bit sequences */

/* packet size */
#define X25_PS128	7
#define X25_PS256	8
#define X25_PS512	9

struct x25_addr {
	char    	xa_addr[16];
};

/*
 *  X.25 Socket address structure.  It contains the network id, X.121
 *  address, facilities information, higher level protocol value (first four
 *  bytes of the User Data field), and up to 12 characters of User Data.
 */
struct sockaddr_x25 {
	u_char		x25_len;
	u_char		x25_family;	/* must be AF_CCITT */
	short		x25_net;	/* network id code (usually a dnic) */
	//char		x25_addr[16];	/* X.121 address (null terminated) */
	struct x25_addr	x25_addr;	/* X.121 address (null terminated) */
	struct x25opts 	x25_opts;
	short		x25_udlen;	/* user data field length */
	char		x25_udata[16];	/* user data field */
};

__BEGIN_DECLS
int ccitt_addr(const char *, struct sockaddr_x25 *);
__END_DECLS

#endif /* _NETCCITT_X25_H_ */
