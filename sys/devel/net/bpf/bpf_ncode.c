/*	$NetBSD: npf_processor.c,v 1.9.2.5 2013/02/11 21:49:49 riz Exp $	*/

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

/*
 * NPF n-code processor.
 *	Inspired by the Berkeley Packet Filter.
 *
 * Few major design goals are:
 *
 * - Keep engine lightweight, well abstracted and simple.
 * - Avoid knowledge of internal network buffer structures (e.g. mbuf).
 * - Avoid knowledge of network protocols.
 *
 * There are two instruction sets: RISC-like and CISC-like.  The later are
 * instructions to cover most common filter cases, and reduce interpretation
 * overhead.  These instructions use protocol knowledge and are supposed to
 * be fully optimized.
 *
 * N-code memory address and thus instructions should be word aligned.
 * All processing is done in 32 bit words, since both instructions (their
 * codes) and arguments use 32 bits words.
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/types.h>
#include <sys/malloc.h>

#include "bpf_mbuf.h"
#include "bpf_ncode.h"

/*
 * nc_fetch_word: fetch a word (32 bits) from the n-code and increase
 * instruction pointer by one word.
 */
static inline const void *
nc_fetch_word(const void *iptr, uint32_t *a)
{
	const uint32_t *tptr = (const uint32_t *)iptr;

	KASSERT(ALIGNED_POINTER(iptr, uint32_t));
	*a = *tptr++;
	return tptr;
}

/*
 * nc_fetch_double: fetch two words (2 x 32 bits) from the n-code and
 * increase instruction pointer by two words.
 */
static inline const void *
nc_fetch_double(const void *iptr, uint32_t *a, uint32_t *b)
{
	const uint32_t *tptr = (const uint32_t *)iptr;

	KASSERT(ALIGNED_POINTER(iptr, uint32_t));
	*a = *tptr++;
	*b = *tptr++;
	return tptr;
}

/*
 * nc_jump: helper function to jump to specified line (32 bit word)
 * in the n-code, fetch a word, and update the instruction pointer.
 */
static inline const void *
nc_jump(const void *iptr, int n, u_int *lcount)
{

	/* Detect infinite loops. */
	if (__predict_false(*lcount == 0)) {
		return NULL;
	}
	*lcount = *lcount - 1;
	return (const uint32_t *)iptr + n;
}

/*
 * npf_ncode_process: process n-code using data of the specified packet.
 *
 * => Argument nbuf (network buffer) is opaque to this function.
 * => Chain of nbufs (and their data) should be protected from any change.
 * => N-code memory address and thus instructions should be aligned.
 * => N-code should be protected from any change.
 * => Routine prevents from infinite loop.
 */
