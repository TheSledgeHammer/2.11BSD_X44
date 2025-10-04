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

#include <sys/malloc.h>

#include "tpi_tpdu.h"
#include "tpi_tpkt.h"

/*
 * TCP: Functions
 * tcp_input
 * tcp_output
 * tcp_disconnect
 */

struct tpkt *
tpkt_alloc(unsigned int size)
{
	struct tpkt *pkt;

	MALLOC(pkt, struct tpkt *, sizeof(*pkt), M_PCB, M_WAITOK);
	if (pkt == NULL) {
		return (NULL);
	}
	bzero(pkt, sizeof(*pkt));
	pkt->pkt_vers = TPKT_VERSION;
	pkt->pkt_reserved = TPKT_RESERVED;
	if (size < TPKT_MINLEN) {
		pkt->pkt_len = TPKT_MINLEN;
	} else if (size > TPKT_MAXLEN) {
		pkt->pkt_len = TPKT_MAXLEN;
	} else {
		pkt->pkt_len = size;
	}
	/* initialize as empty */
	pkt->pkt_tpdu = NULL;
	return (pkt);
}

void
tpkt_free(struct tpkt *pkt)
{
	if (pkt != NULL) {
		FREE(pkt, M_PCB);
	}
}

int
tpkt_connect(struct tpkt *pkt, int cmd)
{
	int error;

	switch (cmd) {
	case TPKT_CONNECT_REQUEST:
	case TPKT_CONNECT_INDICATION:
	case TPKT_CONNECT_RESPONSE:
	case TPKT_CONNECT_CONFIRMATION:
	}
	return (error);
}

int
tpkt_data_transfer(struct tpkt *pkt, int cmd)
{
	int error;

	switch (cmd) {
	case TPKT_DATA_REQUEST:
	case TPKT_DATA_INDICATION:
	}
	return (error);
}

int
tpkt_disconnect(struct tpkt *pkt, int cmd)
{
	int error;

	switch (cmd) {
	case TPKT_DISCONNECT_REQUEST:
	case TPKT_DISCONNECT_INDICATION:
	}
	return (error);
}
