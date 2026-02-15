/*	$NetBSD: xform.h,v 1.1 2003/08/13 20:06:51 jonathan Exp $	*/
/*	$FreeBSD: src/sys/netipsec/xform.h,v 1.1.4.1 2003/01/24 05:11:36 sam Exp $	*/
/*	$OpenBSD: ip_ipsp.h,v 1.119 2002/03/14 01:27:11 millert Exp $	*/
/*
 * The authors of this code are John Ioannidis (ji@tla.org),
 * Angelos D. Keromytis (kermit@csd.uch.gr),
 * Niels Provos (provos@physnet.uni-hamburg.de) and
 * Niklas Hallqvist (niklas@appli.se).
 *
 * The original version of this code was written by John Ioannidis
 * for BSD/OS in Athens, Greece, in November 1995.
 *
 * Ported to OpenBSD and NetBSD, with additional transforms, in December 1996,
 * by Angelos D. Keromytis.
 *
 * Additional transforms and features in 1997 and 1998 by Angelos D. Keromytis
 * and Niels Provos.
 *
 * Additional features in 1999 by Angelos D. Keromytis and Niklas Hallqvist.
 *
 * Copyright (c) 1995, 1996, 1997, 1998, 1999 by John Ioannidis,
 * Angelos D. Keromytis and Niels Provos.
 * Copyright (c) 1999 Niklas Hallqvist.
 * Copyright (c) 2001, Angelos D. Keromytis.
 *
 * Permission to use, copy, and modify this software with or without fee
 * is hereby granted, provided that this entire notice is included in
 * all copies of any software which is or includes a copy or
 * modification of this software.
 * You may use this code under the GNU public license if you so wish. Please
 * contribute changes back to the authors under this freer than GPL license
 * so that we may further the use of strong encryption without limitations to
 * all.
 *
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTY. IN PARTICULAR, NONE OF THE AUTHORS MAKES ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE
 * MERCHANTABILITY OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR
 * PURPOSE.
 */

#ifndef _KAME_IPSEC_XFORM_H_
#define _KAME_IPSEC_XFORM_H_

/*
 * Kame IPsec xform Notes:
 * To maximize compatibility and minimize breaking kame ipsec.
 * The xform implementation is very minimal, only providing the essential xform device driver
 * needs, required to run properly.
 * In keeping with the above and unlike netipsec very little of the cryptographic calculations
 * are actually performed within the xform driver functions. Rather most functions are compatibility shims,
 * translating kame's ipsec cryptography to their equivalent in opencrypto's xform.
 *
 * An example of input/output:
 *  - open crypto session -> perform kame's crypto code -> close crypto session.
 *
 * The likely side effect of this approach is it will perform better than the original but worse
 * than netipsec.
 */

#include <sys/types.h>
#include <netinet/in.h>
#include <crypto/opencrypto/xform.h>

#define	AH_HMAC_MAX_HASHLEN	32	/* 256 bits of authenticator for SHA512 */
#define	AH_HMAC_INITIAL_RPL	1	/* replay counter initial value */

union sockaddr_union {
	struct sockaddr         sa;
	struct sockaddr_in      sin;
	struct sockaddr_in6     sin6;
};

/* tunnel block descriptor */
struct tdb {
	struct secasvar         *tdb_sav;			/* secasaver */

	struct ipsecrequest 	*tdb_isr;			/* ipsec request state */
	u_int32_t				tdb_spi;			/* associated SPI */
	u_int16_t				tdb_length; 		/* length */
	u_int16_t 				tdb_offset;			/* offset */

    struct sockaddr_storage tdb_src;			/* src addr of packet */
    struct sockaddr_storage tdb_dst;			/* dst addr of packet */
    u_int8_t	            tdb_proto;			/* current protocol, e.g. AH */
	int						tdb_skip;			/* data offset */

	struct xformsw 			*tdb_xform;			/* transform */
	const struct enc_xform 	*tdb_encalgxform;	/* encoding algorithm */
	const struct auth_hash 	*tdb_authalgxform;	/* authentication algorithm */
	const struct comp_algo 	*tdb_compalgxform;	/* compression algorithm */
	u_int64_t 				tdb_cryptoid;		/* crypto session id */
};

/*
 * Packet tag assigned on completion of IPsec processing; used
 * to speedup processing when/if the packet comes back for more
 * processing.
 */
struct tdb_ident {
	u_int32_t 				spi;
	union sockaddr_union 	dst;
	u_int8_t 				proto;
};

/*
 * Opaque data structure hung off a crypto operation descriptor.
 */
struct tdb_crypto {
	struct ipsecrequest		*tc_isr;	/* ipsec request state */
	u_int32_t				tc_spi;		/* associated SPI */
	union sockaddr_union	tc_dst;		/* dst addr of packet */
	u_int8_t				tc_proto;	/* current protocol, e.g. AH */
	u_int8_t				tc_nxt;		/* next protocol, e.g. IPV4 */
	int						tc_protoff;	/* current protocol offset */
	int						tc_skip;	/* data offset */
	caddr_t					tc_ptr;		/* associated crypto data */
};

struct secasvar;
struct ipescrequest;

struct xform_state {
	struct secasvar 	*xs_sav;
	void 				*xs_ctx;
};

struct xformsw {
	u_short				xf_type;		/* xform ID */
#define	XF_IP4			1				/* IP inside IP */
#define	XF_AH			2				/* AH */
#define	XF_ESP			3				/* ESP */
#define	XF_TCPSIGNATURE	5				/* TCP MD5 Signature option, RFC 2358 */
#define	XF_IPCOMP		6				/* IPCOMP */
	u_short				xf_flags;
#define	XFT_AUTH		0x0001
#define	XFT_CONF		0x0100
#define	XFT_COMP		0x1000
	char 				*xf_name;		/* human-readable name */
	int	(*xf_init)(struct secasvar*, struct xformsw *);	/* setup */
	int	(*xf_zeroize)(struct secasvar*);		/* cleanup */
	int	(*xf_input)(struct mbuf *, struct secasvar*, int, int, int);/* input */
	int	(*xf_output)(struct mbuf *, struct ipsecrequest *, struct mbuf **, int, int, int);	/* output */
	struct xformsw *xf_next;		/* list of registered xforms */
};

#ifdef _KERNEL

void xform_register(struct xformsw*);
int xform_init(struct secasvar *sav, int xftype);

struct tdb *tdb_alloc(int);
void tdb_free(struct tdb *);
struct tdb *tdb_init(struct secasvar *, struct xformsw *, const struct enc_xform *, const struct auth_hash *, const struct comp_algo *, u_int8_t);
int tdb_zeroize(struct tdb *);

struct sockaddr_in *tdb_get_sin(struct tdb *, int);
struct sockaddr_in6 *tdb_get_sin6(struct tdb *, int);
struct in_addr *tdb_get_in(struct tdb *, int);
struct in6_addr *tdb_get_in6(struct tdb *, int);

void tdb_keycleanup(struct secasvar *);
int tdb_keysetsav(struct secasvar *, int);

/* External declarations of per-file init functions */
void ah_attach(void);
void esp_attach(void);
void ipcomp_attach(void);
//extern void ipe4_attach(void);
void ipsec_attach(void);

#endif /* _KERNEL */
#endif /* _KAME_IPSEC_XFORM_H_ */