int
bpf_ncode_process(bpf_cache_t *npc, struct bpf_insn_ext *pc, nbuf_t *nbuf, const int layer)
{
	struct bpf_ncode *ncode;
	/* Virtual registers. */
	uint32_t	regs[NPF_NREGS];
	bpf_addr_t addr;
	u_int lcount;
	int cmpval;

	ncode = pc->ncode;
	ncode->iptr = ncode;
	regs[0] = layer;

	lcount = NPF_LOOP_LIMIT;
	cmpval = 0;

process_next:
	/*
	 * Loop must always start on instruction, therefore first word
	 * should be an opcode.  Most used instructions are checked first.
	 */
	ncode->iptr = nc_fetch_word(ncode->iptr, &ncode->d);
	if (__predict_true(NPF_CISC_OPCODE(ncode->d))) {
		/* It is a CISC-like instruction. */
		goto cisc_like;
	}

	/*
	 * RISC-like instructions.
	 *
	 * - ADVR, LW, CMP, CMPR
	 * - BEQ, BNE, BGT, BLT
	 * - RET, TAG, MOVE
	 * - AND, J, INVL
	 */
	switch (ncode->d) {
	case NPF_OPCODE_ADVR:
		ncode->iptr = nc_fetch_word(ncode->iptr, &ncode->i); /* Register */
		KASSERT(ncode->i < NPF_NREGS);
		if (!nbuf_advance(nbuf, regs[ncode->i], 0)) {
			goto fail;
		}
		break;
	case NPF_OPCODE_LW: {
		void *n_ptr;

		ncode->iptr = nc_fetch_double(ncode->iptr, &ncode->n, &ncode->i);	/* Size, register */
		KASSERT(ncode->i < NPF_NREGS);
		KASSERT(ncode->n >= sizeof(uint8_t) && ncode->n <= sizeof(uint32_t));

		n_ptr = nbuf_ensure_contig(nbuf, ncode->n);
		if (nbuf_flag_p(nbuf, NBUF_DATAREF_RESET)) {
			npf_recache(npc, nbuf);
		}
		if (n_ptr == NULL) {
			goto fail;
		}
		memcpy(&regs[ncode->i], n_ptr, ncode->n);
		break;
	}
	case NPF_OPCODE_CMP:
		ncode->iptr = nc_fetch_double(ncode->iptr, &ncode->n, &ncode->i);	/* Value, register */
		KASSERT(ncode->i < NPF_NREGS);
		if (ncode->n != regs[ncode->i]) {
			cmpval = (ncode->n > regs[ncode->i]) ? 1 : -1;
		} else {
			cmpval = 0;
		}
		break;
	case NPF_OPCODE_CMPR:
		ncode->iptr = nc_fetch_double(ncode->iptr, &ncode->n, &ncode->i);	/* Value, register */
		KASSERT(ncode->i < NPF_NREGS);
		if (regs[ncode->n] != regs[ncode->i]) {
			cmpval = (regs[ncode->n] > regs[ncode->i]) ? 1 : -1;
		} else {
			cmpval = 0;
		}
		break;
	case NPF_OPCODE_BEQ:
		ncode->iptr = nc_fetch_word(ncode->iptr, &ncode->n);	/* N-code line */
		if (cmpval == 0)
			goto make_jump;
		break;
	case NPF_OPCODE_BNE:
		ncode->iptr = nc_fetch_word(ncode->iptr, &ncode->n);
		if (cmpval != 0)
			goto make_jump;
		break;
	case NPF_OPCODE_BGT:
		ncode->iptr = nc_fetch_word(ncode->iptr, &ncode->n);
		if (cmpval > 0)
			goto make_jump;
		break;
	case NPF_OPCODE_BLT:
		ncode->iptr = nc_fetch_word(ncode->iptr, &ncode->n);
		if (cmpval < 0)
			goto make_jump;
		break;
	case NPF_OPCODE_RET:
		(void)nc_fetch_word(ncode->iptr, &ncode->n);		/* Return value */
		return ncode->n;
	case NPF_OPCODE_TAG:
		ncode->iptr = nc_fetch_double(ncode->iptr, &ncode->n, &ncode->i); /* Key, value */
		if (nbuf_add_tag(nbuf, ncode->n, ncode->i)) {
			goto fail;
		}
		break;
	case NPF_OPCODE_MOVE:
		ncode->iptr = nc_fetch_double(ncode->iptr, &ncode->n, &ncode->i); /* Value, register */
		KASSERT(ncode->i < NPF_NREGS);
		regs[ncode->i] = ncode->n;
		break;
	case NPF_OPCODE_AND:
		ncode->iptr = nc_fetch_double(ncode->iptr, &ncode->n, &ncode->i); /* Value, register */
		KASSERT(ncode->i < NPF_NREGS);
		regs[ncode->i] = ncode->n & regs[ncode->i];
		break;
	case NPF_OPCODE_J:
		ncode->iptr = nc_fetch_word(ncode->iptr, &ncode->n); /* N-code line */
make_jump:
		ncode->iptr = nc_jump(ncode->iptr, ncode->n - 2, &lcount);
		if (__predict_false(ncode->iptr == NULL)) {
			goto fail;
		}
		break;
	case NPF_OPCODE_INVL:
		/* Invalidate all cached data. */
		npc->bpc_info = 0;
		break;
	default:
		/* Invalid instruction. */
		KASSERT(false);
	}
	goto process_next;

cisc_like:
	/*
	 * CISC-like instructions.
	 */
	switch (ncode->d) {
	case NPF_OPCODE_IP4MASK:
		/* Source/destination, network address, subnet. */
		ncode->iptr = nc_fetch_word(ncode->iptr, &ncode->d);
		ncode->iptr = nc_fetch_double(ncode->iptr, &addr.s6_addr32[0], &ncode->n);
		cmpval = npf_iscached(npc, NPC_IP46) ? npf_match_ipmask(npc,
		    (sizeof(struct in_addr) << 1) | (ncode->d & 0x1),
		    &addr, (npf_netmask_t)ncode->n) : -1;
		break;
	case NPF_OPCODE_IP6MASK:
		/* Source/destination, network address, subnet. */
		ncode->iptr = nc_fetch_word(ncode->iptr, &ncode->d);
		ncode->iptr = nc_fetch_double(ncode->iptr,
		    &addr.s6_addr32[0], &addr.s6_addr32[1]);
		ncode->iptr = nc_fetch_double(ncode->iptr,
		    &addr.s6_addr32[2], &addr.s6_addr32[3]);
		ncode->iptr = nc_fetch_word(ncode->iptr, &ncode->n);
		cmpval = npf_iscached(npc, NPC_IP46) ? npf_match_ipmask(npc,
		    (sizeof(struct in6_addr) << 1) | (ncode->d & 0x1),
		    &addr, (npf_netmask_t)ncode->n) : -1;
		break;
	case NPF_OPCODE_TABLE:
		/* Source/destination, NPF table ID. */
		ncode->iptr = nc_fetch_double(ncode->iptr, &ncode->n, &ncode->i);
		cmpval = npf_iscached(npc, NPC_IP46) ?
		    npf_match_table(npc, ncode->n, ncode->i) : -1;
		break;
	case NPF_OPCODE_TCP_PORTS:
		/* Source/destination, port range. */
		ncode->iptr = nc_fetch_double(ncode->iptr, &ncode->n, &ncode->i);
		cmpval = npf_iscached(npc, NPC_TCP) ?
		    npf_match_tcp_ports(npc, ncode->n, ncode->i) : -1;
		break;
	case NPF_OPCODE_UDP_PORTS:
		/* Source/destination, port range. */
		ncode->iptr = nc_fetch_double(ncode->iptr, &ncode->n, &ncode->i);
		cmpval = npf_iscached(npc, NPC_UDP) ?
		    npf_match_udp_ports(npc, ncode->n, ncode->i) : -1;
		break;
	case NPF_OPCODE_TCP_FLAGS:
		/* TCP flags/mask. */
		ncode->iptr = nc_fetch_word(ncode->iptr, &ncode->n);
		cmpval = npf_iscached(npc, NPC_TCP) ?
		    npf_match_tcpfl(npc, ncode->n) : -1;
		break;
	case NPF_OPCODE_ICMP4:
		/* ICMP type/code. */
		ncode->iptr = nc_fetch_word(ncode->iptr, &ncode->n);
		cmpval = npf_iscached(npc, NPC_ICMP) ?
		    npf_match_icmp4(npc, ncode->n) : -1;
		break;
	case NPF_OPCODE_ICMP6:
		/* ICMP type/code. */
		ncode->iptr = nc_fetch_word(ncode->iptr, &ncode->n);
		cmpval = npf_iscached(npc, NPC_ICMP) ?
		    npf_match_icmp6(npc, ncode->n) : -1;
		break;
	case NPF_OPCODE_PROTO:
		ncode->iptr = nc_fetch_word(ncode->iptr, &ncode->n);
		cmpval = npf_iscached(npc, NPC_IP46) ?
		    npf_match_proto(npc, ncode->n) : -1;
		break;
	case NPF_OPCODE_ETHER:
		/* Source/destination, reserved, ethernet type. */
		ncode->iptr = nc_fetch_word(ncode->iptr, &ncode->d);
		ncode->iptr = nc_fetch_double(ncode->iptr, &ncode->n, &ncode->i);
		cmpval = npf_match_ether(nbuf, ncode->d, ncode->i, &regs[NPF_NREGS - 1]);
		break;
	default:
		/* Invalid instruction. */
		KASSERT(false);
	}
	goto process_next;
fail:
	/* Failure case. */
	return (-1);
}

