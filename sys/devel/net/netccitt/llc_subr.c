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
#include <sys/queue.h>
#include <sys/types.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_llc.h>
#include <net/route.h>

#include <netccitt/dll.h>
#include <netccitt/llc_var.h>

/*
 * Trace level
 */
int llc_tracelevel = LLCTR_URGENT;

/*
 * We keep the link control blocks on a doubly linked list -
 * primarily for checking in llc_time()
 */
struct llccb_q llccb_q;

static struct sockaddr_dl sap_saddr;
static struct sockaddr_dl sap_sgate = {
		.sdl_len = sizeof(struct sockaddr_dl), 		/* _len */
		.sdl_family = AF_LINK,					  	/* _af */
};

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
sockaddr_dl_cmphdrif(struct ifnet *ifp, u_char *mac_src, u_char dlsap_src, u_char *mac_dst, u_char dlsap_dst, u_char mac_len, struct sockaddr_dl_header *sdlhdr_to)
{
	if (!sockaddr_dl_setaddrif(ifp, mac_src, dlsap_src, mac_len, &sdlhdr_to->sdlhdr_src) ||
			!sockaddr_dl_setaddrif(ifp, mac_dst, dlsap_dst, mac_len, &sdlhdr_to->sdlhdr_dst)) {
		return (0);
	} else {
		return (1);
	}
}

/* Fill out the sockaddr_dl header aggregate */
int
sockaddr_dl_sethdrif(struct ifnet *ifp, u_char *mac_src, u_char dlsap_src, u_char *mac_dst, u_char dlsap_dst, u_char mac_len, struct mbuf *m)
{
	struct sockaddr_dl_header *sdlhdr;
	sdlhdr = sockaddr_dl_createhdr(m);
	return (sockaddr_dl_cmphdrif(ifp, mac_src, dlsap_src, mac_dst, dlsap_dst, mac_len, sdlhdr));
}

/*
 * Set sapinfo for SAP address, llcconfig, af, and interface
 */
struct llc_sapinfo *
llc_setsapinfo(struct ifnet *ifp, u_char af, u_char sap, struct dllconfig *llconf)
{
	struct protosw *pp;
	struct sockaddr_dl *ifdl_addr;
	struct rtentry *sirt;
	struct llc_sapinfo *sapinfo;
	u_char saploc;
	int size;

	sirt = NULL;
	size = sizeof(struct llc_sapinfo);

	/*
	 * We rely/assume that only STREAM protocols will make use of
	 * connection oriented LLC2. If this will one day not be the
	 * case this will obviously fail.
	 */
	pp = pffindtype(af, SOCK_STREAM);
	if (pp == 0 || pp->pr_input == 0 || pp->pr_ctlinput == 0) {
		printf("network	level protosw error");
		return (NULL);
	}

	/*
	 * We need a way to jot down the LLC2 configuration for
	 * a certain LSAP address. To do this we enter
	 * a "route" for the SAP.
	 */
	ifdl_addr = sockaddr_dl_getaddrif(ifp);
	sockaddr_dl_copy(ifdl_addr, &sap_saddr);
	sockaddr_dl_copy(ifdl_addr, &sap_sgate);
	saploc = LLSAPLOC(&sap_saddr, ifp);
	sap_saddr.sdl_data[saploc] = sap;
	sap_saddr.sdl_alen++;

	/* now enter it */
	rtrequest(RTM_ADD, (struct sockaddr *)&sap_saddr, (struct sockaddr *)&sap_sgate, 0, 0, &sirt);
	if (sirt == 0) {
		return (NULL);
	}

	/* Plug in config information in rt->rt_llinfo */

	sirt->rt_llinfo = malloc(size , M_PCB, M_WAITOK);
	sapinfo = (struct llc_sapinfo *)sirt->rt_llinfo;
	if (sapinfo) {
		bzero((caddr_t)sapinfo, size);
		/*
		 * For the time being we support LLC CLASS II here
		 * only
		 */
		sapinfo->si_class = LLC_CLASS_II;
		sapinfo->si_window = llconf->dllcfg_window;
		sapinfo->si_trace = llconf->dllcfg_trace;
		if (sapinfo->si_trace) {
			llc_tracelevel--;
		} else {
			llc_tracelevel++;
		}
		sapinfo->si_input = pp->pr_input;
		sapinfo->si_ctlinput = pp->pr_ctlinput;
		return (sapinfo);
	}
	return (NULL);
}

/*
 * Get sapinfo for SAP address and interface
 */
