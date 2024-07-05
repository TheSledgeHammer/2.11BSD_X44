/*
 * Copyright (C) Dirk Husemann, Computer Science Department IV,
 * 		 University of Erlangen-Nuremberg, Germany, 1990, 1991, 1992
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Dirk Husemann and the Computer Science Department (IV) of
 * the University of Erlangen-Nuremberg, Germany.
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
 *	@(#)pk_llcsubr.c	8.2 (Berkeley) 2/9/95
 */

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_llc.h>
#include <net/if_types.h>
#include <net/route.h>

#include <netccitt/dll.h>

#include <netccitt/llc_var.h>

struct sockaddr_dl npdl_netmask = {
		.sdl_len = 		sizeof(struct sockaddr_dl),	/* _len */
		.sdl_family = 	0,			/* _family */
		.sdl_index = 	0,			/* _index */
		.sdl_type = 	0,			/* _type */
		.sdl_nlen = 	-1,			/* _nlen */
		.sdl_alen = 	-1,			/* _alen */
		.sdl_slen = 	-1,			/* _slen */
		.sdl_data = {
				-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
		},							/* _data */
};
struct sockaddr npdl_dummy;

struct rtentry *
llc_sapinfo_enter(struct sockaddr_dl *key, struct sockaddr *value, struct rtentry *rt, struct llc_linkcb *link)
{
	struct rtentry *nprt;
	int    i;

	nprt = rtalloc1((struct sockaddr *)key, 0);
	if (nprt == NULL) {
		u_int  size = sizeof(struct llc_sapinfo);
		u_char saploc = LLSAPLOC(key, rt->rt_ifp);
		int npdl_datasize = sizeof(struct sockaddr_dl) - ((int) ((unsigned long)&((struct sockaddr_dl *) 0)->sdl_data[0]));

		npdl_netmask.sdl_data[saploc] = NPDL_SAPNETMASK;
		bzero((caddr_t)&npdl_netmask.sdl_data[saploc + 1], npdl_datasize - saploc - 1);
		if (value == 0) {
			value = &npdl_dummy;
		}

		/* now enter it */
		rtrequest(RTM_ADD, SA(key), SA(value), SA(&npdl_netmask), 0, &nprt);

		/* and reset npdl_netmask */
		for (i = saploc; i < npdl_datasize; i++) {
			npdl_netmask.sdl_data[i] = -1;
		}

		nprt->rt_llinfo = malloc(size, M_PCB, M_WAITOK|M_ZERO);
		if (nprt->rt_llinfo) {
			((struct llc_sapinfo *) (nprt->rt_llinfo))->np_rt = rt;
		}
	} else {
		nprt->rt_refcnt--;
	}
	return (nprt);
}

struct rtentry *
llc_sapinfo_enrich(short type, caddr_t info, struct sockaddr_dl *sdl)
{
	struct rtentry *rt;
	rt = rtalloc1((struct sockaddr *) sdl, 0);
	if (rt != NULL) {
		rt->rt_refcnt--;
		switch (type) {
		case 0:
			((struct llc_sapinfo *) (rt->rt_llinfo))->np_link = (struct llc_linkcb *) info;
			break;
		}
		return (rt);
	}
	return (NULL);
}

int
llc_sapinfo_destroy(struct rtentry *rt)
{
	if (rt->rt_llinfo) {
		free((caddr_t) rt->rt_llinfo, M_PCB);
	}
	return (rtrequest(RTM_DELETE, rt_key(rt), rt->rt_gateway, rt_mask(rt), 0, 0));
}

int
x25_llc(int prc, struct sockaddr *addr)
{
	register struct sockaddr_x25 *sx25;
	register struct x25_ifaddr *x25ifa;
	struct dll_ctlinfo ctlinfo;

	sx25 = (struct sockaddr_x25 *)addr;
	if ((x25ifa = (struct x25_ifaddr *)ifa_ifwithaddr(addr)) == 0) {
		return (0);
	}
	ctlinfo.dlcti_cfg = (struct dllconfig *)(((struct sockaddr_x25 *)(&x25ifa->ia_xc))+1);
	ctlinfo.dlcti_lsap = LLC_X25_LSAP;
	return ((int)llc_ctlinput(prc, addr, (caddr_t)&ctlinfo));
}
