/*	$NetBSD: npf_disassemble.c,v 1.3.2.7 2012/08/13 19:43:44 riz Exp $	*/

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * NPF n-code disassembler.
 *
 * FIXME: config generation should be redesigned..
 */
#include <sys/cdefs.h>
__RCSID("$NetBSD: npf_disassemble.c,v 1.3.2.7 2012/08/13 19:43:44 riz Exp $");

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <net/if.h>

#include <net/nbpf/nbpf.h>
#define NBPF_OPCODES_STRINGS
#include <net/nbpf/nbpf_ncode.h>

#include "ncap.h"

enum {
	NBPF_SHOW_SRCADDR,
	NBPF_SHOW_DSTADDR,
	NBPF_SHOW_SRCPORT,
	NBPF_SHOW_DSTPORT,
	NBPF_SHOW_PROTO,
	NBPF_SHOW_FAMILY,
	NBPF_SHOW_ICMP,
	NBPF_SHOW_TCPF,
	NBPF_SHOW_COUNT,
};

struct ncap_inf {
	FILE 			*ni_fp;
	const uint32_t 	*ni_buf;
	size_t			ni_left;
	const uint32_t 	*ni_ipc;
	const uint32_t 	*ni_pc;

	/* Jump target array, its size and current index. */
	const uint32_t 	**ni_targs;
	size_t			ni_targsize;
	size_t			ni_targidx;

	/* Other meta-data. */
	npfvar_t 		*ni_vlist[NBPF_SHOW_COUNT];
	int				ni_proto;
	bool			ni_srcdst;
};

static size_t
ncap_ncode_get_target(const ncap_inf_t *ni, const uint32_t *pc)
{
	for (size_t i = 0; i < ni->ni_targidx; i++) {
		if (ni->ni_targs[i] == pc)
			return i;
	}
	return (size_t)-1;
}

static size_t
ncap_ncode_add_target(ncap_inf_t *ni, const uint32_t *pc)
{
	size_t i = ncap_ncode_get_target(ni, pc);

	/* If found, just return the index. */
	if (i != (size_t)-1) {
		return i;
	}

	/* Grow array, if needed, and add a new target. */
	if (ni->ni_targidx == ni->ni_targsize) {
		ni->ni_targsize += 16;
		ni->ni_targs = xrealloc(ni->ni_targs,
		    ni->ni_targsize * sizeof(uint32_t));
	}
	assert(ni->ni_targidx < ni->ni_targsize);
	i = ni->ni_targidx++;
	ni->ni_targs[i] = pc;
	return i;
}

static void
ncap_ncode_add_vp(ncap_inf_t *ni, char *buf, unsigned idx)
{
	nbpfvar_t *vl = ni->ni_vlist[idx];

	if (vl == NULL) {
		vl = npfvar_create(".list");
		ni->ni_vlist[idx] = vl;
	}
	nbpfvar_t *vp = npfvar_create(".string");
	npfvar_add_element(vp, NPFVAR_STRING, buf, strlen(buf) + 1);
	npfvar_add_elements(vl, vp);
}

static void
nbpf_tcpflags2str(char *buf, unsigned tfl)
{
	int i = 0;

	if (tfl & TH_FIN)
		buf[i++] = 'F';
	if (tfl & TH_SYN)
		buf[i++] = 'S';
	if (tfl & TH_RST)
		buf[i++] = 'R';
	if (tfl & TH_PUSH)
		buf[i++] = 'P';
	if (tfl & TH_ACK)
		buf[i++] = 'A';
	if (tfl & TH_URG)
		buf[i++] = 'U';
	if (tfl & TH_ECE)
		buf[i++] = 'E';
	if (tfl & TH_CWR)
		buf[i++] = 'C';
	buf[i] = '\0';
}

