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

#include "nbpf.h"
#include "nbpf_ncode.h"

#define increment_word(val, ptr) ((val) = (ptr)++)
#define decrement_word(val, ptr) ((val) = (ptr)--)

/*
 * nc_fetch_word: fetch a word (32 bits) from the n-code and increase
 * instruction pointer by one word.
 */
static inline const void *
nc_fetch_word(const void *iptr, uint32_t *a)
{
	const uint32_t *tptr = (const uint32_t*) iptr;

	KASSERT(ALIGNED_POINTER(iptr, uint32_t));
	increment_word(*a, *tptr);
	/*
	*a = *tptr++;
	*/
	return tptr;
}

/*
 * nc_fetch_double: fetch two words (2 x 32 bits) from the n-code and
 * increase instruction pointer by two words.
 */
static inline const void *
nc_fetch_double(const void *iptr, uint32_t *a, uint32_t *b)
{
	const uint32_t *tptr = (const uint32_t*) iptr;

	KASSERT(ALIGNED_POINTER(iptr, uint32_t));
	increment_word(*a, *tptr);
	increment_word(*b, *tptr);
	/*
	*a = *tptr++;
	*b = *tptr++;
	*/
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
	return (const uint32_t*) iptr + n;
}

/*
 * determine if insn is risc or cisc
 * cisc = 0
 * risc = 1
 */
static inline int
nc_insn_get(struct nbpf_ncode *ncode)
{
	if (__predict_true(NBPF_CISC_OPCODE(ncode->d))) {
		return (0);
	}
	return (1);
}

struct nbpf_insn *
nbpf_nc_alloc(size)
	int size;
{
	struct nbpf_insn *ncode;

	ncode = (struct nbpf_insn*) malloc(size, M_DEVBUF, M_WAITOK);
	return (ncode);
}

void
nbpf_nc_free(ncode)
	struct nbpf_insn *ncode;
{
	free(ncode, M_DEVBUF);
}

/*
 * RISC-like instructions.
 */
