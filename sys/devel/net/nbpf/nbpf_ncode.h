/*	$NetBSD: npf_ncode.h,v 1.5.6.5 2013/02/11 21:49:49 riz Exp $	*/

/*-
 * Copyright (c) 2009-2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This material is based upon work partially supported by The
 * NetBSD Foundation under a contract with Mindaugas Rasiukevicius.
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

#ifndef _NET_NBPF_NCODE_H_
#define _NET_NBPF_NCODE_H_

/*
 * NBPF n-code interface.
 */
struct nbpf_ncode {
	const void 			*iptr; 	/* N-code instruction pointer. */
	uint32_t			d;		/* Local, state variables. */
	uint32_t			i;
	uint32_t			n;
};

struct nbpf_insn {
	struct nbpf_ncode  	*ncode;
	const void 			*nc;
	size_t 				sz;
};

struct nbpf_insn *nbpf_nc_alloc(int);
void nbpf_nc_free(struct nbpf_insn *);

int nbpf_filter(nbpf_state_t *, struct nbpf_insn *, nbpf_buf_t *, int);
int nbpf_validate(struct nbpf_insn *, size_t, int *);

/* Error codes. */
#define	NBPF_ERR_OPCODE		-1	/* Invalid instruction. */
#define	NBPF_ERR_JUMP		-2	/* Invalid jump (e.g. out of range). */
#define	NBPF_ERR_REG		-3	/* Invalid register. */
#define	NBPF_ERR_INVAL		-4	/* Invalid argument value. */
#define	NBPF_ERR_RANGE		-5	/* Processing out of range. */

/* Number of registers: [0..N] */
#define	NBPF_NREGS			4

/* Maximum loop count. */
#define	NBPF_LOOP_LIMIT		100

/* Shift to check if CISC-like instruction. */
#define	NBPF_CISC_SHIFT		7
#define	NBPF_CISC_OPCODE(insn)	(insn >> NBPF_CISC_SHIFT)

/*
 * RISC-like n-code instructions.
 */

/* Return, advance, jump, tag and invalidate instructions. */

#define	NBPF_OPCODE_RET			0x00
#define	NBPF_OPCODE_ADVR		0x01
#define	NBPF_OPCODE_J			0x02
#define	NBPF_OPCODE_INVL		0x03
#define	NBPF_OPCODE_TAG			0x04

/* Set and load instructions. */
#define	NBPF_OPCODE_MOVE		0x10
#define	NBPF_OPCODE_LW			0x11

/* Compare and jump instructions. */
#define	NBPF_OPCODE_CMP			0x21
#define	NBPF_OPCODE_CMPR		0x22
#define	NBPF_OPCODE_BEQ			0x23
#define	NBPF_OPCODE_BNE			0x24
#define	NBPF_OPCODE_BGT			0x25
#define	NBPF_OPCODE_BLT			0x26

/* Arithmetic instructions. */
#define	NBPF_OPCODE_ADD			0x30
#define	NBPF_OPCODE_SUB			0x31
#define	NBPF_OPCODE_MULT		0x32
#define	NBPF_OPCODE_DIV			0x33

/* Bitwise instructions. */
#define	NBPF_OPCODE_NOT			0x40
#define	NBPF_OPCODE_AND			0x41
#define	NBPF_OPCODE_OR			0x42
#define	NBPF_OPCODE_XOR			0x43
#define	NBPF_OPCODE_SLL			0x44
#define	NBPF_OPCODE_SRL			0x45

/*
 * CISC-like n-code instructions.
 */

#define	NBPF_OPCODE_ETHER		0x80
#define	NBPF_OPCODE_PROTO		0x81

#define	NBPF_OPCODE_IP4MASK		0x90
#define	NBPF_OPCODE_TABLE		0x91
#define	NBPF_OPCODE_ICMP4		0x92
#define	NBPF_OPCODE_IP6MASK		0x93
#define	NBPF_OPCODE_ICMP6		0x94

#define	NBPF_OPCODE_TCP_PORTS	0xa0
#define	NBPF_OPCODE_UDP_PORTS	0xa1
#define	NBPF_OPCODE_TCP_FLAGS	0xa2

