/*	$NetBSD: key.h,v 1.2 2004/03/02 02:22:56 thorpej Exp $	*/
/*	$FreeBSD: src/sys/netipsec/key.h,v 1.1.4.1 2003/01/24 05:11:36 sam Exp $	*/
/*	$KAME: key.h,v 1.21 2001/07/27 03:51:30 itojun Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _NETIPSEC_KEY_H_
#define _NETIPSEC_KEY_H_

#ifdef _KERNEL

struct secpolicy;
struct secpolicyindex;
struct ipsecrequest;
struct secasvar;
struct sockaddr;
struct socket;
struct sadb_msg;
struct sadb_x_policy;
struct secasindex;
union sockaddr_union;

extern	int key_havesp(u_int dir);
extern struct secpolicy *key_allocsp(struct secpolicyindex *, u_int,
	const char*, int);
extern struct secpolicy *key_allocsp2(u_int32_t spi, union sockaddr_union *dst,
	u_int8_t proto, u_int dir, const char*, int);
extern struct secpolicy *key_newsp(const char*, int);
extern struct secpolicy *key_gettunnel(const struct sockaddr *,
	const struct sockaddr *, const struct sockaddr *,
	const struct sockaddr *, const char*, int);
/* NB: prepend with _ for KAME IPv6 compatbility */
extern void _key_freesp(struct secpolicy **, const char*, int);

/*
 * Access to the SADB are interlocked with splsoftnet.  In particular,
 * holders of SA's use this to block accesses by protocol processing
 * that can happen either by network swi's or by continuations that
 * occur on crypto callbacks.  Much of this could go away if
 * key_checkrequest were redone.
 */ 
#define	KEY_ALLOCSP(spidx, dir)					\
	key_allocsp(spidx, dir, __FILE__, __LINE__)
#define	KEY_ALLOCSP2(spi, dst, proto, dir)			\
	key_allocsp2(spi, dst, proto, dir, __FILE__, __LINE__)
#define	KEY_NEWSP()						\
	key_newsp(__FILE__, __LINE__)
#define	KEY_GETTUNNEL(osrc, odst, isrc, idst)			\
	key_gettunnel(osrc, odst, isrc, idst, __FILE__, __LINE__)
#define	KEY_FREESP(spp)						\
	_key_freesp(spp, __FILE__, __LINE__)

extern struct secasvar *key_allocsa(union sockaddr_union *, u_int, u_int32_t,
	const char*, int);
extern void key_freesav(struct secasvar **, const char*, int);

#define	KEY_ALLOCSA(dst, proto, spi)				\
	key_allocsa(dst, proto, spi, __FILE__, __LINE__)
#define	KEY_FREESAV(psav)					\
	key_freesav(psav, __FILE__, __LINE__)

extern void key_freeso(struct socket *);
extern int key_checktunnelsanity(struct secasvar *, u_int, caddr_t, caddr_t);
extern int key_checkrequest(struct ipsecrequest *isr, const struct secasindex *);

extern struct secpolicy *key_msg2sp(struct sadb_x_policy *, size_t, int *);
extern struct mbuf *key_sp2msg(struct secpolicy *);
extern int key_ismyaddr(struct sockaddr *);
extern int key_cmpspidx_exactly(struct secpolicyindex *, struct secpolicyindex *);
extern int key_cmpspidx_withmask(struct secpolicyindex *, struct secpolicyindex *);
extern int key_spdacquire(struct secpolicy *);
extern void key_timehandler(void*);
extern u_long key_random(void);
extern void key_randomfill(void *, size_t);
extern void key_freereg(struct socket *);
extern int key_parse(struct mbuf *, struct socket *);
extern void key_init(void);
extern void key_sa_recordxfer(struct secasvar *, struct mbuf *);
extern void key_sa_routechange(struct sockaddr *);
extern void key_sa_stir_iv(struct secasvar *);

#endif /* defined(_KERNEL) */
#endif /* _NETIPSEC_KEY_H_ */