int
nbpf_risc_ncode(nbpf_state_t *state, struct nbpf_ncode *ncode, nbpf_buf_t *nbuf, void *nptr, int cmpval, const int layer)
{
	/* Virtual registers. */
	uint32_t regs[NBPF_NREGS];
	u_int lcount;

	regs[0] = layer;
	lcount = NBPF_LOOP_LIMIT;

	/*
	 * RISC-like instructions.
	 *
	 * - ADVR, LW, CMP, CMPR
	 * - BEQ, BNE, BGT, BLT
	 * - RET, TAG, MOVE
	 * - AND, J, INVL
	 */
	switch (ncode->d) {
	case NBPF_OPCODE_ADVR:
		ncode->iptr = nc_fetch_word(ncode->iptr, &ncode->i); /* Register */
		KASSERT(ncode->i < NBPF_NREGS);
		nptr = nbpf_advance(&nbuf, nptr, regs[ncode->i]);
		if (__predict_false(nptr == NULL)) {
			goto fail;
		}
		break;
	case NBPF_OPCODE_LW:
		ncode->iptr = nc_fetch_double(ncode->iptr, &ncode->n, &ncode->i); /* Size, register */
		KASSERT(ncode->i < NBPF_NREGS);
		KASSERT(ncode->n >= sizeof(uint8_t) && ncode->n <= sizeof(uint32_t));
		if (nbpf_fetch_datum(nbuf, nptr, ncode->n,
				(uint32_t*) regs + ncode->i)) {
			goto fail;
		}
		break;
	case NBPF_OPCODE_CMP:
		ncode->iptr = nc_fetch_double(ncode->iptr, &ncode->n, &ncode->i); /* Value, register */
		KASSERT(ncode->i < NBPF_NREGS);
		if (ncode->n != regs[ncode->i]) {
			cmpval = (ncode->n > regs[ncode->i]) ? 1 : -1;
		} else {
			cmpval = 0;
		}
		break;
	case NBPF_OPCODE_CMPR:
		ncode->iptr = nc_fetch_double(ncode->iptr, &ncode->n, &ncode->i); /* Value, register */
		KASSERT(ncode->i < NBPF_NREGS);
		if (regs[ncode->n] != regs[ncode->i]) {
			cmpval = (regs[ncode->n] > regs[ncode->i]) ? 1 : -1;
		} else {
			cmpval = 0;
		}
		break;
	case NBPF_OPCODE_BEQ:
		ncode->iptr = nc_fetch_word(ncode->iptr, &ncode->n); /* N-code line */
		if (cmpval == 0) {
			goto make_jump;
		}
		break;
	case NBPF_OPCODE_BNE:
		ncode->iptr = nc_fetch_word(ncode->iptr, &ncode->n);
		if (cmpval != 0) {
			goto make_jump;
		}
		break;
	case NBPF_OPCODE_BGT:
		ncode->iptr = nc_fetch_word(ncode->iptr, &ncode->n);
		if (cmpval > 0) {
			goto make_jump;
		}
		break;
	case NBPF_OPCODE_BLT:
		ncode->iptr = nc_fetch_word(ncode->iptr, &ncode->n);
		if (cmpval < 0) {
			goto make_jump;
		}
		break;
	case NBPF_OPCODE_RET:
		(void) nc_fetch_word(ncode->iptr, &ncode->n); /* Return value */
		return (ncode->n);
	case NBPF_OPCODE_TAG:
		ncode->iptr = nc_fetch_double(ncode->iptr, &ncode->n, &ncode->i); /* Key, value */
		if (nbpf_add_tag(state, nptr, ncode->n, ncode->i)) {
			goto fail;
		}
		break;
	case NBPF_OPCODE_MOVE:
		ncode->iptr = nc_fetch_double(ncode->iptr, &ncode->n, &ncode->i); /* Value, register */
		KASSERT(ncode->i < NBPF_NREGS);
		regs[ncode->i] = ncode->n;
		break;
	case NBPF_OPCODE_AND:
		ncode->iptr = nc_fetch_double(ncode->iptr, &ncode->n, &ncode->i); /* Value, register */
		KASSERT(ncode->i < NBPF_NREGS);
		regs[ncode->i] = ncode->n & regs[ncode->i];
		break;
	case NBPF_OPCODE_J:
		ncode->iptr = nc_fetch_word(ncode->iptr, &ncode->n); /* N-code line */
make_jump:
		ncode->iptr = nc_jump(ncode->iptr, ncode->n - 2, &lcount);
		if (__predict_false(ncode->iptr == NULL)) {
			goto fail;
		}
		break;
	case NBPF_OPCODE_INVL:
		/* Invalidate all cached data. */
		state->nbs_info = 0;
		break;
	default:
		/* Invalid instruction. */
		goto fail;
	}
	/* Success */
	return (0);

fail:
	/* Failure case. */
	return (-1);
}

/*
 * CISC-like instructions.
 */
