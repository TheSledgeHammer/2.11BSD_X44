/* $NetBSD: kern_uuid.c,v 1.1 2004/01/29 02:00:03 tsarna Exp $ */
/* $FreeBSD: /repoman/r/ncvs/src/sys/kern/kern_uuid.c,v 1.7 2004/01/12 13:34:11 rse Exp $ */

/*
 * Copyright (c) 2002 Marcel Moolenaar
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_uuid.c,v 1.1 2004/01/29 02:00:03 tsarna Exp $");

#include <sys/param.h>
#include <sys/endian.h>
#include <sys/kernel.h>
#include <sys/lock.h>
#include <sys/systm.h>
#include <sys/uuid.h>

#include <sys/proc.h>
#include <sys/user.h>
#include <sys/mount.h>
#include <sys/sysdecl.h>
#include <sys/uio.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>

/*
 * See also:
 *	http://www.opengroup.org/dce/info/draft-leach-uuids-guids-01.txt
 *	http://www.opengroup.org/onlinepubs/009629399/apdxa.htm
 *
 * Note that the generator state is itself an UUID, but the time and clock
 * sequence fields are written in the native byte order.
 */

/* We use an alternative, more convenient representation in the generator. */
struct uuid_private {
	union {
		uint64_t		ll;		/* internal. */
		struct {
			uint32_t	low;
			uint16_t	mid;
			uint16_t	hi;
		} x;
	} time;
	uint16_t			seq;			/* Big-endian. */
	uint16_t			node[UUID_NODE_LEN>>1];
};

CTASSERT(sizeof(struct uuid_private) == 16);

static struct uuid_private uuid_last;

/* "UUID generator mutex lock" */
static struct lock_object uuid_mutex;

void
uuid_init(void)
{
	simple_lock_init(&uuid_mutex, "uuid_mutex");
}

/*
 * Return the first MAC address we encounter or, if none was found,
 * construct a sufficiently random multicast address. We don't try
 * to return the same MAC address as previously returned. We always
 * generate a new multicast address if no MAC address exists in the
 * system.
 * It would be nice to know if 'ifnet' or any of its sub-structures
 * has been changed in any way. If not, we could simply skip the
 * scan and safely return the MAC address we returned before.
 */
static void
uuid_node(node)
	uint16_t *node;
{
	struct ifnet *ifp;
	struct ifaddr *ifa;
	struct sockaddr_dl *sdl;
	int i, s;

	s = splnet();
	TAILQ_FOREACH(ifp, &ifnet, if_list)	{
		/* Walk the address list */
		TAILQ_FOREACH(ifa, &ifp->if_addrlist, ifa_list)	{
			sdl = (struct sockaddr_dl*) ifa->ifa_addr;
			if (sdl != NULL && sdl->sdl_family == AF_LINK
					&& sdl->sdl_type == IFT_ETHER) {
				/* Got a MAC address. */
				bcopy(LLADDR(sdl), node, UUID_NODE_LEN);
				splx(s);
				return;
			}
		}
	}
	splx(s);

	for (i = 0; i < (UUID_NODE_LEN>>1); i++) {
		node[i] = (uint16_t)arc4random();
	}
	*((uint8_t*)node) |= 0x01;
}

/*
 * Get the current time as a 60 bit count of 100-nanosecond intervals
 * since 00:00:00.00, October 15,1582. We apply a magic offset to convert
 * the Unix time since 00:00:00.00, January 1, 1970 to the date of the
 * Gregorian reform to the Christian calendar.
 */
static uint64_t
uuid_time(void)
{
	struct timeval tvp;
	uint64_t xtime = 0x01B21DD213814000LL;

	microtime(&tvp);
	xtime += (uint64_t)tvp.tv_sec * 10000000LL;
	xtime += (uint64_t)(10 * tvp.tv_usec);
	return (xtime & ((1LL << 60) - 1LL));
}

/*
 * Internal routine to actually generate the UUID.
 */
static void
uuid_generate(uuid, timep, count)
	struct uuid_private *uuid;
	uint64_t *timep;
	int count;
{
	uint64_t xtime;

	simple_lock(&uuid_mutex);

	uuid_node(uuid->node);
	xtime = uuid_time();
	*timep = xtime;
	if (uuid_last.time.ll == 0LL || uuid_last.node[0] != uuid->node[0] ||
	    uuid_last.node[1] != uuid->node[1] ||
	    uuid_last.node[2] != uuid->node[2]) {
		uuid->seq = (uint16_t)arc4random() & 0x3fff;
	} else if (uuid_last.time.ll >= xtime) {
		uuid->seq = (uuid_last.seq + 1) & 0x3fff;
	} else {
		uuid->seq = uuid_last.seq;
	}

	uuid_last = *uuid;
	uuid_last.time.ll = (xtime + count - 1) & ((1LL << 60) - 1LL);

	simple_unlock(&uuid_mutex);
}

