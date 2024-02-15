/*
 * tuba_subr.c
 *
 *  Created on: 15 Feb 2024
 *      Author: marti
 */

#include <sys/errno.h>
#include <sys/mbuf.h>

#include <netinet/ip_var.h>

#include <netiso/argo_debug.h>
#include <netiso/iso.h>
#include <netiso/clnp.h>
#include <netiso/iso_pcb.h>
#include <netiso/iso_var.h>
#include <netiso/tuba_table.h>

struct tuba_cksum {
	int		tck_error;
	int		tck_offset;
	int		tck_thlen;
	u_long	tck_sum;
};

void
tuba_init()
{

}

int
tuba_cksum(sum, offset, siso, index)
	u_long *sum;
	int *offset;
	struct sockaddr_iso **siso;
	u_long index;
{
	register struct tuba_cache *tc;
	int error;

	error = 0;
	if (index <= tuba_table_size && (tc = tuba_table[index])) {
		if (siso) {
			*siso = &tc->tc_siso;
		}
		*sum += ((*offset & 1 ? tc->tc_ssum : tc->tc_sum) + (0xffff ^ index));
		*offset += (tc->tc_siso.siso_nlen + 1);
	} else {
		error = 1;
	}
	return (error);
}

int
tuba_tcp_checksum(af, m, th, siso)
	sa_family_t af;
	struct mbuf *m;
	struct tcpiphdr *th;
	struct sockaddr_iso **siso;
{
	struct in_addr src, dst;
	int error, offset, thlen;
	u_long sum;

	sum = 0;
	offset = 0;
	src = th->ti_src;
	dst = th->ti_dst;

	if (th->ti_sum == 0) {
		siso = NULL;
	} else {
		th = mtod(m, struct tcpiphdr *);
	}
	error = tuba_cksum(&sum, &offset, siso, dst.s_addr);
	if (error) {
		m_freem(m);
		return (EADDRNOTAVAIL);
	}
	error = tuba_cksum(&sum, &offset, siso, src.s_addr);
	if (error) {
		m_freem(m);
		return (EADDRNOTAVAIL);
	}
	REDUCE(sum, sum);
	if (th->ti_sum == 0) {
		th->ti_sum = sum;
		th = mtod(m, struct tcpiphdr *);
	}
	REDUCE(th->ti_sum, th->ti_sum + (0xffff ^ sum));
	thlen = offset << 2;
	return (tcp_input_checksum(af, m, &th->ti_t, offset, thlen, sum - thlen));
}

int
tuba_udp_checksum(af, m, uh, siso)
	sa_family_t af;
	struct mbuf *m;
	struct udpiphdr *uh;
	struct sockaddr_iso **siso;
{
	struct in_addr src, dst;
	int error, offset;
	u_long sum;

	sum = 0;
	offset = 0;
	src = uh->ui_src;
	dst = uh->ui_dst;

	if (uh->ui_sum == 0) {
		siso = NULL;
	} else {
		uh = mtod(m, struct udpiphdr *);
	}
	error = tuba_cksum(&sum, &offset, siso, dst.s_addr);
	if (error) {
		m_freem(m);
		return (EADDRNOTAVAIL);
	}
	error = tuba_cksum(&sum, &offset, siso, src.s_addr);
	if (error) {
		m_freem(m);
		return (EADDRNOTAVAIL);
	}
	REDUCE(sum, sum);
	if (uh->ui_sum == 0) {
		uh->ui_sum = sum;
		uh = mtod(m, struct udpiphdr *);
	}
	REDUCE(uh->ui_sum, uh->ui_sum + (0xffff ^ sum));
	return (udp_input_checksum(af, m, &uh->ui_u, offset, sum));
}

void
tuba_refcnt(isop, delta)
	struct isopcb *isop;
	int delta;
{
	register struct tuba_cache *tc;
	u_int index, sum;

	if (delta != 1) {
		delta = -1;
	}
	if (isop == 0 || isop->isop_faddr == 0 || isop->isop_laddr == 0
			|| (delta == -1 && isop->isop_tuba_cached == 0)
			|| (delta == 1 && isop->isop_tuba_cached != 0)) {
		return;
	}
	isop->isop_tuba_cached = (delta == 1);
	index = tuba_lookup(tuba_tree, isop->isop_faddr);
	tc = tuba_table[index];
	if ((index != 0) && (tc != NULL) && (delta == 1 || tc->tc_refcnt > 0)) {
		tc->tc_refcnt += delta;
	}
	index = tuba_lookup(tuba_tree, isop->isop_laddr);
	tc = tuba_table[index];
	if ((index != 0) && (tc != NULL) && (delta == 1 || tc->tc_refcnt > 0)) {
		tc->tc_refcnt += delta;
	}
}