#ifdef NBPF_OPCODES_STRINGS

# define	NBPF_OPERAND_NONE			0
# define	NBPF_OPERAND_REGISTER		1
# define	NBPF_OPERAND_KEY			2
# define	NBPF_OPERAND_VALUE			3
# define	NBPF_OPERAND_SD				4
# define	NBPF_OPERAND_SD_SRC			1
# define	NBPF_OPERAND_SD_DST			0
# define	NBPF_OPERAND_REL_ADDRESS	5
# define	NBPF_OPERAND_NET_ADDRESS4	6
# define	NBPF_OPERAND_NET_ADDRESS6	7
# define	NBPF_OPERAND_ETHER_TYPE		8
# define	NBPF_OPERAND_SUBNET			9
# define	NBPF_OPERAND_LENGTH			10
# define	NBPF_OPERAND_TABLE_ID		11
# define	NBPF_OPERAND_ICMP_TYPE_CODE	12
# define	NBPF_OPERAND_TCP_FLAGS_MASK	13
# define	NBPF_OPERAND_PORT_RANGE		14
# define	NBPF_OPERAND_PROTO			15

static const struct nbpf_instruction {
	const char 	*name;
	uint8_t		op[4];
} nbpf_instructions[] = {
	[NBPF_OPCODE_RET] = {
		.name = "ret",
		.op = {
			[0] = NBPF_OPERAND_VALUE,
		},
	},
	[NBPF_OPCODE_ADVR] = {
		.name = "advr",
		.op = {
			[0] = NBPF_OPERAND_REGISTER,
		},
	},
	[NBPF_OPCODE_J] = {
		.name = "j",
		.op = {
			[0] = NBPF_OPERAND_REL_ADDRESS,
		},
	},
	[NBPF_OPCODE_INVL] = {
		.name = "invl",
	},
	[NBPF_OPCODE_TAG] = {
		.name = "tag",
		.op = {
			[0] = NBPF_OPERAND_KEY,
			[1] = NBPF_OPERAND_VALUE,
		},
	},
	[NBPF_OPCODE_MOVE] = {
		.name = "move",
		.op = {
			[0] = NBPF_OPERAND_VALUE,
			[1] = NBPF_OPERAND_REGISTER,
		},
	},
	[NBPF_OPCODE_LW] = {
		.name = "lw",
		.op = {
			[0] = NBPF_OPERAND_LENGTH,
			[1] = NBPF_OPERAND_REGISTER,
		},
	},
	[NBPF_OPCODE_CMP] = {
		.name = "cmp",
		.op = {
			[0] = NBPF_OPERAND_VALUE,
			[1] = NBPF_OPERAND_REGISTER,
		},
	},
	[NBPF_OPCODE_CMPR] = {
		.name = "cmpr",
		.op = {
			[0] = NBPF_OPERAND_REGISTER,
			[1] = NBPF_OPERAND_REGISTER,
		},
	},
	[NBPF_OPCODE_BEQ] = {
		.name = "beq",
		.op = {
			[0] = NBPF_OPERAND_REL_ADDRESS,
		},
	},
	[NBPF_OPCODE_BNE] = {
		.name = "bne",
		.op = {
			[0] = NBPF_OPERAND_REL_ADDRESS,
		},
	},
	[NBPF_OPCODE_BGT] = {
		.name = "bge",
		.op = {
			[0] = NBPF_OPERAND_REL_ADDRESS,
		},
	},
	[NBPF_OPCODE_BLT] = {
		.name = "blt",
		.op = {
			[0] = NBPF_OPERAND_REL_ADDRESS,
		},
	},
	[NBPF_OPCODE_ADD] = {
		.name = "add",
		.op = {
			[0] = NBPF_OPERAND_VALUE,
			[1] = NBPF_OPERAND_REGISTER,
		},
	},
	[NBPF_OPCODE_SUB] = {
		.name = "sub",
		.op = {
			[0] = NBPF_OPERAND_VALUE,
			[1] = NBPF_OPERAND_REGISTER,
		},
	},
	[NBPF_OPCODE_MULT] = {
		.name = "mult",
		.op = {
			[0] = NBPF_OPERAND_VALUE,
			[1] = NBPF_OPERAND_REGISTER,
		},
	},
	[NBPF_OPCODE_DIV] = {
		.name = "div",
		.op = {
			[0] = NBPF_OPERAND_VALUE,
			[1] = NBPF_OPERAND_REGISTER,
		},
	},
	[NBPF_OPCODE_NOT] = {
		.name = "not",
		.op = {
			[0] = NBPF_OPERAND_VALUE,
			[1] = NBPF_OPERAND_REGISTER,
		},
	},
	[NBPF_OPCODE_AND] = {
		.name = "and",
		.op = {
			[0] = NBPF_OPERAND_VALUE,
			[1] = NBPF_OPERAND_REGISTER,
		},
	},
	[NBPF_OPCODE_OR] = {
		.name = "or",
		.op = {
			[0] = NBPF_OPERAND_VALUE,
			[1] = NBPF_OPERAND_REGISTER,
		},
	},
	[NBPF_OPCODE_XOR] = {
		.name = "xor",
		.op = {
			[0] = NBPF_OPERAND_VALUE,
			[1] = NBPF_OPERAND_REGISTER,
		},
	},
	[NBPF_OPCODE_SLL] = {
		.name = "sll",
		.op = {
			[0] = NBPF_OPERAND_VALUE,
			[1] = NBPF_OPERAND_REGISTER,
		},
	},
	[NBPF_OPCODE_SRL] = {
		.name = "srl",
		.op = {
			[0] = NBPF_OPERAND_VALUE,
			[1] = NBPF_OPERAND_REGISTER,
		},
	},
	[NBPF_OPCODE_ETHER] = {
		.name = "ether",
		.op = {
			[0] = NBPF_OPERAND_SD,
			[1] = NBPF_OPERAND_NET_ADDRESS4,
			[2] = NBPF_OPERAND_ETHER_TYPE,
		},
	},
	[NBPF_OPCODE_PROTO] = {
		.name = "proto",
		.op = {
			[0] = NBPF_OPERAND_PROTO,
		},
	},
	[NBPF_OPCODE_IP4MASK] = {
		.name = "ip4mask",
		.op = {
			[0] = NBPF_OPERAND_SD,
			[1] = NBPF_OPERAND_NET_ADDRESS4,
			[2] = NBPF_OPERAND_SUBNET,
		},
	},
	[NBPF_OPCODE_TABLE] = {
		.name = "table",
		.op = {
			[0] = NBPF_OPERAND_SD,
			[1] = NBPF_OPERAND_TABLE_ID,
		},
	},
	[NBPF_OPCODE_ICMP4] = {
		.name = "icmp4",
		.op = {
			[0] = NBPF_OPERAND_ICMP_TYPE_CODE,
		},
	},
	[NBPF_OPCODE_ICMP6] = {
		.name = "icmp6",
		.op = {
			[0] = NBPF_OPERAND_ICMP_TYPE_CODE,
		},
	},
	[NBPF_OPCODE_IP6MASK] = {
		.name = "ip6mask",
		.op = {
			[0] = NBPF_OPERAND_SD,
			[1] = NBPF_OPERAND_NET_ADDRESS6,
			[2] = NBPF_OPERAND_SUBNET,
		},
	},
	[NBPF_OPCODE_TCP_PORTS] = {
		.name = "tcp_ports",
		.op = {
			[0] = NBPF_OPERAND_SD,
			[1] = NBPF_OPERAND_PORT_RANGE,
		},
	},
	[NBPF_OPCODE_UDP_PORTS] = {
		.name = "udp_ports",
		.op = {
			[0] = NBPF_OPERAND_SD,
			[1] = NBPF_OPERAND_PORT_RANGE,
		},
	},
	[NBPF_OPCODE_TCP_FLAGS] = {
		.name = "tcp_flags",
		.op = {
			[0] = NBPF_OPERAND_TCP_FLAGS_MASK,
		},
	},
};
#endif /* NBPF_OPCODES_STRINGS */

#endif /* _NET_NBPF_NCODE_H_ */
