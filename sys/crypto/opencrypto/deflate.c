/*	$NetBSD: deflate.c,v 1.2 2003/08/27 00:12:37 thorpej Exp $ */
/*	$FreeBSD: src/sys/opencrypto/deflate.c,v 1.1.2.1 2002/11/21 23:34:23 sam Exp $	*/
/* $OpenBSD: deflate.c,v 1.3 2001/08/20 02:45:22 hugh Exp $ */

/*
 * Copyright (c) 2001 Jean-Jacques Bernard-Gundol (jj@wabbitt.org)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
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

/*
 * This file contains a wrapper around the deflate algo compression
 * functions using the zlib library (see net/zlib.{c,h})
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: deflate.c,v 1.2 2003/08/27 00:12:37 thorpej Exp $");

#include <sys/types.h>
#include <sys/malloc.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <net/zlib.h>

#include <crypto/opencrypto/cryptodev.h>
#include <crypto/opencrypto/deflate.h>

int window_inflate = -1 * MAX_WBITS;
int window_deflate = -12;

/*
 * This function takes a block of data and (de)compress it using the deflate
 * algorithm
 */

static void *
ocf_zalloc(void *nil, u_int type, u_int size)
{
	void *ptr;

	ptr = malloc(type *size, M_CRYPTO_DATA, M_NOWAIT);
	return (ptr);
}

static void
ocf_zfree(void *nil, void *ptr)
{
	free(ptr, M_CRYPTO_DATA);
}

u_int32_t
deflate_global(data, size, decomp, out)
	u_int8_t *data;
	u_int32_t size;
	int decomp;
	u_int8_t **out;
{
	/* decomp indicates whether we compress (0) or decompress (1) */

	z_stream zbuf;
	u_int8_t *output;
	u_int32_t count, result, tocopy;
	int error, i, j;
	struct deflate_buf buf[ZBUF];

	bzero(&zbuf, sizeof(z_stream));
	for (j = 0; j < ZBUF; j++) {
		buf[j].flag = 0;
	}

	zbuf.next_in = data;	/* data that is going to be processed */
	zbuf.zalloc = ocf_zalloc;
	zbuf.zfree = ocf_zfree;
	zbuf.opaque = Z_NULL;
	zbuf.avail_in = size;	/* Total length of data to be processed */

	if (!decomp) {
		buf[0].size = size;
		buf[i].flag = 1;
		i++;
	} else {
		/*
		 * Choose a buffer with 4x the size of the input buffer
		 * for the size of the output buffer in the case of
		 * decompression. If it's not sufficient, it will need to be
		 * updated while the decompression is going on
		 */

		buf[0].size = size * 4;
		buf[i].flag = 1;
		i++;
	}

	buf[0].out = malloc(buf[0].size, M_CRYPTO_DATA, M_NOWAIT);
	if (buf[i].out == NULL) {
		return (0);
	}
	i = 1;

	zbuf.next_out = buf[0].out;
	zbuf.avail_out = buf[0].size;

	error = decomp ?
			inflateInit2(&zbuf, window_inflate) :
			deflateInit2(&zbuf, Z_DEFAULT_COMPRESSION, Z_METHOD, window_deflate,
					Z_MEMLEVEL, Z_DEFAULT_STRATEGY);

	if (error != Z_OK) {
		goto bad2;
	}
	for (;;) {
		error = decomp ?
				inflate(&zbuf, Z_SYNC_FLUSH) : deflate(&zbuf, Z_FINISH);
		if (error == Z_STREAM_END) { /* success */
			break;
			/*
			 * XXX compensate for two problems:
			 * -Former versions of this code didn't set Z_FINISH
			 *  on compression, so the compressed data are not correctly
			 *  terminated and the decompressor doesn't get Z_STREAM_END.
			 *  Accept such packets for interoperability.
			 * -sys/net/zlib.c has a bug which makes that Z_BUF_ERROR is
			 *  set after successful decompression under rare conditions.
			 */
		} else if (decomp && (error == Z_OK || error == Z_BUF_ERROR)
				&& zbuf.avail_in == 0 && zbuf.avail_out != 0) {
			break;
		} else if (error != Z_OK) {
			goto bad;
		} else if (zbuf.avail_out == 0) {
			/* we need more output space, allocate size */
			int nextsize = buf[i - 1].size * 2;
			if (i == ZBUF || nextsize > 1000000) {
				goto bad;
			}
			buf[i].out = malloc(nextsize, M_CRYPTO_DATA, M_NOWAIT);
			if (buf[i].out == NULL) {
				goto bad;
			}
			zbuf.next_out = buf[i].out;
			zbuf.avail_out = buf[i].size = nextsize;
			i++;
		}
	}

	result = count = zbuf.total_out;

	if (i != 1) { /* copy everything into one buffer */
		output = malloc(result, M_CRYPTO_DATA, M_NOWAIT);
		if (output == NULL) {
			goto bad;
		}
		*out = output;
		for (j = 0; j < i; j++) {
			tocopy = MIN(count, buf[j].size);
			/* XXX the last buf can be empty */
			KASSERT(tocopy || j == (i - 1));
			memcpy(output, buf[j].out, tocopy);
			output += tocopy;
			free(buf[j].out, M_CRYPTO_DATA);
			count -= tocopy;
		}
		KASSERT(count == 0);
	} else {
		*out = buf[0].out;
	}
	if (decomp) {
		inflateEnd(&zbuf);
	} else {
		deflateEnd(&zbuf);
	}
	return (result);

bad:
	if (decomp) {
		inflateEnd(&zbuf);
	} else {
		deflateEnd(&zbuf);
	}
bad2:
	for (j = 0; j < i; j++) {
		free(buf[j].out, M_CRYPTO_DATA);
	}
	return (0);
}