struct llc_sapinfo *
llc_getsapinfo(u_char sap, struct ifnet *ifp)
{
	struct sockaddr_dl *ifdl_addr;
	struct sockaddr_dl si_addr;
	struct rtentry *sirt;
	u_char saploc;

	ifdl_addr = sockaddr_dl_getaddrif(ifp);
	sockaddr_dl_copy(ifdl_addr, &si_addr);
	saploc = LLSAPLOC(&si_addr, ifp);
	si_addr.sdl_data[saploc] = sap;
	si_addr.sdl_alen++;
	sirt = rtalloc1((struct sockaddr *)&si_addr, 0);
	if (sirt != NULL) {
		sirt->rt_refcnt--;
	} else {
		return (0);
	}
	return ((struct llc_sapinfo *)sirt->rt_llinfo);
}

void
llc_init()
{
	rn_inithead((void **)&rt_tables[AF_LINK], 32);

	TAILQ_INIT(&llccb_q);

	llcintrq.ifq_maxlen = IFQ_MAXLEN;
}

struct llc_linkcb *
llc_alloc(void)
{
	struct llc_linkcb *nlinkp;

	/* allocate memory for link control block */
	MALLOC(nlinkp, struct llc_linkcb *, sizeof(struct llc_linkcb), M_PCB, M_DONTWAIT);
	if (nlinkp == NULL) {
		return (NULL);
	}
	return (nlinkp);
}

void
llc_free(struct llc_linkcb *nlinkp)
{
	if (nlinkp == NULL) {
		return;
	}
	FREE(nlinkp, M_PCB);
}

struct llc_linkcb *
llc_newlink(struct sockaddr_dl *dst, struct ifnet *ifp, struct rtentry *nlrt, caddr_t nlnext, struct rtentry *llrt)
{
	struct llc_linkcb *nlinkp;
	u_char sap;
	short llcwindow;

	sap = LLSAPADDR(dst);
	nlinkp = llc_alloc();
	if (nlinkp == NULL) {
		return (NULL);
	}
	bzero((caddr_t)nlinkp, sizeof(struct llc_linkcb));

	/* copy link address */
	sockaddr_dl_copy(dst, &nlinkp->llcl_addr);

	/* hold on to the network layer route entry */
	nlinkp->llcl_nlrt = nlrt;

	/* likewise the network layer control block */
	nlinkp->llcl_nlnext = nlnext;

	/* jot down the link layer route entry */
	nlinkp->llcl_llrt = llrt;

	/* reset writeq */
	nlinkp->llcl_wqhead = nlinkp->llcl_wqtail = NULL;

	/* hold on to interface pointer */
	nlinkp->llcl_if = ifp;

	/* get service access point information */
	nlinkp->llcl_sapinfo = llc_getsapinfo(sap, ifp);

	/* get window size from SAP info block */
	llcwindow = nlinkp->llcl_sapinfo->si_window;
	if (llcwindow == 0) {
		llcwindow = LLC_MAX_WINDOW;
	}

	/* allocate memory for window buffer */
	MALLOC(nlinkp->llcl_output_buffers, struct mbuf **, llcwindow*sizeof(struct mbuf *), M_PCB, M_DONTWAIT);
	if (nlinkp->llcl_output_buffers == 0) {
		llc_free(nlinkp);
		return (NULL);
	}

	/* set window size & slotsfree */
	nlinkp->llcl_slotsfree = nlinkp->llcl_window = llcwindow;

	/* enter into linked listed of link control blocks */
	TAILQ_INSERT_TAIL(&llccb_q, nlinkp, llcl_q);

	return (nlinkp);
}

/*
 * llc_dellink() --- farewell to link control block
 */
void
llc_dellink(struct llc_linkcb *linkp)
{
	register struct mbuf *m;
	register struct mbuf *n;
	register struct llc_sapinfo *sapinfo;
	register int i;

	sapinfo = linkp->llcl_sapinfo;

	/* notify upper layer of imminent death */
	if (linkp->llcl_nlnext && sapinfo->si_ctlinput) {
		(*sapinfo->si_ctlinput)(PRC_DISCONNECT_INDICATION, (struct sockaddr *)&linkp->llcl_addr, linkp->llcl_nlnext);
	}

	/* pull the plug */
	if (linkp->llcl_llrt) {
		((struct llc_sapinfo *)(linkp->llcl_llrt->rt_llinfo))->np_link =(struct llc_linkcb *)NULL;
	}

	/* leave link control block queue */
	TAILQ_REMOVE(&llccb_q, linkp, llcl_q);

	/* drop queued packets */
	for (m = linkp->llcl_wqhead; m;) {
		n = m->m_act;
		m_freem(m);
		m = n;
	}

	/* drop packets in the window */
	for (i = 0; i < linkp->llcl_window; i++) {
		if (linkp->llcl_output_buffers[i]) {
			m_freem(linkp->llcl_output_buffers[i]);
		}
	}

	/* return the window space */
	FREE((caddr_t)linkp->llcl_output_buffers, M_PCB);

	/* return the control block space --- now it's gone ... */
	llc_free(linkp);
}