static int
kern_uuidgen(store, count, to_user)
	struct uuid *store;
	int count;
	bool_t to_user;
{
	struct uuid_private uuid;
	uint64_t xtime;
	int error = 0, i;

	KASSERT(count >= 1);

	/* Generate the base UUID. */
	uuid_generate(&uuid, &xtime, count);

	/* Set sequence and variant and deal with byte order. */
	uuid.seq = htobe16(uuid.seq | 0x8000);

	for (i = 0; i < count; xtime++, i++) {
		/* Set time and version (=1) and deal with byte order. */
		uuid.time.x.low = (uint32_t)xtime;
		uuid.time.x.mid = (uint16_t)(xtime >> 32);
		uuid.time.x.hi = ((uint16_t)(xtime >> 48) & 0xfff) | (1 << 12);
		if (to_user) {
			error = copyout(&uuid, store + i, sizeof(uuid));
			if (error != 0) {
				break;
			}
		} else {
			bcopy(&uuid, store + i, sizeof(uuid));
		}
	}
	return (error);
}

int
uuidgenerate(store, count)
	struct uuid *store;
	int count;
{
	return (kern_uuidgen(store, count, FALSE));
}

int
uuidgen()
{
	register struct uuidgen_args {
		syscallarg(struct uuid *) store;
		syscallarg(int) count;
	} *uap = (struct uuidgen_args *)u.u_ap;
	struct uuid_private uuid;
	uint64_t xtime;
	int error;

	if (SCARG(uap,count) < 1 || SCARG(uap,count) > 2048) {
		return (EINVAL);
	}
	return (kern_uuidgen(SCARG(uap, store), SCARG(uap,count), TRUE));
}

int
snprintf_uuid(buf, sz, uuid)
	char *buf;
	size_t sz;
	struct uuid *uuid;
{
	struct uuid_private *id;
	int cnt;

	id = (struct uuid_private*) uuid;
	cnt = snprintf(buf, sz, "%08x-%04x-%04x-%04x-%04x%04x%04x", id->time.x.low,
			id->time.x.mid, id->time.x.hi, be16toh(id->seq),
			be16toh(id->node[0]), be16toh(id->node[1]), be16toh(id->node[2]));
	return (cnt);
}

int
printf_uuid(uuid)
	struct uuid *uuid;
{
	char buf[38];

	snprintf_uuid(buf, sizeof(buf), uuid);
	printf("%s", buf);
	return (0);
}

/*
 * Encode/Decode UUID into byte-stream.
 *   http://www.opengroup.org/dce/info/draft-leach-uuids-guids-01.txt
 *
 * 0                   1                   2                   3
 *   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                          time_low                             |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |       time_mid                |         time_hi_and_version   |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |clk_seq_hi_res |  clk_seq_low  |         node (0-1)            |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         node (2-5)                            |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

void
le_uuid_enc(buf, uuid)
	void *buf;
	struct uuid const *uuid;
{
	u_char *p;
	int i;

	p = buf;
	le32enc(p, uuid->time_low);
	le16enc(p + 4, uuid->time_mid);
	le16enc(p + 6, uuid->time_hi_and_version);
	p[8] = uuid->clock_seq_hi_and_reserved;
	p[9] = uuid->clock_seq_low;
	for (i = 0; i < _UUID_NODE_LEN; i++) {
		p[10 + i] = uuid->node[i];
	}
}

void
le_uuid_dec(buf, uuid)
	void const *buf;
	struct uuid *uuid;
{
	u_char const *p;
	int i;

	p = buf;
	uuid->time_low = le32dec(p);
	uuid->time_mid = le16dec(p + 4);
	uuid->time_hi_and_version = le16dec(p + 6);
	uuid->clock_seq_hi_and_reserved = p[8];
	uuid->clock_seq_low = p[9];
	for (i = 0; i < _UUID_NODE_LEN; i++) {
		uuid->node[i] = p[10 + i];
	}
}
void
be_uuid_enc(buf, uuid)
	void *buf;
	struct uuid const *uuid;
{
	u_char *p;
	int i;

	p = buf;
	be32enc(p, uuid->time_low);
	be16enc(p + 4, uuid->time_mid);
	be16enc(p + 6, uuid->time_hi_and_version);
	p[8] = uuid->clock_seq_hi_and_reserved;
	p[9] = uuid->clock_seq_low;
	for (i = 0; i < _UUID_NODE_LEN; i++) {
		p[10 + i] = uuid->node[i];
	}
}

void
be_uuid_dec(buf, uuid)
	void const *buf;
	struct uuid *uuid;
{
	u_char const *p;
	int i;

	p = buf;
	uuid->time_low = be32dec(p);
	uuid->time_mid = le16dec(p + 4);
	uuid->time_hi_and_version = be16dec(p + 6);
	uuid->clock_seq_hi_and_reserved = p[8];
	uuid->clock_seq_low = p[9];
	for (i = 0; i < _UUID_NODE_LEN; i++) {
		uuid->node[i] = p[10 + i];
	}
}