static const char *
ncap_ncode_operand(ncap_inf_t *ni, char *buf, size_t bufsiz, uint8_t operand)
{
	const uint32_t op = *ni->ni_pc;
	struct sockaddr_storage ss;
	unsigned advance;

	/* Advance by one is a default for most cases. */
	advance = 1;

	switch (operand) {
	case NBPF_OPERAND_NONE:
		abort();

	case NBPF_OPERAND_REGISTER:
		if (op & ~0x3) {
			warnx("invalid register operand 0x%x at offset %td",
			    op, ni->ni_pc - ni->ni_buf);
			return NULL;
		}
		snprintf(buf, bufsiz, "R%d", op);
		break;

	case NBPF_OPERAND_KEY:
		snprintf(buf, bufsiz, "key=<0x%x>", op);
		break;

	case NBPF_OPERAND_VALUE:
		snprintf(buf, bufsiz, "value=<0x%x>", op);
		break;

	case NBPF_OPERAND_SD:
		if (op & ~0x1) {
			warnx("invalid src/dst operand 0x%x at offset %td",
			    op, ni->ni_pc - ni->ni_buf);
			return NULL;
		}
		bool srcdst = (op == NBPF_OPERAND_SD_SRC);
		if (ni) {
			ni->ni_srcdst = srcdst;
		}
		snprintf(buf, bufsiz, "%s", srcdst ? "SRC" : "DST");
		break;

	case NBPF_OPERAND_REL_ADDRESS:
		snprintf(buf, bufsiz, "L%zu",
			ncap_ncode_add_target(ni, ni->ni_ipc + op));
		break;

	case NBPF_OPERAND_NET_ADDRESS4: {
		struct sockaddr_in *sin = (void *)&ss;
		sin->sin_len = sizeof(*sin);
		sin->sin_family = AF_INET;
		sin->sin_port = 0;
		memcpy(&sin->sin_addr, ni->ni_pc, sizeof(sin->sin_addr));
		sockaddr_snprintf(buf, bufsiz, "%a", (struct sockaddr *)sin);
		if (ni) {
			ncap_ncode_add_vp(ni, buf,
					ni->ni_srcdst ? NBPF_SHOW_SRCADDR : NBPF_SHOW_DSTADDR);
		}
		advance = sizeof(sin->sin_addr) / sizeof(op);
		break;
	}
	case NBPF_OPERAND_NET_ADDRESS6: {
		struct sockaddr_in6 *sin6 = (void *)&ss;
		sin6->sin6_len = sizeof(*sin6);
		sin6->sin6_family = AF_INET6;
		sin6->sin6_port = 0;
		memcpy(&sin6->sin6_addr, ni->ni_pc, sizeof(sin6->sin6_addr));
		sockaddr_snprintf(buf, bufsiz, "%a", (struct sockaddr *)sin6);
		if (ni) {
			ncap_ncode_add_vp(ni, buf, ni->ni_srcdst ?
			    NBPF_SHOW_SRCADDR : NBPF_SHOW_DSTADDR);
		}
		advance = sizeof(sin6->sin6_addr) / sizeof(op);
		break;
	}
	case NBPF_OPERAND_ETHER_TYPE:
		snprintf(buf, bufsiz, "ether=0x%x", op);
		break;

	case NBPF_OPERAND_PROTO: {
		uint8_t addrlen = (op >> 8) & 0xff;
		uint8_t proto = op & 0xff;

		snprintf(buf, bufsiz, "addrlen=%u, proto=%u", addrlen, proto);
		if (!ni) {
			break;
		}
		switch (proto) {
		case 0xff:
			/* None. */
			break;
		case IPPROTO_TCP:
			ni->ni_proto |= NCAP_MATCH_TCP;
			break;
		case IPPROTO_UDP:
			ni->ni_proto |= NCAP_MATCH_UDP;
			break;
		case IPPROTO_ICMP:
			ni->ni_proto |= NCAP_MATCH_ICMP;
			/* FALLTHROUGH */
		default:
			snprintf(buf, bufsiz, "proto %d", proto);
			ncap_ncode_add_vp(ni, buf, NBPF_SHOW_PROTO);
			break;
		}
		switch (addrlen) {
		case 4:
		case 16:
			snprintf(buf, bufsiz, "family inet%s",
			    addrlen == 16 ? "6" : "");
			ncap_ncode_add_vp(ni, buf, NBPF_SHOW_FAMILY);
			break;
		}
		break;
	}
	case NBPF_OPERAND_SUBNET: {
		snprintf(buf, bufsiz, "/%d", op);
		if (ni && op != NBPF_NO_NETMASK) {
			ncap_ncode_add_vp(ni, buf, ni->ni_srcdst ?
			    NBPF_SHOW_SRCADDR : NBPF_SHOW_DSTADDR);
		}
		break;
	}
	case NBPF_OPERAND_LENGTH:
		snprintf(buf, bufsiz, "length=%d", op);
		break;

	case NBPF_OPERAND_TABLE_ID:
		if (ni) {
			snprintf(buf, bufsiz, "<%d>", op);
			ncap_ncode_add_vp(ni, buf, ni->ni_srcdst ?
			    NBPF_SHOW_SRCADDR : NBPF_SHOW_DSTADDR);
		}
		snprintf(buf, bufsiz, "id=%d", op);
		break;

	case NBPF_OPERAND_ICMP_TYPE_CODE: {
		uint8_t type = (op & 31) ? op >> 8 : 0;
		uint8_t code = (op & 30) ? op & 0xff : 0;

		if (op & ~0xc000ffff) {
			warnx("invalid icmp/type operand 0x%x at offset %td",
			    op, ni->ni_pc - ni->ni_buf);
			return NULL;
		}
		snprintf(buf, bufsiz, "type=%d, code=%d", type, code);
		if (!ni) {
			break;
		}
		ni->ni_proto |= NCAP_MATCH_ICMP;
		if (*ni->ni_ipc == NBPF_OPCODE_ICMP6) {
			snprintf(buf, bufsiz, "proto \"ipv6-icmp\"");
			ncap_ncode_add_vp(ni, buf, NBPF_SHOW_PROTO);
		}
		if (type || code) {
			snprintf(buf, bufsiz,
			    "icmp-type %d code %d", type, code);
			ncap_ncode_add_vp(ni, buf, NBPF_SHOW_ICMP);
		}
		break;
	}
	case NBPF_OPERAND_TCP_FLAGS_MASK: {
		uint8_t tf = op >> 8, tf_mask = op & 0xff;
		if (op & ~0xffff) {
			warnx("invalid flags/mask operand 0x%x at offset %td",
			    op, ni->ni_pc - ni->ni_buf);
			return NULL;
		}
		char tf_buf[16], tfm_buf[16];
		nbpf_tcpflags2str(tf_buf, tf);
		nbpf_tcpflags2str(tfm_buf, tf_mask);
		snprintf(buf, bufsiz, "flags %s/%s", tf_buf, tfm_buf);
		if (ni) {
			ni->ni_proto |= NCAP_MATCH_TCP;
			ncap_ncode_add_vp(ni, buf, NBPF_SHOW_TCPF);
		}
		break;
	}
	case NBPF_OPERAND_PORT_RANGE: {
		in_port_t p1 = ntohs(op >> 16), p2 = ntohs(op & 0xffff);

		if (p1 == p2) {
			snprintf(buf, bufsiz, "%d", p1);
		} else {
			snprintf(buf, bufsiz, "%d-%d", p1, p2);
		}

		if (!ni) {
			break;
		}
		switch (*ni->ni_ipc) {
		case NBPF_OPCODE_TCP_PORTS:
			ni->ni_proto |= NCAP_MATCH_TCP;
			break;
		case NBPF_OPCODE_UDP_PORTS:
			ni->ni_proto |= NCAP_MATCH_UDP;
			break;
		}
		int sd = ni->ni_srcdst ?  NBPF_SHOW_SRCPORT : NBPF_SHOW_DSTPORT;
		if (ni->ni_vlist[sd]) {
			break;
		}
		ncap_ncode_add_vp(ni, buf, sd);
		break;
	}
	default:
		warnx("invalid operand %d at offset %td",
		    operand, ni->ni_pc - ni->ni_buf);
		return NULL;
	}

	if (ni->ni_left < sizeof(op) * advance) {
		warnx("ran out of bytes");
		return NULL;
	}
	ni->ni_pc += advance;
	ni->ni_left -= sizeof(op) * advance;
	return buf;
}

