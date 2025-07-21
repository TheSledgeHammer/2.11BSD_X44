/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	@(#)tuba_table.c	8.6 (Berkeley) 9/22/94
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/kernel.h>

#include <net/if.h>
#include <net/radix.h>

#include <netiso/iso.h>
#include <netiso/tuba_table.h>

int	tuba_table_size;
struct	tuba_cache **tuba_table;
struct	radix_node_head *tuba_tree;
extern	int arpt_keep, arpt_prune;	/* use same values as arp cache */

static void	tuba_callout(struct	tuba_cache *);

void
tuba_timer(void)
{
	register struct	tuba_cache tc;
	int	s;

	s = splnet();
	tuba_callout(&tc);
	splx(s);
}

static void
tuba_callout(tc)
	struct	tuba_cache *tc;
{
	int	i;
	long timelimit;

	timelimit = time.tv_sec - arpt_keep;
	callout_init(&tc->tc_callout);
	callout_schedule(&tc->tc_callout, arpt_prune * hz);
	for (i = tuba_table_size; i > 0; i--) {
		if ((tc = tuba_table[i]) && (tc->tc_refcnt == 0) && (tc->tc_time < timelimit)) {
			tuba_table[i] = 0;
			rn_delete(&tc->tc_siso.siso_addr, NULL, tuba_tree);
			Free(tc);
		}
	}
}

void
tuba_table_init(void)
{
	rn_inithead((void **)&tuba_tree, 0);
	//timeout(tuba_timer, (caddr_t)0, arpt_prune * hz);
}

int
tuba_lookup(rnh, siso)
	struct radix_node_head *rnh;
	register struct sockaddr_iso *siso;
{
    struct radix_node *rn;
    register struct tuba_cache *tc;
	struct tuba_cache **new;
	int dupentry = 0, sum_a = 0, sum_b = 0, old_size, i;

    rn =  rnh->rnh_matchaddr((caddr_t)&siso->siso_addr, tuba_tree);
    if (rn && (rn->rn_flags & RNF_ROOT) == 0) {
        tc = (struct tuba_cache *)rn;
        tc->tc_time = time.tv_sec;
        i = tc->tc_index;
done:
        siso->siso_nlen--;
        return (i);
    }
    R_Malloc(tc, struct tuba_cache *, sizeof(*tc));
    if (tc == NULL) {
        i = 0;
		goto done;
    }
    Bzero(tc, sizeof(*tc));
	tc->tc_addr = siso->siso_addr;
	siso->siso_nlen--;
	tc->tc_siso.siso_addr = siso->siso_addr;
	rn_insert(&tc->tc_addr, tuba_tree, &dupentry, tc->tc_nodes);
	if (dupentry) {
		panic("tuba_lookup 1");
	}
	tc->tc_siso.siso_family = AF_ISO;
	tc->tc_siso.siso_len = sizeof(tc->tc_siso);
	tc->tc_time = time.tv_sec;
	for (i = sum_a = tc->tc_siso.siso_nlen; --i >= 0; ) {
		*(i & 1 ? &sum_a : &sum_b) += (u_char)tc->tc_siso.siso_data[i];
	}
	REDUCE(tc->tc_sum, (sum_a << 8) + sum_b);
	HTONS(tc->tc_sum);
	SWAB(tc->tc_ssum, tc->tc_sum);
	for (i = tuba_table_size; i > 0; i--) {
		if (tuba_table[i] == 0) {
			goto fixup;
		}
	}
	old_size = tuba_table_size;
	if (tuba_table_size == 0) {
		tuba_table_size = 15;
	}
	if (tuba_table_size > 0x7fff) {
		return (0);
	}
	tuba_table_size = 1 + 2 * tuba_table_size;
	i = (tuba_table_size + 1) * sizeof(tc);
	R_Malloc(new, struct tuba_cache **, i);
	if (new == 0) {
		tuba_table_size = old_size;
		rn_delete(&tc->tc_addr, NULL, tuba_tree);
		Free(tc);
		return (0);
	}
    Bzero(new, i);
	if (tuba_table) {
		Bcopy(tuba_table, new, i >> 1);
		Free(tuba_table);
	}
    tuba_table = new;
	i = tuba_table_size;
fixup:
	tuba_table[i] = tc;
	tc->tc_index = i;
    return (tc->tc_index);
}
