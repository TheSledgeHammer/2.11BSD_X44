/*	$KAME: ip6_fw.h,v 1.13 2007/06/14 12:09:44 itojun Exp $	*/

/*
 * Copyright (C) 1998, 1999, 2000 and 2001 WIDE Project.
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Copyright (c) 1993 Daniel Boulet
 * Copyright (c) 1994 Ugen J.S.Antsilevich
 *
 * Redistribution and use in source forms, with and without modification,
 * are permitted provided that this entire comment appears intact.
 *
 * Redistribution in binary form may occur without any restrictions.
 * Obviously, it would be nice if you gave credit where credit is due
 * but requiring it would be too onerous.
 *
 * This software is provided ``AS IS'' without any warranties of any kind.
 *
 */

#ifndef _IP_FW_H_
#define _IP_FW_H_

#include <sys/queue.h>

/* IPv4/v6 */
#define IP_FW_IFNLEN     		IFNAMSIZ
#define IP_FW_MAX_PORTS			10			/* A reasonable maximum */
#define IP_FW_ICMPTYPES_SIZE	(sizeof(unsigned) * 8)

/* IPv4 Only */
#define IP4_FW_ICMPTYPES_MAX	128
#define IPV4_FW_ICMPTYPES_DIM 	(IP4_FW_ICMPTYPES_MAX / IP_FW_ICMPTYPES_SIZE)

/* IPv6 Only */
#define IP6_FW_ICMPTYPES_MAX	256
#define IPV6_FW_ICMPTYPES_DIM 	(IP6_FW_ICMPTYPES_MAX / IP_FW_ICMPTYPES_SIZE)

struct ip_fw_chain {
	LIST_ENTRY(ip_fw_chain) chain4;
	LIST_ENTRY(ip_fw_chain) chain6;
	struct ip4_fw			*rule4;
	struct ip6_fw    		*rule6;
};

#endif /* _IP_FW_H_ */