ncap_inf_t *
ncap_ncode_disinf(FILE *fp)
{
	ncap_inf_t *ni = zalloc(sizeof(ncap_inf_t));

	memset(ni, 0, sizeof(ncap_inf_t));
	ni->ni_fp = fp;
	return ni;
}

int
ncap_ncode_disassemble(ncap_inf_t *ni, const void *v, size_t len)
{
	FILE *fp = ni->ni_fp;
	int error = -1;

	ni->ni_buf = v;
	ni->ni_left = len;
	ni->ni_pc = v;

	while (ni->ni_left) {
		const struct nbpf_instruction *insn;
		const uint32_t opcode = *ni->ni_pc;
		size_t target;

		/* Get the opcode. */
		if (opcode & ~0xff) {
			warnx("invalid opcode 0x%x at offset (%td)",
			    opcode, ni->ni_pc - ni->ni_buf);
			goto out;
		}
		insn = &nbpf_instructions[opcode];
		if (insn->name == NULL) {
			warnx("invalid opcode 0x%x at offset (%td)",
			    opcode, ni->ni_pc - ni->ni_buf);
			goto out;
		}

		/*
		 * Lookup target array and prefix with the label,
		 * if this opcode is a jump target.
		 */
		ni->ni_ipc = ni->ni_pc;
		target = ncap_ncode_get_target(ni, ni->ni_pc);
		if (fp) {
			if (target != (size_t)-1) {
				fprintf(fp, "L%zu:", target);
			}
			fprintf(fp, "\t%s", insn->name);
		}
		if (ni->ni_left < sizeof(opcode)) {
			warnx("ran out of bytes");
			return -1;
		}
		ni->ni_left -= sizeof(opcode);
		ni->ni_pc++;
		for (size_t i = 0; i < __arraycount(insn->op); i++) {
			const uint8_t o = insn->op[i];
			const char *op;
			char buf[256];

			if (o == NBPF_OPERAND_NONE) {
				break;
			}
			op = ncap_ncode_operand(ni, buf, sizeof(buf), o);
			if (op == NULL) {
				goto out;
			}
			if (fp) {
				fprintf(fp, "%s%s", i == 0 ? " " : ", ", op);
			}
		}
		if (fp) {
			fprintf(fp, "\n");
		}
	}
	error = 0;
out:
	free(ni->ni_targs);
	return error;
}
