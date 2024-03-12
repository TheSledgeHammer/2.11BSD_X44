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
 *	@(#)llc_subr.c	8.1 (Berkeley) 6/10/93
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/domain.h>
#include <sys/socket.h>
#include <sys/protosw.h>
#include <sys/socketvar.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/kernel.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_llc.h>
#include <net/route.h>

/*
 * Functions dealing with struct sockaddr_dl
 */

/* Compare sdl_a w/ sdl_b */
int
sockaddr_dl_cmp(struct sockaddr_dl *sdl_a, struct sockaddr_dl *sdl_b)
{
	if (LLADDRLEN(sdl_a) != LLADDRLEN(sdl_b)) {
		return (1);
	}
	return (bcmp((caddr_t)sdl_a->sdl_data, (caddr_t)sdl_b->sdl_data, LLADDRLEN(sdl_a)));
}

/* Copy sdl_f to sdl_t of len */
void
sockaddr_dl_copylen(struct sockaddr_dl *sdl_f, struct sockaddr_dl *sdl_t, u_char len)
{
	bcopy(sdl_f, sdl_t, len);
}

/* Copy sdl_f to sdl_t */
void
sockaddr_dl_copy(struct sockaddr_dl *sdl_f, struct sockaddr_dl *sdl_t)
{
	sockaddr_dl_copylen(sdl_f, sdl_t, sdl_f->sdl_len);
}

void
sockaddr_dl_copyaddrlen(struct sockaddr_dl *sdl_f, struct sockaddr_dl *sdl_t, u_char len)
{
	bcopy(LLADDR(sdl_f), LLADDR(sdl_t), len);
}

void
sockaddr_dl_copyaddr(struct sockaddr_dl *sdl_f, struct sockaddr_dl *sdl_t)
{
	sockaddr_dl_copyaddrlen(sdl_f, sdl_t, sdl_f->sdl_len);
}

/* Swap sdl_a w/ sdl_b */
void
sockaddr_dl_swapaddr(struct sockaddr_dl *sdl_a, struct sockaddr_dl *sdl_b)
{
	struct sockaddr_dl sdl_tmp;

	sockaddr_dl_copy(sdl_a, &sdl_tmp);
	sockaddr_dl_copy(sdl_b, sdl_a);
	sockaddr_dl_copy(&sdl_tmp, sdl_b);
}

/* Fetch the sdl of the associated if */

struct sockaddr_dl *
sockaddr_dl_getaddrif(struct ifnet *ifp, sa_family_t af)
{
	register struct ifaddr *ifa;

	TAILQ_FOREACH(ifa, &ifp->if_addrlist, ifa_list) {
		if (ifa->ifa_addr->sa_family == af) {
			return((struct sockaddr_dl *)(ifa->ifa_addr));
		}
	}
	return (NULL);
}

/* Check addr of interface with the one given */
int
sockaddr_dl_checkaddrif(struct ifnet *ifp, struct sockaddr_dl *sdl_c, sa_family_t af)
{
	register struct ifaddr *ifa;

	TAILQ_FOREACH(ifa, &ifp->if_addrlist, ifa_list) {
		if ((ifa->ifa_addr->sa_family == af)
				&& !sockaddr_dl_cmp((struct sockaddr_dl*) (ifa->ifa_addr), sdl_c)) {
			return (1);
		}
	}
	return (0);
}

/* Build an sdl from MAC addr, DLSAP addr, and interface */
int
sockaddr_dl_setaddrif(struct ifnet *ifp, u_char *mac_addr, u_char dlsap_addr, u_char mac_len, struct sockaddr_dl *sdl_to, sa_family_t af)
{
	register struct sockaddr_dl *sdl_tmp;

	sdl_tmp = sockaddr_dl_getaddrif(ifp, af);
	if (sdl_tmp) {
		sockaddr_dl_copy(sdl_tmp, sdl_to);
		bcopy((caddr_t)mac_addr, (caddr_t)LLADDR(sdl_to), mac_len);
		*(LLADDR(sdl_to) + mac_len) = dlsap_addr;
		sdl_to->sdl_alen = mac_len + 1;
		return (1);
	} else {
		return (0);
	}
}

/* create sockaddr_dl_header */
struct sockaddr_dl_header *
sockaddr_dl_createhdr(struct mbuf *m)
{
    struct sockaddr_dl_header *sdlhdr;

    sdlhdr = mtod(m, struct sockaddr_dl_header *);
    return (sdlhdr);
}

/* Compare the sockaddr_dl header source and destination aggregate */
int
sockaddr_dl_cmphdrif(struct ifnet *ifp, u_char *mac_src, u_char dlsap_src, u_char *mac_dst, u_char dlsap_dst, u_char mac_len, struct sockaddr_dl_header *sdlhdr_to, sa_family_t af)
{
	if (!sockaddr_dl_setaddrif(ifp, mac_src, dlsap_src, mac_len, &sdlhdr_to->sdlhdr_src, af) ||
			!sockaddr_dl_setaddrif(ifp, mac_dst, dlsap_dst, mac_len, &sdlhdr_to->sdlhdr_dst, af)) {
		return (0);
	} else {
		return (1);
	}
}

/* Fill out the sockaddr_dl header aggregate */
int
sockaddr_dl_sethdrif(struct ifnet *ifp, u_char *mac_src, u_char dlsap_src, u_char *mac_dst, u_char dlsap_dst, u_char mac_len, struct mbuf *m, sa_family_t af)
{
	struct sockaddr_dl_header *sdlhdr;

	sdlhdr = sockaddr_dl_createhdr(m);
	return (sockaddr_dl_cmphdrif(ifp, mac_src, dlsap_src, mac_dst, dlsap_dst, mac_len, sdlhdr, af));
}