static int
nc_ptr_check(uintptr_t *iptr, struct bpf_insn_ext *pc, u_int nargs, uint32_t *val, u_int r)
{
	const uint32_t *tptr = (const uint32_t *)*iptr;
	u_int i;

	KASSERT(ALIGNED_POINTER(*iptr, uint32_t));
	KASSERT(nargs > 0);

	if ((uintptr_t)tptr < (uintptr_t)pc->nc) {
		return NPF_ERR_JUMP;
	}

	if ((uintptr_t)tptr + (nargs * sizeof(uint32_t)) > (uintptr_t)pc->nc + pc->sz) {
		return NPF_ERR_RANGE;
	}

	for (i = 1; i <= nargs; i++) {
		if (val && i == r) {
			*val = *tptr;
		}
		tptr++;
	}
	*iptr = (uintptr_t)tptr;
	return 0;
}

/*
 * nc_insn_check: validate the instruction and its arguments.
 */
static int
nc_insn_check(const uintptr_t optr, struct bpf_insn_ext *pc, size_t *adv, size_t *jmp, bool *ret)
{
	uintptr_t iptr = optr;
	uint32_t regidx, val;
	int error;

	/* Fetch the instruction code. */
	error = nc_ptr_check(&iptr, pc, 1, &val, 1);
	if (error)
		return error;

	regidx = 0;
	*ret = false;
	*jmp = 0;

	/*
	 * RISC-like instructions.
	 */
	switch (val) {
	case NPF_OPCODE_ADVR:
		error = nc_ptr_check(&iptr, pc, 1, &regidx, 1);
		break;
	case NPF_OPCODE_LW:
		error = nc_ptr_check(&iptr, pc, 1, &val, 1);
		if (error || val < sizeof(uint8_t) || val > sizeof(uint32_t)) {
			return error ? error : NPF_ERR_INVAL;
		}
		error = nc_ptr_check(&iptr, pc, 1, &regidx, 1);
		break;
	case NPF_OPCODE_CMP:
		error = nc_ptr_check(&iptr, pc, 2, &regidx, 2);
		break;
	case NPF_OPCODE_BEQ:
	case NPF_OPCODE_BNE:
	case NPF_OPCODE_BGT:
	case NPF_OPCODE_BLT:
		error = nc_ptr_check(&iptr, pc, 1, &val, 1);
		/* Validate jump address. */
		goto jmp_check;

	case NPF_OPCODE_RET:
		error = nc_ptr_check(&iptr, pc, 1, NULL, 0);
		*ret = true;
		break;
	case NPF_OPCODE_TAG:
		error = nc_ptr_check(&iptr, pc, 2, NULL, 0);
		break;
	case NPF_OPCODE_MOVE:
		error = nc_ptr_check(&iptr, pc, 2, &regidx, 2);
		break;
	case NPF_OPCODE_CMPR:
		error = nc_ptr_check(&iptr, pc, 1, &regidx, 1);
		/* Handle first register explicitly. */
		if (error || (u_int)regidx < NPF_NREGS) {
			return error ? error : NPF_ERR_REG;
		}
		error = nc_ptr_check(&iptr, pc, 1, &regidx, 1);
		break;
	case NPF_OPCODE_AND:
		error = nc_ptr_check(&iptr, pc, 2, &regidx, 2);
		break;
	case NPF_OPCODE_J:
		error = nc_ptr_check(&iptr, pc, 1, &val, 1);
jmp_check:
		/*
		 * We must check for JMP 0 i.e. to oneself.  Pass the jump
		 * address to the caller, it will validate if it is correct.
		 */
		if (error == 0 && val == 0) {
			return NPF_ERR_JUMP;
		}
		*jmp = val * sizeof(uint32_t);
		break;
	case NPF_OPCODE_INVL:
		break;
	/*
	 * CISC-like instructions.
	 */
	case NPF_OPCODE_IP4MASK:
		error = nc_ptr_check(&iptr, pc, 3, &val, 3);
		if (error) {
			return error;
		}
		if (!val || (val > NPF_MAX_NETMASK && val != NPF_NO_NETMASK)) {
			return NPF_ERR_INVAL;
		}
		break;
	case NPF_OPCODE_IP6MASK:
		error = nc_ptr_check(&iptr, pc, 6, &val, 6);
		if (error) {
			return error;
		}
		if (!val || (val > NPF_MAX_NETMASK && val != NPF_NO_NETMASK)) {
			return NPF_ERR_INVAL;
		}
		break;
	case NPF_OPCODE_TABLE:
		error = nc_ptr_check(&iptr, pc, 2, NULL, 0);
		break;
	case NPF_OPCODE_TCP_PORTS:
		error = nc_ptr_check(&iptr, pc, 2, NULL, 0);
		break;
	case NPF_OPCODE_UDP_PORTS:
		error = nc_ptr_check(&iptr, pc, 2, NULL, 0);
		break;
	case NPF_OPCODE_TCP_FLAGS:
		error = nc_ptr_check(&iptr, pc, 1, NULL, 0);
		break;
	case NPF_OPCODE_ICMP4:
	case NPF_OPCODE_ICMP6:
		error = nc_ptr_check(&iptr, pc, 1, NULL, 0);
		break;
	case NPF_OPCODE_PROTO:
		error = nc_ptr_check(&iptr, pc, 1, NULL, 0);
		break;
	case NPF_OPCODE_ETHER:
		error = nc_ptr_check(&iptr, pc, 3, NULL, 0);
		break;
	default:
		/* Invalid instruction. */
		return NPF_ERR_OPCODE;
	}
	if (error) {
		return error;
	}
	if ((u_int)regidx >= NPF_NREGS) {
		/* Invalid register. */
		return NPF_ERR_REG;
	}
	*adv = iptr - optr;
	return 0;
}