int
nbpf_cisc_ncode(nbpf_state_t *state, struct nbpf_ncode *ncode, nbpf_buf_t *nbuf, void *nptr, int cmpval, const int layer)
{
	/* Virtual registers. */
	uint32_t regs[NBPF_NREGS];
	nbpf_addr_t addr;

	regs[0] = layer;

	switch (ncode->d) {
	case NBPF_OPCODE_IP4MASK:
		/* Source/destination, network address, subnet. */
		ncode->iptr = nc_fetch_word(ncode->iptr, &ncode->d);
		ncode->iptr = nc_fetch_double(ncode->iptr, &addr.s6_addr32[0],
				&ncode->n);
		cmpval = nbpf_match_ipv4(state, nbuf, nptr,
				(sizeof(struct in_addr) << 1) | (ncode->d & 0x1), &addr,
				(nbpf_netmask_t) ncode->n);
		break;
	case NBPF_OPCODE_IP6MASK:
		/* Source/destination, network address, subnet. */
		ncode->iptr = nc_fetch_word(ncode->iptr, &ncode->d);
		ncode->iptr = nc_fetch_double(ncode->iptr, &addr.s6_addr32[0],
				&addr.s6_addr32[1]);
		ncode->iptr = nc_fetch_double(ncode->iptr, &addr.s6_addr32[2],
				&addr.s6_addr32[3]);
		ncode->iptr = nc_fetch_word(ncode->iptr, &ncode->n);
		cmpval = nbpf_match_ipv6(state, nbuf, nptr,
				(sizeof(struct in6_addr) << 1) | (ncode->d & 0x1), &addr,
				(nbpf_netmask_t) ncode->n);
		break;
	case NBPF_OPCODE_TABLE:
		/* Source/destination, NPF table ID. */
		ncode->iptr = nc_fetch_double(ncode->iptr, &ncode->n, &ncode->i);
		cmpval = nbpf_match_table(state, nbuf, nptr, ncode->n, ncode->i);
		break;
	case NBPF_OPCODE_TCP_PORTS:
		/* Source/destination, port range. */
		ncode->iptr = nc_fetch_double(ncode->iptr, &ncode->n, &ncode->i);
		cmpval = nbpf_match_tcp_ports(state, nbuf, nptr, ncode->n, ncode->i);
		break;
	case NBPF_OPCODE_UDP_PORTS:
		/* Source/destination, port range. */
		ncode->iptr = nc_fetch_double(ncode->iptr, &ncode->n, &ncode->i);
		cmpval = nbpf_match_udp_ports(state, nbuf, nptr, ncode->n, ncode->i);
		break;
	case NBPF_OPCODE_TCP_FLAGS:
		/* TCP flags/mask. */
		ncode->iptr = nc_fetch_word(ncode->iptr, &ncode->n);
		cmpval = nbpf_match_tcpfl(state, nbuf, nptr, ncode->n);
		break;
	case NBPF_OPCODE_ICMP4:
		/* ICMP type/code. */
		ncode->iptr = nc_fetch_word(ncode->iptr, &ncode->n);
		cmpval = nbpf_match_icmp4(state, nbuf, nptr, ncode->n);
		break;
	case NBPF_OPCODE_ICMP6:
		/* ICMP type/code. */
		ncode->iptr = nc_fetch_word(ncode->iptr, &ncode->n);
		cmpval = nbpf_match_icmp6(state, nbuf, nptr, ncode->n);
		break;
	case NBPF_OPCODE_PROTO:
		ncode->iptr = nc_fetch_word(ncode->iptr, &ncode->n);
		cmpval = nbpf_match_proto(state, nbuf, nptr, ncode->n);
		break;
	case NBPF_OPCODE_ETHER:
		/* Source/destination, reserved, ethernet type. */
		ncode->iptr = nc_fetch_word(ncode->iptr, &ncode->d);
		ncode->iptr = nc_fetch_double(ncode->iptr, &ncode->n, &ncode->i);
		cmpval = nbpf_match_ether(nbuf, ncode->d, ncode->n, ncode->i,
				&regs[NBPF_NREGS - 1]);
		break;
	default:
		/* Invalid instruction. */
		goto fail;
	}
	/* Success */
	return (0);

fail:
	/* Failure case. */
	return (-1);
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
nbpf_ncode_process(nbpf_state_t *state, struct nbpf_insn *pc, nbpf_buf_t *nbuf0, const int layer)
{
	/* Pointer of current nbuf in the chain. */
	nbpf_buf_t *nbuf;
	/* Data pointer in the current nbuf. */
	void *nptr;
	/* Virtual registers. */
	uint32_t regs[NBPF_NREGS];
	struct nbpf_ncode *ncode;
	int cmpval;
	int error;

	ncode = pc->ncode;
	ncode->iptr = ncode;
	regs[0] = layer;
	cmpval = 0;

	/* Note: offset = nptr - nbpf_dataptr(nbuf); */
	nbuf = nbuf0;
	nptr = nbpf_dataptr(nbuf);

process_next:
	ncode->iptr = nc_fetch_word(ncode->iptr, &ncode->d);
	if (nc_insn_get(ncode)) {
		error = nbpf_cisc_ncode(state, ncode, nbuf, nptr, cmpval, layer);
	} else {
		error = nbpf_risc_ncode(state, ncode, nbuf, nptr, cmpval, layer);
	}
	if (error != -1) {
		goto process_next;
	}
	return (error);
}

static int
nc_ptr_check(uintptr_t *iptr, struct nbpf_insn *pc, u_int nargs, uint32_t *val, u_int r)
{
	const uint32_t *tptr = (const uint32_t*) *iptr;
	u_int i;

	KASSERT(ALIGNED_POINTER(*iptr, uint32_t));
	KASSERT(nargs > 0);

	if ((uintptr_t) tptr < (uintptr_t) pc->nc) {
		return (NBPF_ERR_JUMP);
	}

	if ((uintptr_t) tptr + (nargs * sizeof(uint32_t))
			> (uintptr_t) pc->nc + pc->sz) {
		return (NBPF_ERR_RANGE);
	}

	for (i = 1; i <= nargs; i++) {
		if (val && i == r) {
			*val = *tptr;
		}
		tptr++;
	}
	*iptr = (uintptr_t) tptr;
	return (0);
}

/*
 * risc_insn_check:
 */
static int
risc_insn_check(uintptr_t iptr, struct nbpf_insn *pc, uint32_t regidx, uint32_t val, size_t *jmp, bool *ret)
{
	int error, res;

	switch (val) {
	case NBPF_OPCODE_ADVR:
		error = nc_ptr_check(&iptr, pc, 1, &regidx, 1);
		break;
	case NBPF_OPCODE_LW:
		break;
	case NBPF_OPCODE_CMP:
		error = nc_ptr_check(&iptr, pc, 2, &regidx, 2);
		break;
	case NBPF_OPCODE_BEQ:
	case NBPF_OPCODE_BNE:
	case NBPF_OPCODE_BGT:
	case NBPF_OPCODE_BLT:
		error = nc_ptr_check(&iptr, pc, 1, &val, 1);
		/* Validate jump address. */
		goto jmp_check;
	case NBPF_OPCODE_RET:
		error = nc_ptr_check(&iptr, pc, 1, NULL, 0);
		*ret = TRUE;
		break;
	case NBPF_OPCODE_TAG:
		error = nc_ptr_check(&iptr, pc, 2, NULL, 0);
		break;
	case NBPF_OPCODE_MOVE:
		error = nc_ptr_check(&iptr, pc, 2, &regidx, 2);
		break;
	case NBPF_OPCODE_CMPR:
		error = nc_ptr_check(&iptr, pc, 1, &regidx, 1);
		/* Handle first register explicitly. */
		if (error || (u_int) regidx < NBPF_NREGS) {
			res = error ? error : NBPF_ERR_REG;
			error = res;
			break;
		}
		error = nc_ptr_check(&iptr, pc, 1, &regidx, 1);
		break;
	case NBPF_OPCODE_AND:
		error = nc_ptr_check(&iptr, pc, 2, &regidx, 2);
		break;
	case NBPF_OPCODE_J:
		error = nc_ptr_check(&iptr, pc, 1, &val, 1);
jmp_check:
		/*
		 * We must check for JMP 0 i.e. to oneself.  Pass the jump
		 * address to the caller, it will validate if it is correct.
		 */
		if (error == 0 && val == 0) {
			error = NBPF_ERR_JUMP;
		}
		*jmp = val * sizeof(uint32_t);
		break;
	case NBPF_OPCODE_INVL:
		break;
	default:
		/* Invalid instruction. */
		return (NBPF_ERR_OPCODE);
	}
	return (error);
}

/*
 * cisc_insn_check:
 */
static int
cisc_insn_check(uintptr_t iptr, struct nbpf_insn *pc, uint32_t regidx, uint32_t val, size_t *jmp, bool *ret)
{
	int error;

	switch (val) {
	case NBPF_OPCODE_IP4MASK:
		error = nc_ptr_check(&iptr, pc, 3, &val, 3);
		if (error) {
			return (error);
		}
		if (!val || (val > NBPF_MAX_NETMASK && val != NBPF_NO_NETMASK)) {
			return (NBPF_ERR_INVAL);
		}
		break;
	case NBPF_OPCODE_IP6MASK:
		error = nc_ptr_check(&iptr, pc, 6, &val, 6);
		if (error) {
			return (error);
		}
		if (!val || (val > NBPF_MAX_NETMASK && val != NBPF_NO_NETMASK)) {
			return (NBPF_ERR_INVAL);
		}
		break;
	case NBPF_OPCODE_TABLE:
		error = nc_ptr_check(&iptr, pc, 2, NULL, 0);
		break;
	case NBPF_OPCODE_TCP_PORTS:
		error = nc_ptr_check(&iptr, pc, 2, NULL, 0);
		break;
	case NBPF_OPCODE_UDP_PORTS:
		error = nc_ptr_check(&iptr, pc, 2, NULL, 0);
		break;
	case NBPF_OPCODE_TCP_FLAGS:
		error = nc_ptr_check(&iptr, pc, 1, NULL, 0);
		break;
	case NBPF_OPCODE_ICMP4:
	case NBPF_OPCODE_ICMP6:
		error = nc_ptr_check(&iptr, pc, 1, NULL, 0);
		break;
	case NBPF_OPCODE_PROTO:
		error = nc_ptr_check(&iptr, pc, 1, NULL, 0);
		break;
	case NBPF_OPCODE_ETHER:
		error = nc_ptr_check(&iptr, pc, 3, NULL, 0);
		break;
	default:
		/* Invalid instruction. */
		return (NBPF_ERR_OPCODE);
	}
	return (error);
}

/*
 * nc_insn_check: validate the instruction and its arguments.
 */
static int
nc_insn_check(const uintptr_t optr, struct nbpf_insn *pc, size_t *adv, size_t *jmp, bool *ret)
{
	struct nbpf_ncode *ncode;
	uintptr_t iptr;
	uint32_t regidx, val;
	int error;

	ncode = pc->ncode;
	iptr = optr;

	error = nc_ptr_check(&iptr, pc, 1, &val, 1);
	if (error) {
		return (error);
	}

	regidx = 0;
	*ret = false;
	*jmp = 0;
	if (nc_insn_get(ncode)) {
		error = cisc_insn_check(iptr, pc, regidx, val, jmp, ret);
	} else {
		error = risc_insn_check(iptr, pc, regidx, val, jmp, ret);
	}
	if (error) {
		return (error);
	}
	if ((u_int) regidx >= NBPF_NREGS) {
		/* Invalid register. */
		return (NBPF_ERR_REG);
	}
	*adv = iptr - optr;
	return (0);
}

/*
 * nc_jmp_check: validate that jump address points to the instruction.
 * Loop from the begining of n-code until we hit jump address or error.
 */
static inline int
nc_jmp_check(struct nbpf_insn *pc, const uintptr_t jaddr)
{
	uintptr_t iaddr;
	int error;

	iaddr = (uintptr_t) pc->nc;
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

	return (error);
}

/*
 * nbpf_ncode_validate: validate n-code.
 * Performs the following operations:
 *
 * - Checks that each instruction is valid (i.e. existing opcode).
 * - Validates registers i.e. that their indexes are correct.
 * - Checks that jumps are within n-code and to the instructions.
 * - Checks that n-code returns, and processing is within n-code memory.
 */
int
nbpf_ncode_validate(struct nbpf_insn *pc, int *errat)
{
	uintptr_t nc_end;
	uintptr_t iptr;
	int error;
	bool ret;

	nc_end = (uintptr_t) pc->nc + pc->sz;
	iptr = (uintptr_t) pc->nc;

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
			return (NBPF_ERR_JUMP);
		}

		/* Advance and check for the end of n-code memory block. */
		iptr += adv;

	} while (iptr != nc_end);

	if (!error) {
		error = ret ? 0 : NBPF_ERR_RANGE;
	}
	*errat = (iptr - (uintptr_t) pc->nc) / sizeof(uint32_t);
	return (error);
}

int
nbpf_filter(nbpf_state_t *state, struct nbpf_insn *pc, nbpf_buf_t *nbuf, int layer)
{
	return (nbpf_ncode_process(state, pc, nbuf, layer));
}

int
nbpf_validate(struct nbpf_insn *f, size_t len, int *ret)
{
	struct nbpf_insn *p;
	int i, error;

	for (i = 0; i < len; ++i) {
		p = &f[i];

		if (p->sz == len) {
			error = nbpf_ncode_validate(p, ret);
			break;
		}
	}
	if (*ret != 0) {
		printf("nbpf_validate: %d\n", *ret);
	}
	return (error);
}