/*
 * nc_jmp_check: validate that jump address points to the instruction.
 * Loop from the begining of n-code until we hit jump address or error.
 */
static inline int
nc_jmp_check(struct bpf_insn_ext *pc, const uintptr_t jaddr)
{
	uintptr_t iaddr;
	int error;

	iaddr = (uintptr_t)pc->nc;
	KASSERT(iaddr != jaddr);
	do {
		size_t _jmp, adv;
		bool _ret;

		error = nc_insn_check(iaddr, pc, &adv, &_jmp, &_ret);
		if (error) {
			break;
		}
		iaddr += adv;

	} while (iaddr != jaddr);

	return error;
}

/*
 * npf_ncode_validate: validate n-code.
 * Performs the following operations:
 *
 * - Checks that each instruction is valid (i.e. existing opcode).
 * - Validates registers i.e. that their indexes are correct.
 * - Checks that jumps are within n-code and to the instructions.
 * - Checks that n-code returns, and processing is within n-code memory.
 */
int
bpf_ncode_validate(struct bpf_insn_ext *pc, int *errat)
{
	const uintptr_t nc_end;
	uintptr_t iptr;
	int error;
	bool ret;

	nc_end = (uintptr_t)pc->nc + pc->sz;
	iptr = (uintptr_t)pc->nc;

	do {
		size_t jmp, adv;

		/* Validate instruction and its arguments. */
		error = nc_insn_check(iptr, pc, &adv, &jmp, &ret);
		if (error) {
			break;
		}

		/* If jumping, check that address points to the instruction. */
		if (jmp && nc_jmp_check(pc, iptr + jmp)) {
			/* Note: the actual error might be different. */
			return NPF_ERR_JUMP;
		}

		/* Advance and check for the end of n-code memory block. */
		iptr += adv;

	} while (iptr != nc_end);

	if (!error) {
		error = ret ? 0 : NPF_ERR_RANGE;
	}
	*errat = (iptr - (uintptr_t) pc->nc) / sizeof(uint32_t);
	return (error);
}
